#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include "FactoryBankChecks.h"
#include "AllocationChecks.h"

static void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
static void collect(juce::Component& root,const juce::String& id,std::vector<juce::Component*>& result){
    if(root.getComponentID()==id)result.push_back(&root);
    for(auto* c:root.getChildren())collect(*c,id,result);
}
static juce::TextButton* button(juce::Component& root,const juce::String& label){
    if(auto* b=dynamic_cast<juce::TextButton*>(&root))if(b->getButtonText()==label)return b;
    for(auto* c:root.getChildren())if(auto* b=button(*c,label))return b;
    return nullptr;
}
static void click(juce::Component& c){
    auto pos=c.getLocalBounds().toFloat().getCentre();auto now=juce::Time::getCurrentTime();
    auto event=[&](juce::ModifierKeys mods){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),pos,mods,1,0,0,0,0,&c,&c,now,pos,now,1,false);};
    c.mouseDown(event(juce::ModifierKeys::leftButtonModifier));
    c.mouseUp(event(juce::ModifierKeys{}));
    juce::Timer::callPendingTimersSynchronously();
}
struct HostEvents : juce::AudioProcessorParameter::Listener {
    int values=0,begins=0,ends=0;
    void parameterValueChanged(int,float) override {++values;}
    void parameterGestureChanged(int,bool starting) override {starting?++begins:++ends;}
};
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI init;try{
    testFactoryBank({});
    GuitarProcessor processor;processor.prepareToPlay(48000,256);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    std::vector<juce::Component*> arrows,selectors;
    collect(*editor,"arrow",arrows);collect(*editor,"color-selector",selectors);
    require(arrows.size()==6&&selectors.size()==6,"Six module controls required");
    const char* ids[]={"bass","treble","compress","reverb","delay","pitch"};
    HostEvents host;
    for(auto* p:processor.getParameters())p->addListener(&host);
    for(int preset:{0,24,59}){
        processor.setCurrentProgram(preset);
        for(int k=0;k<6;++k){
            juce::String base=ids[k];auto* on=processor.state.getParameter(base+"On");auto* color=processor.state.getParameter(base+"Color");
            on->setValueNotifyingHost(0);color->setValueNotifyingHost(0);
            // Real mouse events exercise JUCE Button dispatch and host gestures.
            for(int step=0;step<4;++step){
                const auto before=processor.state.copyState();click(*arrows[(size_t)k]);
                require(on->getValue()==(step==3?0.f:1.f),"Arrow On cycle failed");
                require((int)processor.state.getRawParameterValue(base+"Color")->load()==(step==3?0:2-step),"Arrow bottom/middle/top/off order failed");
                for(int j=0;j<6;++j)if(j!=k){
                    for(auto suffix:{"On","Color"}){juce::String key=juce::String(ids[j])+suffix;auto node=before.getChildWithProperty("id",key);require(std::abs((float)node.getProperty("value")-processor.state.getRawParameterValue(key)->load())<1e-5f,"Arrow changed another module");}
                }
            }
            click(*arrows[(size_t)k]);require(on->getValue()==1,"Disabled arrow did not re-enable");
            const float changed=color->getValue();click(*selectors[(size_t)k]);require(color->getValue()!=changed,"Color selector failed");
        }
    }
    for(const char* id:{"bassOn","trebleOn","reverbOn","reamp"}){
        auto* p=processor.state.getParameter(id);p->setValueNotifyingHost(.73f);require(p->getValue()==1,"Boolean host value was not snapped");
    }
    // Manual edits survive subsequent audio blocks and session recall.
    juce::MemoryBlock saved;processor.getStateInformation(saved);GuitarProcessor restored;restored.setStateInformation(saved.getData(),(int)saved.getSize());
    for(auto* parameter:processor.getParameters()){
        auto* p=dynamic_cast<juce::RangedAudioParameter*>(parameter);
        require(p&&p->getValue()==restored.state.getParameter(p->paramID)->getValue(),"Manual state recall failed");
    }
    juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
    auto beforeAudio=processor.state.copyState();
    allocations=0;watch=true;
    for(int n=0;n<30;++n){audio.clear();processor.processBlock(audio,midi);}
    watch=false;require(allocations==0,"Processor allocated during processBlock");
    require(beforeAudio.isEquivalentTo(processor.state.copyState()),"Audio processing overwrote manual edits");
    require(host.values>0&&host.begins==host.ends&&host.begins>0,"Host gesture notification failed");
    for(auto* p:processor.getParameters())p->removeListener(&host);
    // Host automation -> GUI, including the six faders and mode indicators.
    for(int k=0;k<6;++k){auto* p=processor.state.getParameter(juce::String(ids[k])+"On");p->setValueNotifyingHost(0);}
    juce::Thread::sleep(40);juce::Timer::callPendingTimersSynchronously();
    for(auto* c:arrows)require(!dynamic_cast<juce::Button*>(c)->getToggleState(),"Automated disable did not reach GUI");
    std::vector<juce::Component*> pitch;collect(*editor,"pitch",pitch);require(pitch.size()==1,"Pitch slider missing");
    auto* slider=dynamic_cast<juce::Slider*>(pitch.front());require(slider!=nullptr,"Pitch is not a slider");
    require(std::abs(slider->getPositionOfValue(10)-61)<.01f&&std::abs(slider->getPositionOfValue(-10)-291)<.01f,"Fader interaction range does not match its painted travel");
    auto* pitchParameter=processor.state.getParameter("pitch");pitchParameter->setValueNotifyingHost(1);
    require(slider->getValue()==10,"Pitch automation did not reach fader");
    slider->setValue(-10,juce::sendNotificationSync);require(processor.state.getRawParameterValue("pitch")->load()==-10,"Fader did not update DSP parameter");
    auto pos=slider->getLocalBounds().toFloat().getCentre();auto now=juce::Time::getCurrentTime();
    juce::MouseEvent doubleClick(juce::Desktop::getInstance().getMainMouseSource(),pos,{},1,0,0,0,0,slider,slider,now,pos,now,2,false);
    slider->mouseDoubleClick(doubleClick);
    require(slider->getValue()==-6,"Pitch double-click did not restore its actual default");
    // History and comparison still work after mode cycling.
    processor.setCurrentProgram(0);click(*arrows[0]);float edited=processor.state.getRawParameterValue("bassColor")->load();
    auto* undo=button(*editor,"↶");auto* redo=button(*editor,"↷");require(undo&&redo,"History buttons missing");
    undo->onClick();require(processor.state.getRawParameterValue("bassColor")->load()!=edited,"Undo failed");
    redo->onClick();require(processor.state.getRawParameterValue("bassColor")->load()==edited,"Redo failed");
    auto* ab=button(*editor,"A → B");require(ab,"A/B missing");ab->onClick();ab->onClick();require(processor.state.getRawParameterValue("bassColor")->load()==edited,"A/B failed");
    GuitarProcessor defaults;juce::MemoryBlock defaultState;defaults.getStateInformation(defaultState);processor.setStateInformation(defaultState.getData(),(int)defaultState.getSize());
    std::vector<juce::Component*> switches;collect(*editor,"reamp-switch",switches);
    require(switches.size()==1,"Re-Amplify switch missing");
    auto* toggle=dynamic_cast<juce::ToggleButton*>(switches[0]);require(toggle,"Switch is not a ToggleButton");
    click(*toggle);require(processor.state.getRawParameterValue("reamp")->load()==0,"Switch click did not turn off audio parameter");
    click(*toggle);require(processor.state.getRawParameterValue("reamp")->load()==1,"Switch click did not turn on audio parameter");
    processor.inputChannels[0]=.28f;processor.inputChannels[1]=.25f;processor.outputChannels[0]=.4f;processor.outputChannels[1]=.36f;processor.inMeter=.28f;processor.outMeter=.4f;
    for(int wait=0;wait<8;++wait){juce::Thread::sleep(100);juce::Timer::callPendingTimersSynchronously();}
    require(std::abs((float)toggle->getProperties()["switchPosition"]-1.f)<.001f,"ON switch animation did not settle");
    editor->setSize(1680,1080);
    auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
    juce::File file(argc>1?argv[1]:"/tmp/guitar-rack-ui.png");juce::MemoryOutputStream stream;
    require(juce::PNGImageFormat().writeImageToStream(image,stream)&&file.replaceWithData(stream.getData(),stream.getDataSize()),"PNG write failed");
    auto detail=toggle->getParentComponent()->createComponentSnapshot({48,185,72,89},true,6.f);
    juce::MemoryOutputStream detailStream;
    require(juce::PNGImageFormat().writeImageToStream(detail,detailStream)&&file.getSiblingFile("switch-detail.png").replaceWithData(detailStream.getData(),detailStream.getDataSize()),"Switch detail PNG write failed");
    for(const char* id:ids)processor.state.getParameter(juce::String(id)+"On")->setValueNotifyingHost(0);
    processor.state.getParameter("reamp")->setValueNotifyingHost(0);
    for(int wait=0;wait<8;++wait){juce::Thread::sleep(100);juce::Timer::callPendingTimersSynchronously();}
    require(std::abs((float)toggle->getProperties()["switchPosition"])<.001f,"OFF switch animation did not settle");
    auto offImage=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
    juce::MemoryOutputStream offStream;
    require(juce::PNGImageFormat().writeImageToStream(offImage,offStream)&&file.getSiblingFile(file.getFileNameWithoutExtension()+"-off.png").replaceWithData(offStream.getData(),offStream.getDataSize()),"Off PNG write failed");
    for(auto* c:arrows)require(!dynamic_cast<juce::Button*>(c)->getToggleState(),"Disabled arrow remains lit");
    editor.reset();
    for(int i=0;i<10;++i){std::unique_ptr<juce::AudioProcessorEditor> reopened(processor.createEditor());reopened->setSize(840,540);}
    // End-to-end: each real arrow changes its own audio stage, not only APVTS/UI.
    editor.reset(processor.createEditor());arrows.clear();collect(*editor,"arrow",arrows);
    for(int stage=0;stage<6;++stage){
        GuitarProcessor dry;
        for(auto* rack:{&dry,&processor}){
            auto set=[&](juce::String id,float v){auto* p=rack->state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(v));};
            set("input",0);set("output",0);set("reamp",0);
            for(int k=0;k<6;++k){set(juce::String(ids[k])+"On",0);set(juce::String(ids[k])+"Color",2);set(ids[k],k==stage?8.f:0.f);}
            rack->prepareToPlay(48000,256);
        }
        click(*arrows[(size_t)stage]);
        juce::AudioBuffer<float> reference(2,256),effect(2,256);double difference=0;
        for(int block=0;block<250;++block){
            for(int n=0;n<256;++n){double t=(block*256+n)/48000.;float x=.05f*float(std::sin(t*2*juce::MathConstants<double>::pi*60)+std::sin(t*2*juce::MathConstants<double>::pi*440)+std::sin(t*2*juce::MathConstants<double>::pi*4000));for(int c=0;c<2;++c){reference.setSample(c,n,x);effect.setSample(c,n,x);}}
            dry.processBlock(reference,midi);processor.processBlock(effect,midi);
            if(block>80)for(int n=0;n<256;++n){double d=effect.getSample(0,n)-reference.getSample(0,n);difference+=d*d;}
        }
        require(difference>.001,"Manual module selection did not alter the corresponding audio stage");
    }
    editor.reset();
    // Validate mono -> stereo processing across sample rates and non-power-of-two buffers.
    for(double rate:{44100.,48000.,96000.})for(int block:{17,64,257,1024}){
        GuitarProcessor mono;auto layout=mono.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());require(mono.setBusesLayout(layout),"Mono input layout failed");mono.prepareToPlay(rate,block);require(mono.getLatencySamples()==mono.engine.latencySamples(),"Host latency mismatch");
        juce::AudioBuffer<float> buffer(2,block);double energy=0;
        for(int offset=0;offset<mono.getLatencySamples()+2048;offset+=block){
            buffer.clear();if(offset==0)buffer.setSample(0,0,.2f);mono.processBlock(buffer,midi);
            for(int c=0;c<2;++c)for(int n=0;n<block;++n){const float x=buffer.getSample(c,n);require(std::isfinite(x),"Mono processing unstable");energy+=x*x;}
        }
        require(energy>1e-8,"Mono processing produced no audio after latency");
        mono.prepareToPlay(rate,block);int sample=0;allocations=0;watch=true;
        for(int offset=0;offset<mono.getLatencySamples()+2048;offset+=block){
            buffer.clear();if(offset==0)buffer.setSample(0,0,.2f);mono.processBlockBypassed(buffer,midi);
            for(int n=0;n<block;++n,++sample)for(int c=0;c<2;++c)require(std::abs(buffer.getSample(c,n)-(sample==mono.getLatencySamples()?.2f:0.f))<1e-7,"Host bypass latency/mono routing incorrect");
        }
        watch=false;require(allocations==0,"Bypass processing allocated");
    }
    std::cout<<"PASS: six independent mode/off cycles after presets, host gestures, GUI automation, manual state recall, no preset overwrite, Pitch fader/default, Undo/Redo/A-B, resize/render, editor lifecycle and mono processing\n";return 0;
}catch(const std::exception& e){watch=false;std::cerr<<e.what()<<'\n';return 1;}}
