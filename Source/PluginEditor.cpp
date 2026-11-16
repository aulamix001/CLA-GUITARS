#include "PluginEditor.h"

namespace {
constexpr int width = 1120, height = 720;
const char* ids[] = {"input", "bass", "treble", "compress", "reverb", "delay", "pitch", "output"};
const char* labels[] = {"INPUT SENS", "BASS", "TREBLE", "COMPRESS", "REVERB", "DELAY", "PITCH", "OUTPUT"};
const char* names[6][3] = {{"UPPER", "LOWER", "SUB"}, {"ROOF", "TOP", "BITE"},
    {"WALL", "SPANK", "PUSH"}, {"ARENA", "HALL", "CLUB"}, {"QUARTER", "EDGE", "EIGHT"},
    {"SPREADER", "WIDE", "STEREO"}};
constexpr int centers[] = {110, 334, 426, 518, 610, 702, 794, 1014};
juce::Colour ink(0xffbabbb6), dark(0xff111315);
juce::Colour colorFor(int index) {
    return juce::Colour(index == 0 ? 0xffed3130 : index == 1 ? 0xff339cc4 : 0xff83c948);
}
void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size,
          juce::Colour c, int style = juce::Font::plain, int align = juce::Justification::centred) {
    g.setColour(c); g.setFont(juce::FontOptions("Arial", size, style));
    g.drawText(s, r, align, false);
}
void screw(juce::Graphics& g, float x, float y, float r = 7) {
    g.setColour(juce::Colours::black.withAlpha(.8f)); g.fillEllipse(x-r-1,y-r+1,2*r+3,2*r+3);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff90938f),x,y-r,dark,x,y+r,false));
    g.fillEllipse(x-r,y-r,2*r,2*r);
    g.setColour(juce::Colour(0xff070909)); g.fillEllipse(x-r+2,y-r+2,2*r-4,2*r-4);
    g.setColour(juce::Colour(0xff6f7472)); g.drawLine(x-2,y+2,x+2,y-2,1.2f);
}
void led(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c) {
    g.setColour(dark); g.fillRoundedRectangle(r.expanded(4),5);
    g.setColour(juce::Colour(0xff74766f)); g.drawRoundedRectangle(r.expanded(3),4,1);
    g.setGradientFill(juce::ColourGradient(c.brighter(.4f),r.getX(),r.getY(),c.darker(.5f),r.getRight(),r.getBottom(),false));
    g.fillRoundedRectangle(r,3);
    g.setColour(juce::Colours::white.withAlpha(.35f)); g.fillRoundedRectangle(r.reduced(3,2).withHeight(3),1);
}
}

