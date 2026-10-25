#include "PluginEditor.h"
#include <cmath>
GrainBrainAudioProcessorEditor::GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor& p):AudioProcessorEditor(&p),processor(p),orb(p.params){
 addAndMakeVisible(orb);
 addAndMakeVisible(clearRoutes);
 clearRoutes.onClick=[this]{for(int i=0;i<16;++i)if(auto* p=processor.params.getParameter("joyRoute"+juce::String(i)))p->setValueNotifyingHost(0.f);};
 heading.setText("GRAIN BRAIN / GRANULAR RANDOMIZER",juce::dontSendNotification);
 heading.setFont(juce::Font(juce::FontOptions(23.0f).withStyle("Bold")));
 heading.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(heading);
 sampleLabel.setText(p.getSampleName(),juce::dontSendNotification);sampleLabel.setColour(juce::Label::textColourId,juce::Colours::lightgreen);addAndMakeVisible(sampleLabel);
 helpLabel.setText("Drop WAV / AIFF here  |  Drag LINK onto brain: left = X, right = Y  |  CLEAR LINKS resets",juce::dontSendNotification);
 helpLabel.setColour(juce::Label::textColourId,juce::Colours::lightgrey);addAndMakeVisible(helpLabel);
 addAndMakeVisible(loadButton);addAndMakeVisible(factoryButton);addAndMakeVisible(saveButton);addAndMakeVisible(openButton);addAndMakeVisible(presetBox);
 presetBox.addItem("Choose preset...",1);
 const char* presetNames[]={
  "Classic / Nebula","Classic / Frozen Cloud","Classic / Glitch Storm","Classic / Deep Space","Classic / Tight Pulses",
  "Motion / Breathing Clouds","Motion / Rhythmic Pulses","Motion / Stereo Drift","Motion / Slow Orbit",
  "Pro / Recursive Orbit","Pro / Polyrhythm Engine","Pro / Fractal Drift","Pro / Interlocked Chaos"
 };
 for(int i=0;i<13;++i)presetBox.addItem(presetNames[i],i+2);
 presetBox.setSelectedId(1,juce::dontSendNotification);
 presetBox.onChange=[this]{int id=presetBox.getSelectedId();if(id>=2)processor.applyPreset(id-2);};
 loadButton.onClick=[this]{chooser=std::make_unique<juce::FileChooser>("Choose a sample",juce::File{},"*.wav;*.aif;*.aiff");
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){auto f=c.getResult();if(f.existsAsFile())sampleLabel.setText(processor.loadSample(f)?processor.getSampleName():"Could not load sample",juce::dontSendNotification);});};
 factoryButton.onClick=[this]{processor.useFactoryWave();sampleLabel.setText(processor.getSampleName(),juce::dontSendNotification);};
 saveButton.onClick=[this]{
  auto directory=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GrainBrain").getChildFile("Presets");
  directory.createDirectory();
  chooser=std::make_unique<juce::FileChooser>("Save GrainBrain preset",directory.getChildFile("My Sound.gbpreset"),"*.gbpreset");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::warnAboutOverwriting,[this](const juce::FileChooser& c){auto f=c.getResult();if(f!=juce::File{}){if(!f.hasFileExtension("gbpreset"))f=f.withFileExtension("gbpreset");sampleLabel.setText(processor.saveUserPreset(f)?"Saved: "+f.getFileName():"Could not save preset",juce::dontSendNotification);}});
 };
 openButton.onClick=[this]{
  auto directory=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GrainBrain").getChildFile("Presets");
  chooser=std::make_unique<juce::FileChooser>("Open GrainBrain preset",directory,"*.gbpreset");
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){auto f=c.getResult();if(f.existsAsFile())sampleLabel.setText(processor.loadUserPreset(f)?"Loaded: "+f.getFileName():"Invalid preset",juce::dontSendNotification);});
 };
 const char* ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch"};
 const char* names[]={"SIZE ms","DENSITY","POSITION","SPRAY","PITCH","PITCH RAND","REVERSE","STEREO","GAIN","LFO RATE","LFO DEPTH","FILTER Hz","RESONANCE","FILTER TYPE","FILTER MOTION","GLITCH"};
 for(int i=0;i<16;++i){
  auto s=std::make_unique<juce::Slider>();s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,75,18);
  s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff8af0cb));s->setColour(juce::Slider::thumbColourId,juce::Colours::white);
  addAndMakeVisible(*s);
  attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,ids[i],*s));
  sliders.push_back(std::move(s));
  auto l=std::make_unique<juce::Label>();l->setText(names[i],juce::dontSendNotification);l->setJustificationType(juce::Justification::centred);
  l->setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(*l);labels.push_back(std::move(l));
  struct LinkHandle : juce::Component {
   juce::String id;
   explicit LinkHandle(juce::String name):id(std::move(name)){setMouseCursor(juce::MouseCursor::DraggingHandCursor);}
   void paint(juce::Graphics& g) override {g.setColour(juce::Colour(0xff89ead1));g.setFont(11.f);g.drawFittedText("↗ LINK",getLocalBounds(),juce::Justification::centred,1);}
   void mouseDrag(const juce::MouseEvent&) override {
    if(auto* drag=juce::DragAndDropContainer::findParentDragContainerFor(this))
     if(!drag->isDragAndDropActive())drag->startDragging(id,this);
   }
  };
  auto handle=std::make_unique<LinkHandle>(ids[i]);
  addAndMakeVisible(*handle);
  dragHandles.push_back(std::move(handle));
 }
 setSize(980,720); // All child controls exist before resized() runs.
}
void GrainBrainAudioProcessorEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff111622));g.setColour(juce::Colour(0xff25384a));
 g.fillRoundedRectangle(16,85,float(getWidth()-32),55,10);
 g.setColour(juce::Colour(0xff283444));g.drawRoundedRectangle(16,85,float(getWidth()-32),55,10,1);
}
void GrainBrainAudioProcessorEditor::resized(){
 heading.setBounds(20,14,560,42);presetBox.setBounds(730,25,225,28);
 loadButton.setBounds(25,96,130,32);factoryButton.setBounds(165,96,130,32);
 saveButton.setBounds(305,96,130,32);openButton.setBounds(445,96,130,32);
 sampleLabel.setBounds(585,96,360,32);helpLabel.setBounds(25,676,920,24);
 orb.setBounds(345,162,290,290);
 clearRoutes.setBounds(419,465,150,28);
 for(int i=0;i<16;++i){int x=0,y=0;
  if(i<6){x=15+(i%3)*108;y=165+(i/3)*148;}
  else if(i<12){x=655+((i-6)%3)*108;y=165+((i-6)/3)*148;}
  else {x=135+((i-12)%4)*178;y=505;}
  sliders[i]->setBounds(x,y,100,78);labels[i]->setBounds(x-5,y+78,110,22);dragHandles[i]->setBounds(x+15,y+105,70,21);
 }
}
bool GrainBrainAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files){
 for(auto& f:files)if(juce::File(f).hasFileExtension("wav;aif;aiff"))return true;return false;
}
void GrainBrainAudioProcessorEditor::filesDropped(const juce::StringArray& files,int,int){
 if(!files.isEmpty()){auto f=juce::File(files[0]);sampleLabel.setText(processor.loadSample(f)?processor.getSampleName():"Invalid audio file",juce::dontSendNotification);}
}

