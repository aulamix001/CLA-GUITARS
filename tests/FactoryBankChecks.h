#pragma once
#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <set>

struct TestTempo : juce::AudioPlayHead {
    double bpm=90;
    juce::Optional<PositionInfo> getPosition() const override {
        PositionInfo position;position.setBpm(bpm);return position;
    }
};
static void testHostTempo() {
    GuitarProcessor processor;
    TestTempo tempo;processor.setPlayHead(&tempo);
    for(auto key:{"bassOn","trebleOn","compressOn","reverbOn","pitchOn"})
        processor.state.getParameter(key)->setValueNotifyingHost(0);
    processor.state.getParameter("delayColor")->setValueNotifyingHost(1); // Eight
    for(double bpm:{90.,180.}) {
        tempo.bpm=bpm;processor.prepareToPlay(48000,256);
        int onset=processor.getLatencySamples()+(int)std::floor(24000*60/bpm);
        juce::AudioBuffer<float> audio(2,onset+8);audio.clear();audio.setSample(0,0,.5f);audio.setSample(1,0,.5f);
        juce::MidiBuffer midi;processor.processBlock(audio,midi);
        double energy=0;for(int n=onset;n<onset+3;++n)energy+=std::abs(audio.getSample(0,n));
        if(energy<.001)throw std::runtime_error("Host BPM did not reach delay processing");
        for(int n=processor.getLatencySamples()+1;n<onset-1;++n)
            if(std::abs(audio.getSample(0,n))>1e-6f)throw std::runtime_error("Host-synced delay arrived too early");
    }
    processor.setPlayHead(nullptr);
}
static void testFactoryBank(const juce::File& exportDirectory) {
    testHostTempo();
    GuitarProcessor processor, restored;
    if(processor.getNumPrograms()!=60)throw std::runtime_error("Factory bank must contain exactly 60 programs");
    // Manual p.8-13 and supplied Parameter_List: neutral startup, .1 steps,
    // green first colors, independent On switches, stable legacy parameter IDs.
    for(auto key:{"input","output","bass","treble","compress","reverb","delay","pitch","balance"}) {
        auto* parameter=processor.state.getParameter(key);
        if(!parameter||std::abs(parameter->convertFrom0to1(parameter->getDefaultValue()))>1e-5f)
            throw std::runtime_error("Startup faders must be neutral");
        if(std::abs(parameter->getNormalisableRange().interval-.1f)>1e-5f)
            throw std::runtime_error("Fader resolution must be .1");
    }
    if(processor.state.getRawParameterValue("amp")->load()!=0||processor.state.getRawParameterValue("reamp")->load()!=0)
        throw std::runtime_error("Startup must be Clean with Re-Amplify off");
    for(auto key:{"bassColor","trebleColor","compressColor","reverbColor","delayColor","pitchColor"})
        if(processor.state.getRawParameterValue(key)->load()!=2)
            throw std::runtime_error("Startup colors must be green");
    auto* balance=processor.state.getParameter("balance");
    if(balance->getNormalisableRange().start!=-6||balance->getNormalisableRange().end!=6)
        throw std::runtime_error("Balance range must be +/-6");
    balance->setValueNotifyingHost(balance->convertTo0to1(4.2f));
    juce::MemoryBlock balanced;processor.getStateInformation(balanced);restored.setStateInformation(balanced.getData(),(int)balanced.getSize());
    if(std::abs(restored.state.getRawParameterValue("balance")->load()-4.2f)>1e-5f)
        throw std::runtime_error("Balance must survive session recall");
    balance->setValueNotifyingHost(balance->convertTo0to1(0));
    std::set<juce::String> names;
    std::map<juce::String,int> folders;
    juce::String csv="Category,Name,Amp,ReAmplify,Input,Output,Bass,Treble,Compress,Reverb,Delay,Pitch,BassColor,TrebleColor,CompressColor,ReverbColor,DelayColor,PitchColor,BassOn,TrebleOn,CompressOn,ReverbOn,DelayOn,PitchOn\n";
    const char* keys[]={"input","output","bass","treble","compress","reverb","delay","pitch","bassColor","trebleColor","compressColor","reverbColor","delayColor","pitchColor","bassOn","trebleOn","compressOn","reverbOn","delayOn","pitchOn"};
    const char* amps[]={"Clean","Crunch","Heavy","Bypass"};
    const char* colorNames[6][3]={{"Upper","Lower","Sub"},{"Roof","Top","Bite"},{"Wall","Spank","Push"},{"Arena","Hall","Club"},{"Quarter","Edge","Eight"},{"Spreader","Wide","Stereo"}};
    if(exportDirectory!=juce::File{})exportDirectory.createDirectory();
    for(int i=0;i<processor.getNumPrograms();++i) {
        const auto& preset=GuitarProcessor::presets()[(size_t)i];
        if(!names.insert(preset.name).second)throw std::runtime_error("Duplicate preset name");
        ++folders[preset.folder];
        processor.setCurrentProgram(i);
        auto value=[&](const juce::String& id){return processor.state.getRawParameterValue(id)->load();};
        if(processor.getCurrentProgram()!=i||value("amp")!=preset.amp||value("reamp")!=(preset.reamp?1.f:0.f))throw std::runtime_error("Amp mapping failed");
        for(int k=0;k<20;++k) {
            const float v=value(keys[k]);
            if(!std::isfinite(v)||(k<8&&(v < -10 || v > 10))||(k>=8&&k<14&&(v<0||v>2))||(k>=14&&(v<0||v>1)))throw std::runtime_error("Preset value out of range");
        }
        const char* stages[]={"bass","treble","compress","reverb","delay","pitch"};
        for(int k=0;k<6;++k) {
            if(std::abs(value(stages[k])-preset.faders[(size_t)k])>1e-5f || value(juce::String(stages[k])+"Color")!=preset.colors[(size_t)k] || value(juce::String(stages[k])+"On")!=(preset.enabled[(size_t)k]?1.f:0.f))throw std::runtime_error("Visible effect controls did not follow preset");
        }
        if(std::abs(value("input")-preset.input)>1e-5f||std::abs(value("output")-preset.output)>1e-5f)throw std::runtime_error("Gain mapping failed");
        juce::MemoryBlock data;processor.getStateInformation(data);restored.setStateInformation(data.getData(),(int)data.getSize());
        if(restored.getCurrentProgram()!=i)throw std::runtime_error("Saved program index did not restore");
        for(auto* parameter:processor.getParameters()) {
            auto* p=dynamic_cast<juce::RangedAudioParameter*>(parameter);
            if(p&&std::abs(p->getValue()-restored.state.getParameter(p->paramID)->getValue())>1e-5f)throw std::runtime_error("Saved parameter did not restore");
        }
        processor.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
        for(int block=0;block<120;++block) {
            for(int n=0;n<256;++n){int sample=block*256+n;float env=std::exp(-sample/24000.f);audio.setSample(0,n,.18f*env*(std::sin(sample*.029f)+.3f*std::sin(sample*.107f)));audio.setSample(1,n,.16f*env*(std::sin(sample*.031f)+.3f*std::sin(sample*.109f)));}
            processor.processBlock(audio,midi);
            for(int c=0;c<2;++c)for(int n=0;n<256;++n)if(!std::isfinite(audio.getSample(c,n))||std::abs(audio.getSample(c,n))>50)throw std::runtime_error("Preset audio is not stable");
        }
        csv+=juce::String(preset.folder)+","+preset.name+","+amps[preset.amp]+","+(preset.reamp?"On":"Off");
        for(int k=0;k<20;++k){csv+=",";if(k>=8&&k<14)csv+=colorNames[k-8][preset.colors[(size_t)(k-8)]];else csv+=juce::String(value(keys[k]),2);}
        csv+="\n";
        if(exportDirectory!=juce::File{}) {
            auto folder=exportDirectory.getChildFile(juce::String(preset.folder).replace(" / ","-"));folder.createDirectory();
            auto xml=processor.state.copyState().createXml();
            if(!xml->writeTo(folder.getChildFile(juce::String(preset.name)+".xml")))throw std::runtime_error("Preset export failed");
        }
    }
    if(folders.size()!=6)throw std::runtime_error("Expected six preset categories");
    for(const auto& folder:folders)if(folder.second!=10)throw std::runtime_error("Each category must contain 10 presets");
    if(exportDirectory!=juce::File{}&&!exportDirectory.getSiblingFile("presets.csv").replaceWithText(csv))throw std::runtime_error("Preset manifest export failed");
    std::cout<<"PASS: 60 presets, 6 categories x 10, control mapping, session recall and stereo audio processing\n";
}