juce::Slider::SliderLayout RackLook::getSliderLayout(juce::Slider& slider) {
    if (slider.getSliderStyle() == juce::Slider::LinearVertical) {
        juce::Slider::SliderLayout layout;
        layout.sliderBounds = {0, 61, slider.getWidth(), 230};
        return layout;
    }
    return juce::LookAndFeel_V4::getSliderLayout(slider);
}
void RackLook::drawLinearSlider(juce::Graphics& g, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider& s) {
    const float cx = s.getWidth()*.5f;
    const float y = 61.f + (10.f-static_cast<float>(s.getValue()))*11.5f;
    g.setColour(juce::Colour(0xff777b75).withAlpha(.34f));
    g.fillRoundedRectangle(cx-16,0,32,343,13);
    g.setColour(juce::Colours::black); g.fillRoundedRectangle(cx-3.7f,35,7.4f,288,4);
    g.setColour(juce::Colours::white.withAlpha(.15f)); g.drawLine(cx+4.3f,37,cx+4.3f,320,1);
    screw(g,cx,16,7); screw(g,cx,339,6);
    juce::Path cap;
    cap.startNewSubPath(cx-17,y-40);cap.lineTo(cx+16,y-40);
    cap.quadraticTo(cx+23,y-29,cx+23,y-15);cap.quadraticTo(cx+23,y+9,cx+18,y+39);
    cap.lineTo(cx-18,y+39);cap.quadraticTo(cx-25,y+16,cx-24,y-8);cap.quadraticTo(cx-23,y-29,cx-17,y-40);cap.closeSubPath();
    juce::Path side;side.startNewSubPath(cx+17,y-39);side.lineTo(cx+28,y-31);side.lineTo(cx+30,y+29);side.lineTo(cx+19,y+41);side.closeSubPath();
    juce::DropShadow(juce::Colours::black.withAlpha(.85f),7,{7,7}).drawForPath(g,cap);
    g.setColour(juce::Colour(0xff141918));g.fillPath(side);
    juce::ColourGradient metal(juce::Colour(0xff262c2f),cx,y-40,juce::Colour(0xff424b4f),cx,y+39,false);
    metal.addColour(.055,juce::Colour(0xffc5cacc));metal.addColour(.12,juce::Colour(0xff52595e));
    metal.addColour(.36,juce::Colour(0xff6f777c));metal.addColour(.61,juce::Colour(0xffbfc8ce));
    metal.addColour(.82,juce::Colour(0xffe7ecee));metal.addColour(.94,juce::Colour(0xffccd3d5));
    g.setGradientFill(metal);g.fillPath(cap);
    g.saveState();g.reduceClipRegion(cap);
    for(int line=0;line<85;++line){float yy=y-40+line*.94f;g.setColour((line%3==0?juce::Colours::white:juce::Colours::black).withAlpha(line%3==0?.008f:.004f));g.drawHorizontalLine((int)yy,cx-23,cx+23);}
    g.restoreState();g.setColour(juce::Colour(0xff111718));g.strokePath(cap,juce::PathStrokeType(1.3f));
    g.setColour(juce::Colour(0xffdce2e2).withAlpha(.8f));g.drawLine(cx-15,y-38,cx+14,y-38,1.1f);
    g.setColour(juce::Colour(0xff3e484b));g.drawLine(cx-14,y,cx+14,y,1.6f);
    g.setColour(juce::Colours::white.withAlpha(.5f));g.drawLine(cx-14,y+1.6f,cx+14,y+1.6f,.8f);
}
void RackLook::drawRotarySlider(juce::Graphics& g,int,int,int,int,float position,float,float,juce::Slider& s) {
    const auto c=s.getLocalBounds().toFloat().getCentre();const float radius=30;
    juce::Path gear;
    for(int i=0;i<64;++i){float angle=i*juce::MathConstants<float>::twoPi/64;float r=radius+((i%4==1||i%4==2)?1.3f:0.f);auto p=c+juce::Point<float>(std::cos(angle)*r,std::sin(angle)*r);if(i==0)gear.startNewSubPath(p);else gear.lineTo(p);}
    gear.closeSubPath();juce::DropShadow(juce::Colours::black.withAlpha(.9f),5,{3,4}).drawForPath(g,gear);
    juce::ColourGradient chrome(juce::Colour(0xffbdc1b8),c.x-20,c.y-30,juce::Colour(0xff323a36),c.x+22,c.y+31,false);
    chrome.addColour(.22,juce::Colour(0xff555f58));chrome.addColour(.48,juce::Colour(0xffb5bbb0));chrome.addColour(.72,juce::Colour(0xff404b42));
    g.setGradientFill(chrome);g.fillPath(gear);g.setColour(juce::Colour(0xff131b16));g.strokePath(gear,juce::PathStrokeType(1.1f));
    g.setColour(juce::Colour(0xff121814));g.fillEllipse(c.x-28,c.y-28,56,56);
    g.setColour(juce::Colour(0xffb9c0b4));g.drawEllipse(c.x-27,c.y-27,54,54,1.9f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff303731),c.x-18,c.y-23,juce::Colour(0xff141b16),c.x+20,c.y+26,false));g.fillEllipse(c.x-25,c.y-25,50,50);
    g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(c.x-23.7f,c.y-24.5f,47.4f,47.4f,.7f);
    const float angle=-1.05f+position*2.0f;auto tip=c+juce::Point<float>(std::cos(angle)*28,std::sin(angle)*28);auto inner=c+juce::Point<float>(std::cos(angle)*5,std::sin(angle)*5);
    g.setColour(juce::Colours::black.withAlpha(.7f));g.drawLine(inner.x,inner.y+1,tip.x,tip.y+1,4);
    g.setColour(juce::Colour(0xffdedfd0));g.drawLine(inner.x,inner.y,tip.x,tip.y,3);
}

void RackLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(1);
    if(b.getComponentID()=="arrow") {
        g.setColour(juce::Colour(0xff0a0c0c));g.fillRect(r);g.setColour(juce::Colour(0xff787b70));g.drawRect(r,1);
        r=r.reduced(5,4);auto c=juce::Colour(0xffc6c95b);
        if(!b.getToggleState())c=juce::Colour(0xff555747);
        g.setGradientFill(juce::ColourGradient(c.brighter(down?0.f:.45f),r.getCentreX(),r.getY(),c.darker(.45f),r.getCentreX(),r.getBottom(),false));g.fillRect(r);
        g.setColour(juce::Colours::white.withAlpha(over?.7f:.35f));g.drawRect(r,1);
    } else {
        g.setColour(down?juce::Colour(0xff383c38):juce::Colours::black);g.fillRect(r);
        g.setColour(over?juce::Colours::white:juce::Colour(0xff979b93));g.drawRect(r,1);
    }
}
void RackLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) {
    if(b.getComponentID()=="arrow") {
        juce::Path arrow;float x=b.getWidth()*.5f,y=b.getHeight()*.5f;
        arrow.startNewSubPath(x,y-5);arrow.lineTo(x-5,y);arrow.lineTo(x-2,y);arrow.lineTo(x-2,y+6);arrow.lineTo(x+2,y+6);arrow.lineTo(x+2,y);arrow.lineTo(x+5,y);arrow.closeSubPath();
        g.setColour(juce::Colours::white.withAlpha(.7f));g.fillPath(arrow,juce::AffineTransform::translation(0,1));g.setColour(juce::Colour(0xff4d541d));g.fillPath(arrow);
    } else if(b.getComponentID()=="undo" || b.getComponentID()=="redo") {
        juce::Path p;p.startNewSubPath(13,15);p.cubicTo(26,19,29,5,18,6);p.lineTo(12,6);
        juce::Path head;head.addTriangle(8,6,15,2,15,10);
        auto transform=b.getComponentID()=="redo"?juce::AffineTransform::scale(-1,1).translated((float)b.getWidth(),0):juce::AffineTransform();
        g.setColour(b.isEnabled()?ink:ink.darker(.65f));g.strokePath(p,juce::PathStrokeType(1.5f),transform);g.fillPath(head,transform);
    } else if(b.getComponentID()=="options") {
        juce::Path p;p.addTriangle(6,9,14,9,10,14);g.setColour(ink);g.fillPath(p);
    } else if(b.getComponentID()=="compare") {
        text(g,b.getButtonText().startsWith("A")?"A":"B",{4,1,13,19},13,ink);
        text(g,b.getButtonText().startsWith("A")?"B":"A",{39,1,13,19},13,ink);
        g.setColour(ink);g.drawLine(21,11,34,11,1);g.drawLine(30,7,34,11,1);g.drawLine(30,15,34,11,1);
    } else text(g,b.getButtonText(),b.getLocalBounds().toFloat(),14,ink);
}
void RackLook::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool,bool) {
    float cx=b.getWidth()*.5f,cy=b.getHeight()*.5f;
    juce::Path base;base.addEllipse(cx-20,cy-20,40,40);juce::DropShadow(juce::Colours::black.withAlpha(.8f),4,{3,3}).drawForPath(g,base);
    juce::ColourGradient chrome(juce::Colour(0xffc2c4b5),cx-16,cy-18,juce::Colour(0xff27312a),cx+18,cy+18,false);chrome.addColour(.42,juce::Colour(0xff555f53));chrome.addColour(.6,juce::Colour(0xffa7ad9f));g.setGradientFill(chrome);g.fillPath(base);
    g.setColour(juce::Colour(0xff101810));g.fillEllipse(cx-16,cy-16,32,32);g.setColour(juce::Colour(0xffa3ad9b));g.drawEllipse(cx-14,cy-14,28,28,1.6f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff323d31),cx-10,cy-10,juce::Colour(0xff868d7f),cx+8,cy+12,false));g.fillEllipse(cx-11,cy-9,22,22);
    float ly=cy+(b.getToggleState()?-19.f:8.f);juce::Path lever;lever.startNewSubPath(cx-7,ly);lever.lineTo(cx+5,ly-1);lever.lineTo(cx+8,cy+8);lever.quadraticTo(cx+1,cy+16,cx-5,cy+8);lever.closeSubPath();
    juce::DropShadow(juce::Colours::black.withAlpha(.7f),3,{3,2}).drawForPath(g,lever);
    juce::ColourGradient stem(juce::Colour(0xff454f42),cx-7,0,juce::Colour(0xffd1d4be),cx+6,0,false);stem.addColour(.55,juce::Colour(0xff89947d));g.setGradientFill(stem);g.fillPath(lever);
    auto bulb=juce::Rectangle<float>(cx-8,ly-6,16,15);auto color=b.getToggleState()?juce::Colour(0xffede89b):juce::Colour(0xff7a8065);
    if(b.getToggleState()){g.setColour(color.withAlpha(.09f));g.fillEllipse(bulb.expanded(4));}
    g.setGradientFill(juce::ColourGradient(color.brighter(.25f),cx-2,ly-5,color.darker(.3f),cx+3,ly+10,false));g.fillEllipse(bulb);g.setColour(juce::Colours::white.withAlpha(.55f));g.fillEllipse(cx-5,ly-4,6,4);
}

