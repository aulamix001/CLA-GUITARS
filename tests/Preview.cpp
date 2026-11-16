#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
#include "FactoryBankChecks.h"

static juce::TextButton* button(juce::Component& root,const juce::String& label) {
    if(auto* b=dynamic_cast<juce::TextButton*>(&root))if(b->getButtonText()==label)return b;
    for(auto* child:root.getChildren())if(auto* b=button(*child,label))return b;
    return nullptr;
}
static juce::TextButton* arrow(juce::Component& root) {
    if(auto* b=dynamic_cast<juce::TextButton*>(&root))if(b->getComponentID()=="arrow")return b;
    for(auto* child:root.getChildren())if(auto* b=arrow(*child))return b;
    return nullptr;
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    try {
        testFactoryBank(argc>2?juce::File(argv[2]):juce::File{});
        GuitarProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto initial=processor.state.copyState();
        auto* next=arrow(*editor);if(!next)throw std::runtime_error("Color button missing");
        next->onClick();
        if(processor.state.getRawParameterValue("bassColor")->load()!=2)throw std::runtime_error("Color did not cycle");
        auto* undo=button(*editor,"↶");auto* redo=button(*editor,"↷");
        if(!undo||!redo)throw std::runtime_error("History buttons missing");
        undo->onClick();if(processor.state.getRawParameterValue("bassColor")->load()!=1)throw std::runtime_error("Undo failed");
        redo->onClick();if(processor.state.getRawParameterValue("bassColor")->load()!=2)throw std::runtime_error("Redo failed");
        auto* ab=button(*editor,"A → B");if(!ab)throw std::runtime_error("A/B missing");
        ab->onClick();if(processor.state.getRawParameterValue("bassColor")->load()!=1)throw std::runtime_error("B initial state failed");
        ab->onClick();if(processor.state.getRawParameterValue("bassColor")->load()!=2)throw std::runtime_error("A state was not retained");
        undo->onClick();
        // Render the actual editor with test meter levels, not a separate mock-up.
        processor.inMeter.store(.28f);processor.outMeter.store(.4f);
        processor.inputChannels[0].store(.28f);processor.inputChannels[1].store(.25f);
        processor.outputChannels[0].store(.4f);processor.outputChannels[1].store(.36f);
        juce::Thread::sleep(70);
        juce::Timer::callPendingTimersSynchronously();
        auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
        juce::File file(argc>1?argv[1]:"/tmp/guitar-rack-ui.png");
        juce::MemoryOutputStream stream;
        if(!juce::PNGImageFormat().writeImageToStream(image,stream) || !file.replaceWithData(stream.getData(),stream.getDataSize()))throw std::runtime_error("PNG write failed");
        std::cout<<"PASS: Color, Undo, Redo, A/B state restoration; editor PNG rendered at "<<image.getWidth()<<"x"<<image.getHeight()<<"\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
