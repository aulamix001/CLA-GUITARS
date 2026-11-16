#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace { const char* ids[]={"bass","treble","compress","reverb","delay","pitch"}; }
juce::AudioProcessorValueTreeState::ParameterLayout GuitarProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    for(auto id:{"input","output"}) l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},id,-10.f,10.f,0.f));
    const juce::StringArray colors[]={{"Upper","Lower","Sub"},{"Roof","Top","Bite"},{"Wall","Spank","Push"},{"Arena","Hall","Club"},{"Quarter","Eight","Slap"},{"Spreader","Wide","Stereo"}};
    for(int i=0;i<6;++i) {
        juce::String id=ids[i];
        l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},id,-10.f,10.f,0.f));
        l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"Color",1},id+" Color",colors[i],0));
        l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id+"On",1},id+" On",true));
    }
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"amp",1},"Amp",juce::StringArray{"Clean","Crunch","Heavy","Bypass"},0));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reamp",1},"Re-Amplify",true));
    return l;
}
GuitarProcessor::GuitarProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"GuitarRack",layout()) {}
bool GuitarProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() && (l.getMainInputChannelSet()==juce::AudioChannelSet::mono() || l.getMainInputChannelSet()==juce::AudioChannelSet::stereo());
}
void GuitarProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
    juce::ScopedNoDenormals nd;
    if(getTotalNumInputChannels()==1) b.copyFrom(1,0,b,0,0,b.getNumSamples());
    auto value=[this](juce::String id){return state.getRawParameterValue(id)->load();};
    guitar::Settings s; s.input=value("input");s.output=value("output");s.reamp=value("reamp")>0.5f;s.amp=(int)value("amp");
    float* f[]={&s.bass,&s.treble,&s.compress,&s.reverb,&s.delay,&s.pitch};
    for(int i=0;i<6;++i){*f[i]=value(ids[i]);s.color[i]=(int)value(juce::String(ids[i])+"Color");s.enabled[i]=value(juce::String(ids[i])+"On")>0.5f;}
    engine.set(s);engine.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples());
    inMeter.store(engine.inputPeak);outMeter.store(engine.outputPeak);
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
      {"FX / Special","Slap Texture",0,true,{-1,2,1,2,7,4},{0,2,2,2,2,2}}
    }; return p;
}
int GuitarProcessor::getNumPrograms(){return (int)presets().size();}
const juce::String GuitarProcessor::getProgramName(int i){return juce::isPositiveAndBelow(i,getNumPrograms())?presets()[(size_t)i].name:"";}
void GuitarProcessor::setCurrentProgram(int i){
    if(!juce::isPositiveAndBelow(i,getNumPrograms()))return;
    program=i; auto& p=presets()[(size_t)i];
    auto set=[this](juce::String id,float v){auto* a=state.getParameter(id);a->beginChangeGesture();a->setValueNotifyingHost(a->convertTo0to1(v));a->endChangeGesture();};
    set("amp",(float)p.amp);set("reamp",p.reamp?1.f:0.f);set("input",0);set("output",0);
    for(int k=0;k<6;++k){juce::String id=ids[k];set(id,p.faders[k]);set(id+"Color",(float)p.colors[k]);set(id+"On",1);}
}
void GuitarProcessor::getStateInformation(juce::MemoryBlock& b){auto tree=state.copyState();tree.setProperty("program",program,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,b);}
void GuitarProcessor::setStateInformation(const void* d,int n){if(auto xml=getXmlFromBinary(d,n)){auto t=juce::ValueTree::fromXml(*xml);if(t.hasType(state.state.getType())){program=juce::jlimit(0,getNumPrograms()-1,(int)t.getProperty("program",0));state.replaceState(t);}}}
juce::AudioProcessorEditor* GuitarProcessor::createEditor(){return new GuitarEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GuitarProcessor();}