GuitarEditor::GuitarEditor(GuitarProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&look);addAndMakeVisible(canvas);canvas.setSize(width,height);
    canvas.draw=[this](juce::Graphics& g){drawPanel(g);};
    setResizable(true,true);getConstrainer()->setFixedAspectRatio(double(width)/height);
    setResizeLimits(840,540,1680,1080);setSize(width,height);
    texture=juce::Image(juce::Image::RGB,width,height,false);
    cloth=juce::Image(juce::Image::RGB,1060,124,false);
    juce::Random rng(2517);
    for(int y=0;y<height;++y)for(int x=0;x<width;++x){float cloud=4*std::sin(x*.033f)*std::sin(y*.028f)+2*std::sin((x+y)*.009f);int v=juce::jlimit(4,250,42+rng.nextInt(13)+(int)cloud);texture.setPixelAt(x,y,juce::Colour((juce::uint8)(v+2),(juce::uint8)(v+2),(juce::uint8)(v-1)));}
    for(int y=0;y<124;++y)for(int x=0;x<1060;++x){
        float warp=std::pow(juce::jmax(0.f,std::sin((x+y)*.82f)),.6f),weft=std::pow(juce::jmax(0.f,std::sin((x-y)*.65f)),.7f);
        float age=.91f+.07f*std::sin(x*.014f)*std::sin(y*.039f);
        float light=(.38f+.34f*warp+.20f*weft+rng.nextFloat()*.07f)*age;
        cloth.setPixelAt(x,y,juce::Colour::fromFloatRGBA(juce::jlimit(0.f,1.f,light*.98f),juce::jlimit(0.f,1.f,light*.66f),juce::jlimit(0.f,1.f,light*.39f),1));
    }
    for(int i=0;i<8;++i){auto& s=sliders[i];s.setSliderStyle(juce::Slider::LinearVertical);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setDoubleClickReturnValue(true,0);s.setPopupDisplayEnabled(true,true,&canvas);s.setTextValueSuffix(" dB");s.setName(labels[i]);canvas.addAndMakeVisible(s);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s));s.onDragStart=[this]{recordChange();};s.onDragEnd=[this]{recordChange();};}
    for(int k=0;k<6;++k){auto& b=colors[k];canvas.addAndMakeVisible(b);b.setComponentID("arrow");b.setToggleState(p.state.getRawParameterValue(juce::String(ids[k+1])+"On")->load()>.5f,juce::dontSendNotification);b.setTooltip("Clique: próxima Color. Shift+clique: Color anterior. Menu ▾: ativar/desativar módulo.");b.onClick=[this,k]{cycleColor(k,juce::ModifierKeys::getCurrentModifiers().isShiftDown());};}
    canvas.addAndMakeVisible(reamp);reamp.setTooltip("Re-Amplify ON/OFF");reampAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"reamp",reamp);reamp.onClick=[this]{recordChange();};
    canvas.addAndMakeVisible(amp);amp.setSliderStyle(juce::Slider::RotaryVerticalDrag);amp.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);amp.setPopupDisplayEnabled(false,false,nullptr);amp.setName("Amp mode");amp.setTooltip("Clean / Crunch / Heavy / Bypass");attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"amp",amp));amp.onDragStart=[this]{recordChange();};amp.onDragEnd=[this]{recordChange();};
    for(auto* b:{&undo,&redo,&compare,&load,&save,&options,&help})canvas.addAndMakeVisible(*b);
    undo.setComponentID("undo");redo.setComponentID("redo");compare.setComponentID("compare");options.setComponentID("options");
    undo.setTooltip("Undo");redo.setTooltip("Redo");compare.setTooltip("Alternar Setup A / Setup B");
    undo.onClick=[this]{undoState(-1);};redo.onClick=[this]{undoState(1);};compare.onClick=[this]{switchComparison();};
    load.onClick=[this]{presetMenu();};save.onClick=[this]{savePreset();};options.onClick=[this]{moreOptions();};
    help.onClick=[this]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Guitar Rack — controles","Arraste os faders: −10 a +10 dB. Duplo clique: 0 dB.\nSetas amarelas: próxima Color; Shift+clique: anterior.\nMenu ▾: ativar/desativar cada efeito, copiar A/B e restaurar a referência.\nA → B / B → A alterna dois estados independentes.\nRecriação independente; não é um produto Waves.");};
    baseline=p.state.copyState();comparison[0]=baseline.createCopy();comparison[1]=baseline.createCopy();recordChange();startTimerHz(30);
}
GuitarEditor::~GuitarEditor(){stopTimer();canvas.draw=nullptr;setLookAndFeel(nullptr);}
void GuitarEditor::paint(juce::Graphics& g){g.fillAll(juce::Colours::black);}
void GuitarEditor::resized(){float scale=juce::jmin(getWidth()/float(width),getHeight()/float(height));canvas.setTransform(juce::AffineTransform::scale(scale));canvas.setTopLeftPosition(0,0);
    undo.setBounds(241,3,42,21);redo.setBounds(287,3,42,21);compare.setBounds(887,3,57,21);load.setBounds(948,3,55,21);save.setBounds(1007,3,55,21);options.setBounds(1066,3,20,21);help.setBounds(1090,3,25,21);
    reamp.setBounds(52,195,66,61);amp.setBounds(122,183,76,76);
    for(int i=0;i<8;++i)sliders[i].setBounds(centers[i]-32,304,64,354);
    for(int k=0;k<6;++k)colors[k].setBounds(centers[k+1]-29,249,58,39);
}
void GuitarEditor::setParameter(const juce::String& id,float v){auto* a=processor.state.getParameter(id);a->beginChangeGesture();a->setValueNotifyingHost(a->convertTo0to1(v));a->endChangeGesture();}
void GuitarEditor::recordChange(){if(restoring)return;auto s=processor.state.copyState();if(historyIndex>=0&&history[(size_t)historyIndex].isEquivalentTo(s))return;history.resize((size_t)(historyIndex+1));history.push_back(s);if(history.size()>100)history.erase(history.begin());historyIndex=(int)history.size()-1;}
void GuitarEditor::restore(const juce::ValueTree& s){if(!s.isValid())return;restoring=true;for(auto* parameter:processor.getParameters()){auto* p=dynamic_cast<juce::RangedAudioParameter*>(parameter);if(!p)continue;auto node=s.getChildWithProperty("id",p->paramID);if(node.isValid()){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1((float)node.getProperty("value")));p->endChangeGesture();}}restoring=false;}
void GuitarEditor::undoState(int direction){recordChange();int next=juce::jlimit(0,(int)history.size()-1,historyIndex+direction);if(next!=historyIndex){historyIndex=next;restore(history[(size_t)next]);}}
void GuitarEditor::switchComparison(){recordChange();comparison[(size_t)activeSlot]=processor.state.copyState();activeSlot=1-activeSlot;restore(comparison[(size_t)activeSlot]);baseline=comparison[(size_t)activeSlot].createCopy();presetTitle=activeSlot==0?"Setup A":"Setup B";recordChange();}
void GuitarEditor::cycleColor(int k,bool reverse){juce::String id=juce::String(ids[k+1])+"Color";int current=(int)processor.state.getRawParameterValue(id)->load();setParameter(id,(current+(reverse?2:1))%3);recordChange();}
void GuitarEditor::savePreset(){
    chooser=std::make_unique<juce::FileChooser>("Save preset",juce::File{},"*.xml");const juce::Component::SafePointer<GuitarEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& c){if(safe==nullptr||c.getResult()==juce::File{})return;auto xml=safe->processor.state.copyState().createXml();if(xml->writeTo(c.getResult().withFileExtension("xml"))){safe->presetTitle=c.getResult().getFileNameWithoutExtension();safe->baseline=safe->processor.state.copyState();}});
}
void GuitarEditor::loadPreset(){
    chooser=std::make_unique<juce::FileChooser>("Load preset",juce::File{},"*.xml");const juce::Component::SafePointer<GuitarEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& c){if(safe==nullptr)return;if(auto xml=juce::XmlDocument::parse(c.getResult())){auto s=juce::ValueTree::fromXml(*xml);if(s.hasType(safe->processor.state.state.getType())){safe->restore(s);safe->presetTitle=c.getResult().getFileNameWithoutExtension();safe->baseline=safe->processor.state.copyState();safe->recordChange();}}});
}
void GuitarEditor::presetMenu(){juce::PopupMenu menu;auto& presets=GuitarProcessor::presets();for(auto folder:{"Acoustic","Clean","Crunch","Heavy","Lead / Solo","FX / Special"}){juce::PopupMenu sub;for(size_t i=0;i<presets.size();++i)if(juce::String(folder)==presets[i].folder)sub.addItem((int)i+1,presets[i].name);menu.addSubMenu(folder,sub);}menu.addSeparator();menu.addItem(1000,"Load user preset…");const juce::Component::SafePointer<GuitarEditor> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(load),[safe](int id){if(safe==nullptr)return;if(id>0&&id<1000){safe->processor.setCurrentProgram(id-1);safe->presetTitle=safe->processor.getProgramName(id-1);safe->baseline=safe->processor.state.copyState();safe->recordChange();}if(id==1000)safe->loadPreset();});}
void GuitarEditor::moreOptions(){juce::PopupMenu m;m.addItem(1,"Copy current setup to the other A/B slot");m.addItem(2,"Restore reference settings");m.addSeparator();for(int k=0;k<6;++k)m.addItem(10+k,labels[k+1],true,processor.state.getRawParameterValue(juce::String(ids[k+1])+"On")->load()>.5f);const juce::Component::SafePointer<GuitarEditor> safe(this);m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(options),[safe](int id){if(safe==nullptr)return;if(id==1)safe->comparison[(size_t)(1-safe->activeSlot)]=safe->processor.state.copyState();if(id==2){const float f[]={1,-4,4,1.5f,-4,4,-6,2};for(int i=0;i<8;++i)safe->setParameter(ids[i],f[i]);const int c[]={1,2,1,0,1,2};for(int i=0;i<6;++i){safe->setParameter(juce::String(ids[i+1])+"Color",(float)c[i]);safe->setParameter(juce::String(ids[i+1])+"On",1);}safe->setParameter("amp",2);safe->setParameter("reamp",1);}if(id>=10&&id<16){juce::String key=juce::String(ids[id-9])+"On";safe->setParameter(key,safe->processor.state.getRawParameterValue(key)->load()>.5f?0:1);}safe->recordChange();});}
void GuitarEditor::timerCallback(){for(int c=0;c<2;++c){meterInput[(size_t)c]=juce::jmax(processor.inputChannels[(size_t)c].load(),meterInput[(size_t)c]*.86f);meterOutput[(size_t)c]=juce::jmax(processor.outputChannels[(size_t)c].load(),meterOutput[(size_t)c]*.86f);}meterIn=juce::jmax(processor.inMeter.load(),meterIn*.86f);meterOut=juce::jmax(processor.outMeter.load(),meterOut*.86f);clipIn=processor.inMeter.load()>=1?45:juce::jmax(0,clipIn-1);clipOut=processor.outMeter.load()>=1?45:juce::jmax(0,clipOut-1);for(int k=0;k<6;++k)colors[k].setToggleState(processor.state.getRawParameterValue(juce::String(ids[k+1])+"On")->load()>.5f,juce::dontSendNotification);undo.setEnabled(historyIndex>0);redo.setEnabled(historyIndex+1<(int)history.size());compare.setButtonText(activeSlot==0?"A → B":"B → A");canvas.repaint();}

