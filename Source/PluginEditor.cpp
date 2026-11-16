#include "PluginEditor.h"
namespace {const char* ids[]={"input","bass","treble","compress","reverb","delay","pitch","output"};const char* labels[]={"INPUT SENS","BASS","TREBLE","COMPRESS","REVERB","DELAY","PITCH","OUTPUT"};const char* names[6][3]={{"UPPER","LOWER","SUB"},{"ROOF","TOP","BITE"},{"WALL","SPANK","PUSH"},{"ARENA","HALL","CLUB"},{"QUARTER","EIGHT","SLAP"},{"SPREADER","WIDE","STEREO"}};}
void RackLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle,juce::Slider&) {
 float cx=(float)x+w*0.5f;
 g.setColour(juce::Colour(0xff090a0b));g.fillRoundedRectangle(cx-4,(float)y,8,(float)h,4);
 g.setColour(juce::Colours::white.withAlpha(0.13f));g.drawLine(cx+5,(float)y,cx+5,(float)y+h);
 juce::Rectangle<float> r(cx-20,pos-32,40,64);
 g.setColour(juce::Colours::black.withAlpha(0.6f));g.fillRoundedRectangle(r.translated(6,7),5);
 juce::ColourGradient grad(juce::Colour(0xffd9dfe1),0,r.getY(),juce::Colour(0xff555b60),0,r.getBottom(),false);
 grad.addColour(0.17,juce::Colour(0xff747c81));grad.addColour(0.76,juce::Colour(0xfff2f5f6));g.setGradientFill(grad);g.fillRoundedRectangle(r,4);
 g.setColour(juce::Colour(0xff111315));g.drawRoundedRectangle(r,4,1.5f);g.drawLine(cx-13,pos,cx+13,pos,2);
}
GuitarEditor::GuitarEditor(GuitarProcessor& p):AudioProcessorEditor(p),processor(p) {
 setLookAndFeel(&look);setSize(1120,720);setResizable(true,true);setResizeLimits(900,580,1680,1080);
 texture=juce::Image(juce::Image::RGB,1120,720,true);juce::Random rng(2517);
 for(int y=0;y<720;++y)for(int x=0;x<1120;++x){auto v=(juce::uint8)(40+rng.nextInt(12));texture.setPixelAt(x,y,juce::Colour(v,v,v));}
 for(int i=0;i<8;++i){auto& s=sliders[i];s.setSliderStyle(juce::Slider::LinearVertical);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,20);s.setNumDecimalPlacesToDisplay(1);s.setTextValueSuffix(" dB");s.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);addAndMakeVisible(s);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s));}
 for(int k=0;k<6;++k){addAndMakeVisible(colors[k]);colors[k].onClick=[this,k]{auto* a=processor.state.getParameter(juce::String(ids[k+1])+"Color");int next=((int)processor.state.getRawParameterValue(juce::String(ids[k+1])+"Color")->load()+1)%3;a->beginChangeGesture();a->setValueNotifyingHost(a->convertTo0to1((float)next));a->endChangeGesture();};addAndMakeVisible(enabled[k]);enabled[k].setButtonText("ON");buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,juce::String(ids[k+1])+"On",enabled[k]));}
 addAndMakeVisible(reamp);buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"reamp",reamp));
 addAndMakeVisible(amp);amp.addItemList({"Clean","Crunch","Heavy","Bypass"},1);ampAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"amp",amp);
 addAndMakeVisible(load);load.onClick=[this]{presetMenu();};addAndMakeVisible(save);
 save.onClick=[this] { savePreset(); };
 startTimerHz(30);
}
void GuitarEditor::savePreset() {
 chooser=std::make_unique<juce::FileChooser>("Save preset",juce::File{},"*.xml");
 const juce::Component::SafePointer<GuitarEditor> safe(this);
 chooser->launchAsync(
     juce::FileBrowserComponent::saveMode
         | juce::FileBrowserComponent::canSelectFiles
         | juce::FileBrowserComponent::warnAboutOverwriting,
     [safe](const juce::FileChooser& fileChooser) {
         if (safe == nullptr || fileChooser.getResult() == juce::File{})
             return;
         auto xml = safe->processor.state.copyState().createXml();
         xml->writeTo(fileChooser.getResult().withFileExtension("xml"));
     });
}
void GuitarEditor::presetMenu(){
 juce::PopupMenu menu;auto& presets=GuitarProcessor::presets();
 for(auto folder:{"Acoustic","Clean","Crunch","Heavy","Lead / Solo","FX / Special"}){juce::PopupMenu sub;for(size_t i=0;i<presets.size();++i)if(juce::String(folder)==presets[i].folder)sub.addItem((int)i+1,presets[i].name);menu.addSubMenu(folder,sub);}
 menu.addSeparator();menu.addItem(1000,"Load user preset…");
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(load),[safe=juce::Component::SafePointer<GuitarEditor>(this)](int id){if(!safe)return;if(id>0&&id<1000)safe->processor.setCurrentProgram(id-1);if(id==1000){safe->chooser=std::make_unique<juce::FileChooser>("Load preset",juce::File{},"*.xml");safe->chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& c){if(!safe)return;if(auto xml=juce::XmlDocument::parse(c.getResult())){auto tree=juce::ValueTree::fromXml(*xml);if(tree.hasType(safe->processor.state.state.getType()))safe->processor.state.replaceState(tree);}});}});
}
void GuitarEditor::timerCallback(){meterIn=juce::jmax(processor.inMeter.load(),meterIn*0.87f);meterOut=juce::jmax(processor.outMeter.load(),meterOut*0.87f);for(int k=0;k<6;++k){int c=juce::jlimit(0,2,(int)processor.state.getRawParameterValue(juce::String(ids[k+1])+"Color")->load());colors[k].setButtonText(juce::String(names[k][c])+" ↑");colors[k].setColour(juce::TextButton::buttonColourId,juce::Colour(c==0?0xff9c3330:c==1?0xff247b9c:0xff629b28));}repaint();}
void GuitarEditor::resized(){
 float sx=getWidth()/1120.f,sy=getHeight()/720.f;auto place=[&](juce::Component& c,int x,int y,int w,int h){c.setBounds(juce::roundToInt(x*sx),juce::roundToInt(y*sy),juce::roundToInt(w*sx),juce::roundToInt(h*sy));};
 place(load,945,4,72,24);place(save,1025,4,72,24);place(reamp,48,180,185,28);place(amp,95,220,145,28);
 for(int i=0;i<8;++i){int x=i==0?76:i==7?980:300+(i-1)*92;place(sliders[i],x,320,65,345);}
 for(int k=0;k<6;++k){place(colors[k],302+k*92,250,65,33);place(enabled[k],306+k*92,286,65,24);}
}
void GuitarEditor::paint(juce::Graphics& g){
 g.addTransform(juce::AffineTransform::scale(getWidth()/1120.f,getHeight()/720.f));g.drawImage(texture,juce::Rectangle<float>(0,0,1120,720));
 g.setColour(juce::Colour(0xff0c0e10));g.drawRoundedRectangle(14,31,1092,677,22,16);
 juce::ColourGradient metal(juce::Colour(0xffd2d5d6),15,0,juce::Colour(0xff3d4246),43,0,false);g.setGradientFill(metal);g.fillRoundedRectangle(18,43,18,645,8);g.fillRoundedRectangle(1084,43,18,645,8);
 g.setColour(juce::Colour(0xff744b2e));g.fillRect(42,43,1036,117);
 for(int y=44;y<160;y+=4)for(int x=43;x<1076;x+=8){g.setColour(((x/8+y/4)%2)?juce::Colour(0xffb18854):juce::Colour(0xff33271e));g.drawLine((float)x,(float)y,(float)x+6,(float)y+4,2);}
 g.setColour(juce::Colour(0xff111315));g.fillRect(0,0,1120,32);g.setColour(juce::Colours::silver);g.setFont(16);g.drawText("GUITAR RACK  •  INDEPENDENT AUDIO",12,2,410,28,juce::Justification::centredLeft);g.drawText(processor.getProgramName(processor.getCurrentProgram()),430,2,500,28,juce::Justification::centred);
 g.setFont(juce::FontOptions(35,juce::Font::bold|juce::Font::italic));g.setColour(juce::Colours::black.withAlpha(.7f));g.drawText("Guitar Rack",86,93,450,53,juce::Justification::left);g.setColour(juce::Colours::silver);g.drawText("Guitar Rack",83,90,450,53,juce::Justification::left);
 g.setColour(juce::Colours::black);g.drawLine(280,161,280,695,3);g.drawLine(846,161,846,695,3);g.drawLine(40,275,279,275,3);g.drawLine(847,275,1078,275,3);
 g.setColour(juce::Colour(0xff16191b));g.fillRect(294,172,544,68);g.setColour(juce::Colours::grey);g.drawRect(294,172,544,68);
 for(int k=0;k<6;++k){int active=(int)processor.state.getRawParameterValue(juce::String(ids[k+1])+"Color")->load();for(int r=0;r<3;++r){g.setFont(juce::FontOptions(12,juce::Font::bold|juce::Font::italic));g.setColour(r==active?(r==0?juce::Colours::red:r==1?juce::Colours::deepskyblue:juce::Colours::chartreuse):juce::Colour(0xff515558));g.drawText(names[k][r],298+k*92,176+r*19,84,18,juce::Justification::centred);}}
 g.setColour(juce::Colours::black.withAlpha(.6f));g.setFont(juce::FontOptions(34,juce::Font::bold));g.drawText("Guitars",870,189,180,54,juce::Justification::centred);
 for(int i=0;i<8;++i){int x=i==0?76:i==7?980:300+(i-1)*92;g.setColour(juce::Colours::silver);g.setFont(13);g.drawText(labels[i],x-15,672,100,20,juce::Justification::centred);g.setFont(11);for(int j=0;j<3;++j)g.drawText(j==0?"+10":j==1?"0":"−10",x-25,352+j*130,30,15,juce::Justification::centred);}
 auto meter=[&](int x,float value){g.setColour(juce::Colours::black);g.fillRect(x-4,330,48,282);float db=juce::Decibels::gainToDecibels(value,-60.f);for(int j=0;j<32;++j){bool on=db> -48+j*1.5f;g.setColour(on?(j>29?juce::Colours::red:j>25?juce::Colours::yellow:juce::Colours::chartreuse):juce::Colour(0xff16351d));g.fillRect(x,600-j*8,16,6);g.fillRect(x+21,600-j*8,16,6);}};meter(186,meterIn);meter(895,meterOut);
 float db=juce::Decibels::gainToDecibels(meterIn,-60.f);g.setColour(db>0?juce::Colours::red:db>-12?juce::Colours::yellow:juce::Colours::green);g.fillRoundedRectangle(99,294,19,12,3);
}
