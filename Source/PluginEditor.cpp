#include "PluginEditor.h"
#include <cmath>
GrainBrainAudioProcessorEditor::GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor& p):AudioProcessorEditor(&p),processor(p),orb(p){
 addAndMakeVisible(orb);
 addAndMakeVisible(clearRoutes);
 for(auto* b:{&storeA,&storeB,&recallA,&recallB})addAndMakeVisible(*b);
 storeA.onClick=[this]{processor.getStateInformation(snapshotA);};
 storeB.onClick=[this]{processor.getStateInformation(snapshotB);};
 recallA.onClick=[this]{if(snapshotA.getSize()>0)processor.setStateInformation(snapshotA.getData(),int(snapshotA.getSize()));};
 recallB.onClick=[this]{if(snapshotB.getSize()>0)processor.setStateInformation(snapshotB.getData(),int(snapshotB.getSize()));};
 for(auto* b:{&freezeButton,&recordButton,&playButton,&seqButton,&resetJoyButton})addAndMakeVisible(*b);
 auto toggle=[this](const char* id){if(auto* par=processor.params.getParameter(id))par->setValueNotifyingHost(par->getValue()>.5f?0.f:1.f);};
 freezeButton.onClick=[toggle]{toggle("freeze");};
 recordButton.onClick=[toggle]{toggle("joyRecord");};
 playButton.onClick=[toggle]{toggle("joyPlay");};
 seqButton.onClick=[toggle]{toggle("seqOn");};
 resetJoyButton.onClick=[this]{for(const auto* id:{"joyX","joyY"})if(auto* p=processor.params.getParameter(id))p->setValueNotifyingHost(.5f);};
 auto setDial=[this](juce::Slider& slider,juce::Label& label,const char* title,const char* id,std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment){
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,18);
  slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffa47df3));
  label.setText(title,juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId,juce::Colours::white);
  addAndMakeVisible(slider);addAndMakeVisible(label);
  attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,slider);
 };
 setDial(delayMixKnob,delayLabel,"DELAY MIX","delayMix",delayMixAttachment);
 setDial(delayFeedbackKnob,feedbackLabel,"FEEDBACK","delayFeedback",delayFeedbackAttachment);
 setDial(seqDepthKnob,seqLabel,"SEQ DEPTH","seqDepth",seqDepthAttachment);
 for(auto* b:{&randomButton,&randomSafeButton,&lfoSyncButton})addAndMakeVisible(*b);
 randomAmount.setRange(.05,1.0,.05);randomAmount.setValue(.6,juce::dontSendNotification);
 randomAmount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 randomAmount.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,18);
 randomAmount.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffffbd74));
 addAndMakeVisible(randomAmount);
 randomLabel.setText("CHAOS",juce::dontSendNotification);
 randomLabel.setJustificationType(juce::Justification::centred);
 randomLabel.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(randomLabel);
 randomButton.onClick=[this]{processor.randomizePatch(float(randomAmount.getValue()),false);};
 randomSafeButton.onClick=[this]{processor.randomizePatch(float(randomAmount.getValue()),true);};
 lfoSyncButton.onClick=[toggle]{toggle("lfoSync");};
 auto setupCombo=[this](juce::ComboBox& combo,juce::Label& label,const char* title,const char* id,const juce::StringArray& choices){
  for(int i=0;i<choices.size();++i)combo.addItem(choices[i],i+1);
  addAndMakeVisible(combo);
  label.setText(title,juce::dontSendNotification);label.setColour(juce::Label::textColourId,juce::Colours::white);
  label.setJustificationType(juce::Justification::centred);addAndMakeVisible(label);
  comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,id,combo));
 };
 setupCombo(filterModeBox,modeLabel,"FILTER TYPE","filterMode",{"Low-pass","High-pass","Band-pass","Notch","Vowel bank"});
 setupCombo(beatDivisionBox,beatLabel,"GLITCH BEAT","seqDivision",{"1/2","1/4","1/8","1/16","1/32","1/64"});
 setupCombo(lfoDivisionBox,lfoBeatLabel,"LFO BEAT","lfoDivision",{"1/2","1/4","1/8","1/16","1/32","1/64"});
 auto dial=[this](juce::Slider& slider,juce::Label& label,const char* title,const char* id){
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,18);
  slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff8af0cb));
  label.setText(title,juce::dontSendNotification);label.setColour(juce::Label::textColourId,juce::Colours::white);
  label.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(slider);addAndMakeVisible(label);
  newAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,slider));
 };
 dial(vowelKnob,vowelLabel,"A E I O U","vowel");
 dial(vowelShiftKnob,shiftLabel,"VOWEL SHIFT","vowelShift");
 dial(timeMorphKnob,timeLabel,"TIME DRIFT","seqTimeMod");
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
 setSize(1100,1070); // All child controls exist before resized() runs.
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
 sampleLabel.setBounds(585,96,360,32);helpLabel.setBounds(25,1025,1040,24);
 orb.setBounds(380,162,290,290);clearRoutes.setBounds(470,503,150,28);
 freezeButton.setBounds(310,457,87,28);recordButton.setBounds(400,457,93,28);
 playButton.setBounds(496,457,93,28);resetJoyButton.setBounds(593,457,107,28);
 seqButton.setBounds(810,460,158,29);
 storeA.setBounds(735,525,80,28);recallA.setBounds(823,525,80,28);
 storeB.setBounds(735,560,80,28);recallB.setBounds(823,560,80,28);
 delayMixKnob.setBounds(388,586,85,90);delayLabel.setBounds(383,676,95,22);
 delayFeedbackKnob.setBounds(490,586,85,90);feedbackLabel.setBounds(480,676,105,22);
 seqDepthKnob.setBounds(596,586,85,90);seqLabel.setBounds(586,676,105,22);
 for(int i=0;i<16;++i){int x=0,y=0;
  if(i<6){x=25+(i%3)*112;y=170+(i/3)*150;}
  else if(i<12){x=730+((i-6)%3)*112;y=170+((i-6)/3)*150;}
  else {x=40+((i-12)%2)*112;y=520+((i-12)/2)*130;}
  sliders[i]->setBounds(x,y,100,78);labels[i]->setBounds(x-5,y+78,110,22);
  dragHandles[i]->setBounds(x+15,y+105,70,21);
 }
 randomButton.setBounds(30,832,127,32);randomSafeButton.setBounds(166,832,150,32);
 randomAmount.setBounds(74,878,90,90);randomLabel.setBounds(65,970,105,22);
 filterModeBox.setBounds(353,842,180,28);modeLabel.setBounds(353,815,180,24);
 vowelKnob.setBounds(349,895,92,86);vowelLabel.setBounds(341,982,110,22);
 vowelShiftKnob.setBounds(465,895,92,86);shiftLabel.setBounds(455,982,110,22);
 beatDivisionBox.setBounds(623,842,120,28);beatLabel.setBounds(623,815,120,24);
 lfoDivisionBox.setBounds(793,842,120,28);lfoBeatLabel.setBounds(793,815,120,24);
 timeMorphKnob.setBounds(641,895,92,86);timeLabel.setBounds(634,982,105,22);
 lfoSyncButton.setBounds(795,901,117,32);
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
 // Show active routing cables, cyan for X and magenta for Y.
 const juce::String routeNames[]={"SIZE","DENSITY","POSITION","SPRAY","PITCH","PITCH RAND","REVERSE","SPREAD","GAIN","LFO RATE","LFO DEPTH","CUTOFF","RESONANCE","TYPE","MOTION","GLITCH"};
 int nx=0,ny=0;
 for(int i=0;i<16;++i){
  auto id="joyRoute"+juce::String(i);
  const auto* p=params.getRawParameterValue(id);
  if(!p)continue;
  float route=p->load();
  if(route<.5f)continue;
  const bool xAxis=route<1.5f;
  const int slot=xAxis?nx++:ny++;
  const float yy=55.f+float(slot%8)*23.f;
  const float tx=xAxis?17.f:float(getWidth()-17);
  const float startX=xAxis?cx-r*.7f:cx+r*.7f;
  juce::Path cable;cable.startNewSubPath(startX,cy+(slot%4-2)*11.f);cable.cubicTo(cx+(xAxis?-r:r),yy,tx+ (xAxis?40.f:-40.f),yy,tx,yy);
  g.setColour(xAxis?juce::Colour(0x8865f7ee):juce::Colour(0x88f38be7));
  g.strokePath(cable,juce::PathStrokeType(1.15f));
  g.setFont(9.5f);
  g.drawFittedText(routeNames[i],juce::Rectangle<int>(xAxis?0:getWidth()-91,int(yy-8),91,16),xAxis?juce::Justification::centredLeft:juce::Justification::centredRight,1);
 }
 const auto visual=processor.getGrainVisuals();
 for(const auto& gr:visual)if(gr.energy>.001f){
  const float xx=cx+(gr.position-.5f)*r*1.55f, yy=cy-gr.pan*r*.58f;
  g.setColour(juce::Colour(0xff79f4dd).withAlpha(juce::jlimit(0.f,.85f,gr.energy)));
  g.fillEllipse(xx-2.f,yy-2.f,4.f,4.f);
 }
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