void GuitarEditor::drawPanel(juce::Graphics& g){
    g.drawImageAt(texture,0,0);
    // Cast-metal frame and rolled chrome side rails.
    g.setColour(juce::Colour(0xff111415));g.fillRoundedRectangle(12,29,1096,681,25);
    g.setColour(juce::Colour(0xff60635f));g.drawRoundedRectangle(13,29,1094,680,24,3);
    g.drawImage(texture,juce::Rectangle<float>(31,42,1058,650));
    for(bool right:{false,true}){
        float x=right?1080.f:16.f;juce::Path rail;rail.startNewSubPath(x+17,32);rail.quadraticTo(x-2,47,x,103);rail.lineTo(x,656);rail.quadraticTo(x,701,x+18,708);rail.lineTo(x+29,700);rail.quadraticTo(x+14,691,x+14,654);rail.lineTo(x+14,105);rail.quadraticTo(x+10,73,x+31,61);rail.closeSubPath();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff272d31),x,0,juce::Colour(0xffa6adae),x+22,0,false));g.fillPath(rail);g.setColour(juce::Colour(0xff141a1c));g.strokePath(rail,juce::PathStrokeType(3));
        g.setColour(juce::Colours::white.withAlpha(.5f));g.drawLine(x+4,111,x+4,652,2);
    }
    g.saveState();juce::Path grille;grille.startNewSubPath(60,40);grille.lineTo(1060,40);grille.lineTo(1090,98);grille.lineTo(1090,164);grille.lineTo(30,164);grille.lineTo(30,98);grille.closeSubPath();g.reduceClipRegion(grille);
    g.drawImageAt(cloth,30,40);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.65f),0,41,juce::Colours::transparentBlack,0,80,false));g.fillRect(30,40,1060,42);
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,0,128,juce::Colours::black.withAlpha(.6f),0,164,false));g.fillRect(30,128,1060,37);
    g.restoreState();
    screw(g,58,126,9);screw(g,1063,126,9);
    text(g,"Chris Lord-Alge",{94,99,330,48},35,juce::Colours::black,juce::Font::bold|juce::Font::italic,juce::Justification::left);
    text(g,"Chris Lord-Alge",{92,96,330,48},35,juce::Colour(0xffdce0dc),juce::Font::bold|juce::Font::italic,juce::Justification::left);
    g.setColour(juce::Colour(0xff919a98));g.drawLine(94,140,287,140,2);
    // The reference locates the embossed emblem on the right dark panel.
    g.setColour(juce::Colours::white.withAlpha(.09f));g.drawEllipse(899,187,140,62,4);
    g.setColour(juce::Colour(0xff181b19));g.drawEllipse(900,185,140,62,4);
    g.setColour(juce::Colour(0xff303432));g.fillRect(902,227,139,27);
    text(g,"Guitars",{903,204,139,40},32,juce::Colour(0xff535751),juce::Font::bold);
    text(g,"Guitars",{902,202,139,40},32,juce::Colour(0xff171b18),juce::Font::bold);
    for(int x:{279,846}){g.setColour(juce::Colours::black);g.fillRect(x,164,3,529);g.setColour(juce::Colours::white.withAlpha(.1f));g.fillRect(x+3,165,1,525);}
    g.setColour(juce::Colours::black);g.fillRect(30,273,248,3);g.fillRect(850,273,239,3);
    // Shared recessed Color display.
    g.setColour(juce::Colour(0xff0b0e0d));g.fillRect(290,172,548,67);g.setColour(juce::Colour(0xff5b6057));g.drawRect(293,175,542,61,1);
    for(int y=178;y<235;y+=3){g.setColour(juce::Colours::white.withAlpha(.025f));g.drawHorizontalLine(y,295,833);}
    for(int k=0;k<6;++k){int active=juce::jlimit(0,2,(int)processor.state.getRawParameterValue(juce::String(ids[k+1])+"Color")->load());bool on=processor.state.getRawParameterValue(juce::String(ids[k+1])+"On")->load()>.5f;for(int row=0;row<3;++row){auto c=active==row&&on?colorFor(row):juce::Colour(0xff444943);if(k==0&&row==1&&active==row&&on)c=colorFor(2);text(g,names[k][row],{float(centers[k+1]-43),178.f+row*18,86,18},12,c,juce::Font::bold|juce::Font::italic);}}
    text(g,"Re-Amplify",{48,166,160,24},16,ink,juce::Font::plain,juce::Justification::left);
    text(g,"ON",{64,189,42,20},13,juce::Colour(0xffe8e27c),juce::Font::bold|juce::Font::italic);
    text(g,"OFF",{64,253,42,18},12,ink,juce::Font::italic);
    int mode=(int)processor.state.getRawParameterValue("amp")->load();
    const char* modes[]={"CLEAN","CRUNCH","HEAVY","BYPASS"};
    for(int j=0;j<4;++j){g.setColour(mode==j?juce::Colour(0xffe7d96a):juce::Colour(0xff777d74));g.fillEllipse(196,193.f+20*j,3,3);text(g,modes[j],{204,185.f+20*j,70,20},12,mode==j?juce::Colour(0xffe7d96a):juce::Colour(0xff858c82),juce::Font::bold|juce::Font::italic,juce::Justification::left);}
    for(int i=0;i<8;++i){int x=centers[i];for(int j=0;j<3;++j){float y=357.f+j*115;text(g,j==0?"+10":j==1?"0":"−10",{float(i==0?x-63:x+22),y,39,17},12,juce::Colour(0xff969e94));}text(g,labels[i],{float(x-49),660,98,25},14,ink);}
    const float inputDb=juce::Decibels::gainToDecibels(meterIn,-60.f);
    led(g,{99,300,21,13},inputDb<-40?juce::Colour(0xff35452c):inputDb>=0?juce::Colours::red:inputDb>-12?juce::Colour(0xffe5de55):juce::Colour(0xff86c849));
    auto meter=[&](int x,const std::array<float,2>& levels,int clip){
        g.setColour(juce::Colour(0xff111412));g.fillRoundedRectangle(float(x+9),292,34,353,10);screw(g,float(x+26),309,8);screw(g,float(x+26),628,8);
        juce::Rectangle<float> r(float(x-4),332,61,272);g.setColour(juce::Colour(0xff060807));g.fillRect(r);g.setColour(juce::Colour(0xff72766c));g.drawRect(r.reduced(1),1);g.setColour(juce::Colour(0xff353e35));g.fillRect(r.reduced(4));
        for(int j=0;j<36;++j){float y=590.f-j*6.7f;for(int ch=0;ch<2;++ch){float db=juce::Decibels::gainToDecibels(levels[(size_t)ch],-60.f);bool on=db>=-24.f+j*(24.f/35);auto c=j>32?(clip?juce::Colour(0xffe73527):juce::Colour(0xff60302b)):(on?juce::Colour(0xff89e83c):juce::Colour(0xff25502d));g.setColour(c);g.fillRect(float(x+3+ch*33),y,13.f,5.2f);g.setColour(juce::Colours::black.withAlpha(.15f));g.drawVerticalLine(x+9+ch*33,y,y+5);}}
        for(int j=0;j<5;++j)text(g,juce::String(j*6),{float(x+15),341.f+j*58,23,15},10,juce::Colour(0xffb8bfb3));
    };meter(182,meterInput,clipIn);meter(890,meterOutput,clipOut);
    // Input meter trim/reset medallion, with stereo channel markings.
    g.setColour(juce::Colour(0xff0a0c0b));g.fillEllipse(191,610,32,32);g.setColour(juce::Colour(0xffa3aaa0));g.drawEllipse(196,615,22,22,2);g.setColour(juce::Colour(0xff303932));g.fillRect(203,612,8,8);g.setColour(juce::Colour(0xffb8bdb3));g.drawLine(207,610,207,624,2);text(g,"L",{177,624,15,18},11,ink,juce::Font::italic);text(g,"R",{223,624,15,18},11,ink,juce::Font::italic);
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack,0,672,juce::Colours::black.withAlpha(.9f),0,704,false));g.fillRoundedRectangle(31,672,1058,33,15);
    // Narrow original-style preset toolbar.
    g.setColour(juce::Colours::black);g.fillRect(0,0,width,29);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd7d8d1),0,3,juce::Colour(0xff8c918c),0,24,false));g.fillRect(3,3,234,21);g.setColour(juce::Colour(0xff424641));g.drawRect(3,3,234,21,1);
    text(g,"CHRIS LORD-ALGE COLLECTION",{5,3,230,21},14,juce::Colour(0xff7c2026),juce::Font::bold);
    g.setColour(juce::Colour(0xff9c9f96));g.drawRect(334,3,549,21,1);
    auto state=processor.state.copyState();text(g,presetTitle+(state.isEquivalentTo(baseline)?"":"*"),{337,3,542,21},14,ink);
}
