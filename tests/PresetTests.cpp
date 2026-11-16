#include "FactoryBankChecks.h"
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    try {
        testFactoryBank(argc>1?juce::File(argv[1]):juce::File{});
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
        return 1;
    }
}