void GrainBrainOrb::paint(juce::Graphics& g){
 auto area=getLocalBounds().toFloat().reduced(12.f);
 const auto cx=area.getCentreX(),cy=area.getCentreY(),r=juce::jmin(area.getWidth(),area.getHeight())*.45f;
 g.setColour(dragHighlight?juce::Colour(0xff32675b):juce::Colour(0xff202f46));g.fillEllipse(cx-r,cy-r,2*r,2*r);
 for(int side=-1;side<=1;side+=2)for(int k=0;k<5;++k){juce::Path fold;float yy=cy-51.f+k*25.f;fold.startNewSubPath(cx+side*7.f,yy);fold.cubicTo(cx+side*35.f,yy-21.f,cx+side*73.f,yy+23.f,cx+side*58.f,yy+8.f);g.setColour(juce::Colour(0x556be4c8));g.strokePath(fold,juce::PathStrokeType(1.4f));}
 g.setColour(juce::Colour(0xff38546b));g.drawEllipse(cx-r,cy-r,2*r,2*r,2.f);
 for(int i=1;i<=3;++i){float rr=r*i/4.f;g.setColour(juce::Colour(0x443fb8af));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,1.f);}
 auto raw=[&](const char* id){return params.getRawParameterValue(id)->load();};
 float x=juce::jlimit(0.f,1.f,raw("joyX"));
 float y=juce::jlimit(0.f,1.f,raw("joyY"));
 float px=cx+(x-.5f)*2.f*r*.84f,py=cy+(.5f-y)*2.f*r*.84f;
 g.setColour(juce::Colour(0x558af0cb));g.drawLine(cx,cy,px,py,2.f);
 juce::ColourGradient gradient(juce::Colour(0xffa8ffdc),px-12,py-12,juce::Colour(0xff1687ab),px+24,py+24,false);
 g.setGradientFill(gradient);g.fillEllipse(px-15,py-15,30,30);
 g.setColour(juce::Colours::white);g.drawEllipse(px-15,py-15,30,30,1.5f);
 g.setColour(juce::Colours::lightgrey);g.setFont(12.f);g.drawFittedText("BRAIN JOYSTICK  /  X + Y",getLocalBounds().removeFromBottom(20),juce::Justification::centred,1);
}
void GrainBrainOrb::mouseDrag(const juce::MouseEvent& e){
 const float radius=juce::jmin(getWidth(),getHeight())*.45f*.84f;
 const float x=juce::jlimit(0.f,1.f,.5f+(e.position.x-getWidth()*.5f)/(2.f*radius));
 const float y=juce::jlimit(0.f,1.f,.5f-(e.position.y-getHeight()*.5f)/(2.f*radius));
 auto set=[&](const char* id,float value){if(auto* p=params.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();}};
 set("joyX",x);set("joyY",y);
 repaint();
}

bool GrainBrainOrb::isInterestedInDragSource(const SourceDetails& details){
 const juce::String ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch"};
 for(const auto& id:ids)if(details.description.toString()==id)return true;
 return false;
}
void GrainBrainOrb::itemDropped(const SourceDetails& details){
 dragHighlight=false;
 const juce::String ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch"};
 for(int i=0;i<16;++i)if(details.description.toString()==ids[i]){
  if(auto* p=params.getParameter("joyRoute"+juce::String(i))){
   const float axis=details.localPosition.x<getWidth()/2?1.f:2.f;
   const float old=p->convertFrom0to1(p->getValue());
   p->setValueNotifyingHost(p->convertTo0to1(std::abs(old-axis)<.1f?0.f:axis));
  }
  break;
 }
 repaint();
}
