#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
// JUCE 8 AudioParameterBool retains fractional normalized writes internally.
// Snap host automation at the parameter boundary so state/GUI/DSP agree exactly.
class SwitchParameter : public juce::RangedAudioParameter {
    std::atomic<float> value;
    const float initial;
    const juce::NormalisableRange<float> range{0.f,1.f,1.f};
public:
    SwitchParameter(const juce::ParameterID& id,const juce::String& name,bool enabled)
        : juce::RangedAudioParameter(id,name,juce::AudioProcessorParameterWithIDAttributes{}),value(enabled?1.f:0.f),initial(enabled?1.f:0.f) {}
    const juce::NormalisableRange<float>& getNormalisableRange() const override { return range; }
    float getValue() const override { return value.load(); }
    void setValue(float normalized) override { value.store(normalized>=.5f?1.f:0.f); }
    float getDefaultValue() const override { return initial; }
    int getNumSteps() const override { return 2; }
    bool isDiscrete() const override { return true; }
    bool isBoolean() const override { return true; }
    juce::String getText(float v,int) const override { return v>=.5f?"On":"Off"; }
    float getValueForText(const juce::String& text) const override {
        return text.equalsIgnoreCase("on")||text.equalsIgnoreCase("true")||text.equalsIgnoreCase("yes")||text.getFloatValue()>=.5f?1.f:0.f;
    }
};
const char* ids[]={"bass","treble","compress","reverb","delay","pitch"}; }
juce::AudioProcessorValueTreeState::ParameterLayout GuitarProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for(auto id:{"input","output"}) l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},id,juce::NormalisableRange<float>{-10.f,10.f,0.1f},0.f));
    const juce::StringArray colors[]={{"Upper","Lower","Sub"},{"Roof","Top","Bite"},{"Wall","Spank","Push"},{"Arena","Hall","Club"},{"Quarter","Edge","Eight"},{"Spreader","Wide","Stereo"}};
    const float defaults[]={0,0,0,0,0,0};
    const int defaultColors[]={2,2,2,2,2,2};
    for(int i=0;i<6;++i) {
        juce::String id=ids[i];
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},id,juce::NormalisableRange<float>{-10.f,10.f,0.1f},defaults[i]));
        l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"Color",1},id+" Color",colors[i],defaultColors[i]));
        l.add(std::make_unique<SwitchParameter>(juce::ParameterID{id+"On",1},id+" On",true));
    }
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"amp",1},"Amp",juce::StringArray{"Clean","Crunch","Heavy","Bypass"},0));
    l.add(std::make_unique<SwitchParameter>(juce::ParameterID{"reamp",1},"Re-Amplify",false));
    // Append new parameters to preserve the existing host parameter order.
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"balance",1},"Balance",juce::NormalisableRange<float>{-6.f,6.f,0.1f},0.f));
    return l;
}
GuitarProcessor::GuitarProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"GuitarRack",layout()) {
    const char* faders[]={"input","bass","treble","compress","reverb","delay","pitch","output"};
    for(size_t i=0;i<8;++i)faderParameters[i]=state.getRawParameterValue(faders[i]);
    for(size_t i=0;i<6;++i){colorParameters[i]=state.getRawParameterValue(juce::String(ids[i])+"Color");enabledParameters[i]=state.getRawParameterValue(juce::String(ids[i])+"On");}
    balanceParameter=state.getRawParameterValue("balance");
    ampParameter=state.getRawParameterValue("amp");reampParameter=state.getRawParameterValue("reamp");
}
bool GuitarProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && (l.getMainInputChannelSet()==juce::AudioChannelSet::mono() || l.getMainInputChannelSet()==juce::AudioChannelSet::stereo());
}
void GuitarProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
    juce::ScopedNoDenormals nd;
    if(getTotalNumInputChannels()==1) b.copyFrom(1,0,b,0,0,b.getNumSamples());
    delayDryInput(b,false);
    guitar::Settings s;s.input=faderParameters[0]->load();s.output=faderParameters[7]->load();s.reamp=reampParameter->load()>.5f;s.amp=(int)ampParameter->load();
    s.balance=getTotalNumInputChannels()==2?balanceParameter->load():0.f;
    if(auto* playHead=getPlayHead())
        if(auto position=playHead->getPosition())
            if(auto bpm=position->getBpm()) s.bpm=*bpm;
    float* f[]={&s.bass,&s.treble,&s.compress,&s.reverb,&s.delay,&s.pitch};
    for(size_t i=0;i<6;++i){*f[i]=faderParameters[i+1]->load();s.color[i]=(int)colorParameters[i]->load();s.enabled[i]=enabledParameters[i]->load()>.5f;}
    for(int c=0;c<2;++c)inputChannels[(size_t)c].store(b.getMagnitude(c,0,b.getNumSamples())*guitar::gain(s.input));
    engine.set(s);engine.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples());
    for(int c=0;c<2;++c)outputChannels[(size_t)c].store(b.getMagnitude(c,0,b.getNumSamples()));
    inMeter.store(engine.inputPeak);outMeter.store(engine.outputPeak);
    if(engine.inputPeak>1.f)inputClipped=true;
    if(engine.outputPeak>1.f)outputClipped=true;
}
void GuitarProcessor::delayDryInput(juce::AudioBuffer<float>& b,bool replaceOutput) {
    if(bypassDelay[0].empty())return;
    for(int n=0;n<b.getNumSamples();++n){
        for(int c=0;c<2;++c){
            auto& sample=bypassDelay[static_cast<size_t>(c)][bypassHead];
            const float delayed=sample;sample=b.getSample(c,n);
            if(replaceOutput)b.setSample(c,n,delayed);
        }
        bypassHead=(bypassHead+1)%bypassDelay[0].size();
    }
}
void GuitarProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
    if(getTotalNumInputChannels()==1)b.copyFrom(1,0,b,0,0,b.getNumSamples());
    delayDryInput(b,true);
}
const std::vector<GuitarProcessor::Preset>& GuitarProcessor::presets() {
    static const std::vector<Preset> p={
      {"Acoustic","Steel Sparkle",0,false,{-2,4,1,0,-10,-7},{1,0,1,1,0,2}},
      {"Acoustic","Nylon Warmth",0,false,{2,1,-2,-2,-10,-10},{1,0,2,1,2,2}},
      {"Acoustic","Acoustic Wide",0,false,{-1,3,2,1,-8,1},{0,0,1,1,1,0}},
      {"Clean","Clean Chorus",0,true,{0,2,1,-2,-7,2},{1,1,1,2,1,0}},
      {"Clean","Clean Bright",0,true,{-2,5,0,-3,-10,-8},{0,0,1,1,2,2}},
      {"Clean","Funk Rhythm",0,true,{-3,3,5,-7,-10,-10},{0,2,1,2,2,2}},
      {"Crunch","Crunch Rhythm",1,true,{2,1,1,-4,-10,-9},{1,1,1,2,1,2}},
      {"Crunch","Rock Grit",1,true,{3,3,3,-5,-8,-8},{0,2,2,2,1,1}},
      {"Crunch","Indie Edge",1,true,{-1,4,0,1,-2,-3},{1,1,1,1,1,0}},
      {"Heavy","Heavy Wall",2,true,{4,1,4,-5,-9,-5},{1,1,0,2,2,2}},
      {"Heavy","Metal Rhythm",2,true,{2,3,2,-8,-10,-10},{2,2,2,2,2,2}},
      {"Heavy","Thick Heavy",2,true,{5,-1,5,-4,-8,-3},{1,0,0,2,2,1}},
      {"Lead / Solo","Lead Solo",2,true,{1,3,6,1,3,-3},{1,1,2,1,0,2}},
      {"Lead / Solo","Big Lead",2,true,{3,2,7,4,4,0},{0,0,0,0,0,1}},
      {"Lead / Solo","Screaming Lead",2,true,{0,5,8,2,3,-2},{1,2,2,1,1,2}},
      {"FX / Special","Ambient Glass",0,true,{-2,4,3,8,6,6},{0,0,1,0,0,0}},
      {"FX / Special","Wide Echo",1,true,{0,1,2,5,8,7},{1,1,1,1,0,1}},
      {"FX / Special","Slap Texture",0,true,{-1,2,1,2,7,4},{0,2,2,2,2,2}},
      {"Acoustic","Steel Studio",0,false,{-2,3,2,-6,-10,-10},{1,0,1,2,0,2},{true,true,true,true,false,false},0,-1},
      {"Acoustic","Nylon Intimate",3,false,{2,1,-1,-5,-10,-10},{1,1,0,2,2,2},{true,true,true,true,false,false},1,-1},
      {"Acoustic","Fingerstyle Air",0,false,{-1,5,3,-1,-8,-6},{0,0,1,1,1,2},{true,true,true,true,true,true},0,-2},
      {"Acoustic","Strum Radio",0,false,{-3,4,5,-3,-10,-10},{0,2,2,2,0,2},{true,true,true,true,false,false},-1,-2},
      {"Acoustic","Acoustic Cathedral",0,false,{1,3,2,7,1,0},{2,0,1,0,0,0},{true,true,true,true,true,true},0,-3},
      {"Acoustic","Acoustic Double",0,false,{0,2,1,-4,-10,4},{1,1,1,2,2,1},{true,true,true,true,false,true},0,-2},
      {"Acoustic","Acoustic Dry DI",3,false,{0,0,0,-10,-10,-10},{0,0,0,2,2,2},{false,false,false,false,false,false},0,0},
      {"Clean","Pop Arpeggio",0,true,{-1,4,3,0,-2,-2},{0,0,1,1,1,2},{true,true,true,true,true,true},0,-2},
      {"Clean","Disco Chops",0,true,{-4,5,6,-6,-10,-10},{0,2,2,2,2,2},{true,true,true,true,false,false},-1,-2},
      {"Clean","Jazz Velvet",0,true,{3,-4,2,-4,-10,-10},{1,0,0,2,2,2},{true,true,true,true,false,false},0,-1},
      {"Clean","Country Snap",0,true,{-2,4,5,-5,0,-10},{0,2,1,2,2,2},{true,true,true,true,true,false},0,-2},
      {"Clean","Dream Chorus",0,true,{0,3,2,3,-1,5},{1,1,0,1,1,0},{true,true,true,true,true,true},0,-3},
      {"Clean","Studio DI Polish",3,false,{-1,2,4,-10,-10,-10},{0,0,1,2,0,2},{true,true,true,false,false,false},1,-2},
      {"Clean","Clean Stereo Rhythm",0,true,{1,2,3,-3,-7,3},{1,1,2,2,1,2},{true,true,true,true,true,true},0,-2},
      {"Crunch","Blues Breakup",1,true,{2,1,0,-3,-6,-10},{1,1,0,2,2,2},{true,true,true,true,true,false},-2,-1},
      {"Crunch","Classic Brit",1,true,{3,3,2,-4,-10,-10},{0,2,1,2,1,2},{true,true,true,true,false,false},1,-2},
      {"Crunch","Punk Downstrokes",1,true,{1,4,4,-8,-10,-10},{1,2,2,2,2,2},{true,true,true,true,false,false},2,-3},
      {"Crunch","Indie Jangle",1,true,{-2,5,1,1,-3,0},{0,0,1,1,1,1},{true,true,true,true,true,true},-1,-2},
      {"Crunch","Garage Room",1,true,{2,2,5,4,-8,-10},{1,1,0,2,2,2},{true,true,true,true,true,false},1,-3},
      {"Crunch","Southern Slide",1,true,{3,0,4,1,0,-5},{1,0,2,1,0,2},{true,true,true,true,true,true},0,-2},
      {"Crunch","Crunch Doubletrack",1,true,{2,2,3,-5,-10,4},{0,2,1,2,2,1},{true,true,true,true,false,true},0,-3},
      {"Heavy","Modern Tight",2,true,{1,4,4,-8,-10,-10},{0,2,2,2,2,2},{true,true,true,true,false,false},1,-3},
      {"Heavy","Drop Tune Punch",2,true,{4,2,5,-7,-10,-10},{2,1,1,2,2,2},{true,true,true,true,false,false},0,-4},
      {"Heavy","Doom Weight",2,true,{7,-2,3,1,-6,-5},{2,0,0,1,0,2},{true,true,true,true,true,true},1,-4},
      {"Heavy","Industrial Edge",2,true,{-1,6,6,-5,-2,2},{0,2,2,2,1,0},{true,true,true,true,true,true},1,-4},
      {"Heavy","Arena Stack",2,true,{4,3,4,3,-4,2},{1,1,0,0,0,1},{true,true,true,true,true,true},0,-4},
      {"Heavy","Heavy Dry Focus",2,true,{2,2,2,-10,-10,-10},{0,1,2,2,2,2},{true,true,true,false,false,false},0,-2},
      {"Heavy","Heavy Stereo Wall",2,true,{5,2,5,-3,-8,5},{1,2,0,2,1,2},{true,true,true,true,true,true},0,-4},
      {"Lead / Solo","Blues Lead",1,true,{2,1,5,0,2,-8},{1,0,1,1,2,2},{true,true,true,true,true,true},1,-2},
      {"Lead / Solo","Singing Sustain",2,true,{2,2,8,2,2,-4},{0,1,0,1,0,2},{true,true,true,true,true,true},0,-3},
      {"Lead / Solo","Fusion Smooth",1,true,{3,-1,6,0,1,-5},{1,0,2,2,1,2},{true,true,true,true,true,true},0,-2},
      {"Lead / Solo","Focused Width Lead",2,true,{1,4,7,3,3,5},{0,2,1,0,1,0},{true,true,true,true,true,true},0,-4},
      {"Lead / Solo","Ballad Solo",1,true,{2,2,6,5,4,1},{1,1,0,0,0,1},{true,true,true,true,true,true},0,-3},
      {"Lead / Solo","Dry Forward Solo",2,true,{0,3,8,-10,-10,-10},{0,2,2,2,2,2},{true,true,true,false,false,false},0,-2},
      {"Lead / Solo","Wide Singing Lead",2,true,{2,3,7,2,5,4},{1,1,1,1,1,2},{true,true,true,true,true,true},0,-4},
      {"FX / Special","Glass Cathedral",0,true,{-3,5,3,9,5,6},{0,0,1,0,0,0},{true,true,true,true,true,true},-1,-4},
      {"FX / Special","Dreamscape Wide",0,true,{1,2,4,7,7,8},{1,1,0,1,0,1},{true,true,true,true,true,true},0,-4},
      {"FX / Special","Short Echo Stereo",0,true,{-2,4,2,-4,8,4},{0,2,1,2,2,2},{true,true,true,true,true,true},0,-3},
      {"FX / Special","Dark Echo Cloud",1,true,{4,-5,4,8,9,3},{2,0,0,0,0,0},{true,true,true,true,true,true},-1,-4},
      {"FX / Special","Pitch Halo",0,true,{-1,3,1,5,-10,9},{0,0,1,1,1,0},{true,true,true,true,false,true},0,-4},
      {"FX / Special","Echo Only DI",3,false,{0,0,0,-10,8,-10},{0,0,0,2,1,2},{false,false,false,false,true,false},0,-3},
      {"FX / Special","Heavy Atmosphere",2,true,{4,2,6,9,8,7},{2,1,0,0,0,1},{true,true,true,true,true,true},-1,-5}
    }; return p;
}
int GuitarProcessor::getNumPrograms(){return (int)presets().size();}
const juce::String GuitarProcessor::getProgramName(int i){return juce::isPositiveAndBelow(i,getNumPrograms())?presets()[(size_t)i].name:"";}
void GuitarProcessor::setCurrentProgram(int i){
    if(!juce::isPositiveAndBelow(i,getNumPrograms()))return;
    program=i; auto& p=presets()[(size_t)i];
    auto set=[this](juce::String id,float v){auto* a=state.getParameter(id);a->beginChangeGesture();a->setValueNotifyingHost(a->convertTo0to1(v));a->endChangeGesture();};
    set("amp",(float)p.amp);set("reamp",p.reamp?1.f:0.f);set("input",p.input);set("output",p.output);
    for(int k=0;k<6;++k){juce::String id=ids[k];set(id,p.faders[k]);set(id+"Color",(float)p.colors[k]);set(id+"On",p.enabled[(size_t)k]?1.f:0.f);}
}
void GuitarProcessor::getStateInformation(juce::MemoryBlock& b){auto tree=state.copyState();tree.setProperty("program",program,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,b);}
void GuitarProcessor::setStateInformation(const void* d,int n){if(auto xml=getXmlFromBinary(d,n)){auto t=juce::ValueTree::fromXml(*xml);if(t.hasType(state.state.getType())){program=juce::jlimit(0,getNumPrograms()-1,(int)t.getProperty("program",0));state.replaceState(t);}}}
juce::AudioProcessorEditor* GuitarProcessor::createEditor(){return new GuitarEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GuitarProcessor();}
