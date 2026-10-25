#include "PluginEditor.h"
#include <cmath>
namespace C=GrainBrainColours;
namespace {
const char* const knobIds[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch"};
bool isPercent(const juce::String& id){
 for(const auto* p:{"position","spray","reverse","spread","gain","lfoDepth","filterMotion","filterMix","glitch","delayMix","delayFeedback","seqDepth","seqTimeMod"})if(id==p)return true;
 return false;
}
juce::String formatValue(const juce::String& id,double v){
 if(id=="size")return juce::String(juce::roundToInt(v))+" ms";
 if(id=="density")return juce::String(v,1)+" /s";
 if(id=="pitch"||id=="vowelShift")return (v>=0?"+":"")+juce::String(v,1)+" st";
 if(id=="pitchRand")return juce::String(v,1)+" st";
 if(id=="cutoff")return v>=1000?juce::String(v/1000.,2)+" kHz":juce::String(juce::roundToInt(v))+" Hz";
 if(id=="lfoRate")return juce::String(v,2)+" Hz";
 if(id=="resonance")return juce::String(v,2);
 if(id=="vowel"){
  const char* letters[]={"A","E","I","O","U"};
  const int a=juce::jlimit(0,4,int(std::floor(v)));const double f=v-a;
  if(a==4||f<.08)return letters[a];
  if(f>.92)return letters[a+1];
  return juce::String(letters[a])+" > "+letters[a+1];
 }
 if(isPercent(id))return juce::String(juce::roundToInt(v*100))+"%";
 return juce::String(v,2);
}
double parseValue(const juce::String& id,const juce::String& text){
 auto t=text.trim();double n=t.retainCharacters("0123456789.-").getDoubleValue();
 if(id=="cutoff"&&t.containsIgnoreCase("k"))n*=1000;
 if(isPercent(id)&&(t.contains("%")||n>1.0))n/=100;
 return n;
}
// Attachments install the parameter's own text functions, so this must run after attaching.
void formatKnob(juce::Slider& s,const juce::String& id){
 s.textFromValueFunction=[id](double v){return formatValue(id,v);};
 s.valueFromTextFunction=[id](const juce::String& t){return parseValue(id,t);};
 if(id=="pitch"||id=="vowelShift")s.getProperties().set("bipolar",true);
 s.updateText();
}
void styleLabel(juce::Label& l,const juce::String& text){
 l.setText(text,juce::dontSendNotification);l.setJustificationType(juce::Justification::centred);
 l.setFont(juce::Font(juce::FontOptions(10.5f).withStyle("Bold")).withExtraKerningFactor(.08f));
 l.setColour(juce::Label::textColourId,C::dim);
}
struct LinkHandle : juce::Component {
 juce::String id;
 juce::RangedAudioParameter* route;
 LinkHandle(juce::String name,juce::RangedAudioParameter* r):id(std::move(name)),route(r){
  setMouseCursor(juce::MouseCursor::DraggingHandCursor);
 }
 void paint(juce::Graphics& g) override {
  const float axis=route?route->convertFrom0to1(route->getValue()):0.f;
  const auto colour=axis<.5f?C::dim:axis<1.5f?C::cyan:C::pink;
  auto b=getLocalBounds().toFloat().reduced(1.f);
  if(axis>=.5f){g.setColour(colour.withAlpha(.16f));g.fillRoundedRectangle(b,b.getHeight()*.5f);}
  g.setColour(colour.withAlpha(isMouseOver()?1.f:.6f));g.drawRoundedRectangle(b,b.getHeight()*.5f,1.f);
  g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
  g.drawFittedText(axis<.5f?"LINK":axis<1.5f?"LINK X":"LINK Y",getLocalBounds(),juce::Justification::centred,1);
 }
 void mouseEnter(const juce::MouseEvent&) override {repaint();}
 void mouseExit(const juce::MouseEvent&) override {repaint();}
 void mouseDrag(const juce::MouseEvent&) override {
  if(auto* drag=juce::DragAndDropContainer::findParentDragContainerFor(this))
   if(!drag->isDragAndDropActive())drag->startDragging(id,this);
 }
};
}

GrainBrainLookAndFeel::GrainBrainLookAndFeel(){
 setColour(juce::ResizableWindow::backgroundColourId,C::background);
 setColour(juce::Label::textColourId,C::text);
 setColour(juce::Slider::textBoxTextColourId,C::text.withAlpha(.85f));
 setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
 setColour(juce::Slider::textBoxHighlightColourId,C::mint.withAlpha(.3f));
 setColour(juce::TextEditor::backgroundColourId,C::well);
 setColour(juce::TextEditor::textColourId,C::text);
 setColour(juce::TextEditor::focusedOutlineColourId,C::mint);
 setColour(juce::CaretComponent::caretColourId,C::mint);
 setColour(juce::TextButton::textColourOffId,C::text);
 setColour(juce::TextButton::textColourOnId,C::text);
 setColour(juce::TextButton::buttonOnColourId,C::mint);
 setColour(juce::ComboBox::textColourId,C::text);
 setColour(juce::ComboBox::arrowColourId,C::dim);
 setColour(juce::PopupMenu::backgroundColourId,C::panel);
 setColour(juce::PopupMenu::textColourId,C::text);
 setColour(juce::PopupMenu::highlightedBackgroundColourId,C::mint.withAlpha(.18f));
 setColour(juce::PopupMenu::highlightedTextColourId,C::mint);
 setColour(juce::PopupMenu::headerTextColourId,C::dim);
}
void GrainBrainLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& slider){
 const auto bounds=juce::Rectangle<int>(x,y,w,h).toFloat().reduced(3.f);
 const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f;
 const auto c=bounds.getCentre();
 const float track=juce::jmax(3.f,radius*.13f),arcR=radius-track;
 const float angle=start+pos*(end-start);
 const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
 const juce::PathStrokeType stroke(track,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
 juce::Path bg;bg.addCentredArc(c.x,c.y,arcR,arcR,0.f,start,end,true);
 g.setColour(C::well.brighter(.08f));g.strokePath(bg,stroke);
 const float from=bool(slider.getProperties()["bipolar"])?(start+end)*.5f:start;
 if(std::abs(angle-from)>.001f){
  juce::Path value;value.addCentredArc(c.x,c.y,arcR,arcR,0.f,juce::jmin(from,angle),juce::jmax(from,angle),true);
  g.setColour(accent.withAlpha(.16f));g.strokePath(value,juce::PathStrokeType(track*2.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(accent);g.strokePath(value,stroke);
 }
 const float kr=arcR-track*1.5f;
 g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2c3b54),c.x,c.y-kr,juce::Colour(0xff131a27),c.x,c.y+kr,false));
 g.fillEllipse(c.x-kr,c.y-kr,2*kr,2*kr);
 g.setColour(juce::Colours::white.withAlpha(slider.isMouseOverOrDragging()?.22f:.1f));g.drawEllipse(c.x-kr,c.y-kr,2*kr,2*kr,1.f);
 juce::Path pointer;pointer.startNewSubPath(c.getPointOnCircumference(kr*.3f,angle));pointer.lineTo(c.getPointOnCircumference(kr*.82f,angle));
 g.setColour(C::text);g.strokePath(pointer,juce::PathStrokeType(2.2f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void GrainBrainLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool highlighted,bool down){
 const auto b=button.getLocalBounds().toFloat().reduced(.5f);
 const bool on=button.getToggleState();
 const auto accent=button.findColour(juce::TextButton::buttonOnColourId);
 auto fill=on?C::well.interpolatedWith(accent,.22f):C::well;
 if(down)fill=fill.brighter(.2f);else if(highlighted)fill=fill.brighter(.08f);
 g.setColour(fill);g.fillRoundedRectangle(b,6.f);
 g.setColour(on?accent.withAlpha(.9f):highlighted?accent.withAlpha(.5f):C::panelEdge.brighter(.25f));
 g.drawRoundedRectangle(b,6.f,1.f);
}
void GrainBrainLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool){
 auto area=button.getLocalBounds();
 const bool on=button.getToggleState();
 const auto accent=button.findColour(juce::TextButton::buttonOnColourId);
 if(button.getProperties()["led"]){
  auto led=area.removeFromLeft(22).toFloat().withSizeKeepingCentre(7.f,7.f).translated(5.f,0.f);
  if(on){g.setColour(accent.withAlpha(.35f));g.fillEllipse(led.expanded(3.f));}
  g.setColour(on?accent:C::dim.withAlpha(.45f));g.fillEllipse(led);
  area.removeFromRight(8);
 }
 g.setFont(juce::Font(juce::FontOptions(11.5f).withStyle("Bold")).withExtraKerningFactor(.06f));
 g.setColour((on?accent.brighter(.3f):C::text).withMultipliedAlpha(button.isEnabled()?1.f:.4f));
 g.drawFittedText(button.getButtonText(),area,juce::Justification::centred,1);
}
void GrainBrainLookAndFeel::drawComboBox(juce::Graphics& g,int width,int height,bool,int,int,int,int,juce::ComboBox& box){
 const auto b=juce::Rectangle<float>(0,0,float(width),float(height)).reduced(.5f);
 g.setColour(C::well);g.fillRoundedRectangle(b,6.f);
 g.setColour(box.isMouseOver(true)?C::mint.withAlpha(.5f):C::panelEdge.brighter(.25f));g.drawRoundedRectangle(b,6.f,1.f);
 juce::Path arrow;const float ax=float(width)-15.f,ay=float(height)*.5f;
 arrow.startNewSubPath(ax-4.f,ay-2.f);arrow.lineTo(ax,ay+2.f);arrow.lineTo(ax+4.f,ay-2.f);
 g.setColour(C::dim);g.strokePath(arrow,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void GrainBrainLookAndFeel::positionComboBoxText(juce::ComboBox& box,juce::Label& label){
 label.setBounds(6,1,box.getWidth()-28,box.getHeight()-2);label.setFont(getComboBoxFont(box));
}
juce::Label* GrainBrainLookAndFeel::createSliderTextBox(juce::Slider& slider){
 auto* l=LookAndFeel_V4::createSliderTextBox(slider);
 l->setFont(juce::Font(juce::FontOptions(11.5f)));
 l->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
 l->setColour(juce::Label::outlineWhenEditingColourId,C::mint);
 return l;
}

GrainBrainAudioProcessorEditor::GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor& p):AudioProcessorEditor(&p),processor(p),orb(p){
 setLookAndFeel(&lookAndFeel);
 addAndMakeVisible(orb);
 addAndMakeVisible(clearRoutes);
 for(auto* b:{&storeA,&storeB,&recallA,&recallB})addAndMakeVisible(*b);
 storeA.onClick=[this]{processor.getStateInformation(snapshotA);};
 storeB.onClick=[this]{processor.getStateInformation(snapshotB);};
 recallA.onClick=[this]{if(snapshotA.getSize()>0)processor.setStateInformation(snapshotA.getData(),int(snapshotA.getSize()));};
 recallB.onClick=[this]{if(snapshotB.getSize()>0)processor.setStateInformation(snapshotB.getData(),int(snapshotB.getSize()));};
 for(auto* b:{&freezeButton,&recordButton,&playButton,&seqButton,&resetJoyButton})addAndMakeVisible(*b);
 auto led=[](juce::TextButton& b,juce::Colour accent){b.getProperties().set("led",true);b.setColour(juce::TextButton::buttonOnColourId,accent);};
 led(freezeButton,C::cyan);led(recordButton,C::red);led(playButton,C::mint);led(seqButton,C::red);led(lfoSyncButton,C::cyan);
 auto toggle=[this](const char* id){if(auto* par=processor.params.getParameter(id))par->setValueNotifyingHost(par->getValue()>.5f?0.f:1.f);};
 freezeButton.onClick=[toggle]{toggle("freeze");};
 recordButton.onClick=[toggle]{toggle("joyRecord");};
 playButton.onClick=[toggle]{toggle("joyPlay");};
 seqButton.onClick=[toggle]{toggle("seqOn");};
 resetJoyButton.onClick=[this]{for(const auto* id:{"joyX","joyY"})if(auto* p=processor.params.getParameter(id))p->setValueNotifyingHost(.5f);};
 auto setDial=[this](juce::Slider& slider,juce::Label& label,const char* title,const char* id,std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,juce::Colour accent){
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,16);
  slider.setColour(juce::Slider::rotarySliderFillColourId,accent);
  styleLabel(label,title);
  addAndMakeVisible(slider);addAndMakeVisible(label);
  attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,slider);
  formatKnob(slider,id);
 };
 setDial(delayMixKnob,delayLabel,"DELAY MIX","delayMix",delayMixAttachment,C::pink);
 setDial(delayFeedbackKnob,feedbackLabel,"FEEDBACK","delayFeedback",delayFeedbackAttachment,C::pink);
 setDial(seqDepthKnob,seqLabel,"SEQ DEPTH","seqDepth",seqDepthAttachment,C::red);
 for(auto* b:{&randomButton,&randomSafeButton,&lfoSyncButton})addAndMakeVisible(*b);
 randomButton.setColour(juce::TextButton::buttonOnColourId,C::amber);randomSafeButton.setColour(juce::TextButton::buttonOnColourId,C::amber);
 randomAmount.setRange(.05,1.0,.05);randomAmount.setValue(.6,juce::dontSendNotification);
 randomAmount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 randomAmount.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,16);
 randomAmount.setColour(juce::Slider::rotarySliderFillColourId,C::amber);
 randomAmount.textFromValueFunction=[](double v){return juce::String(juce::roundToInt(v*100))+"%";};
 randomAmount.valueFromTextFunction=[](const juce::String& t){return parseValue("chaos",t)/(t.contains("%")?100.:1.);};
 randomAmount.updateText();
 addAndMakeVisible(randomAmount);
 styleLabel(randomLabel,"CHAOS");addAndMakeVisible(randomLabel);
 randomButton.onClick=[this]{processor.randomizePatch(float(randomAmount.getValue()),false);};
 randomSafeButton.onClick=[this]{processor.randomizePatch(float(randomAmount.getValue()),true);};
 lfoSyncButton.onClick=[toggle]{toggle("lfoSync");};
 auto setupCombo=[this](juce::ComboBox& combo,juce::Label& label,const char* title,const char* id,const juce::StringArray& choices){
  for(int i=0;i<choices.size();++i)combo.addItem(choices[i],i+1);
  addAndMakeVisible(combo);
  styleLabel(label,title);addAndMakeVisible(label);
  comboAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.params,id,combo));
 };
 setupCombo(filterModeBox,modeLabel,"TYPE","filterMode",{"Low-pass","High-pass","Band-pass","Notch","Vowel bank"});
 setupCombo(beatDivisionBox,beatLabel,"GLITCH BEAT","seqDivision",{"1/2","1/4","1/8","1/16","1/32","1/64"});
 setupCombo(lfoDivisionBox,lfoBeatLabel,"LFO BEAT","lfoDivision",{"1/2","1/4","1/8","1/16","1/32","1/64"});
 modeLabel.setJustificationType(juce::Justification::centredLeft);
 auto dial=[this](juce::Slider& slider,juce::Label& label,const char* title,const char* id,juce::Colour accent){
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,16);
  slider.setColour(juce::Slider::rotarySliderFillColourId,accent);
  styleLabel(label,title);
  addAndMakeVisible(slider);addAndMakeVisible(label);
  newAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,id,slider));
  formatKnob(slider,id);
 };
 dial(vowelKnob,vowelLabel,"A E I O U","vowel",C::violet);
 dial(vowelShiftKnob,shiftLabel,"VOWEL SHIFT","vowelShift",C::violet);
 dial(timeMorphKnob,timeLabel,"TIME DRIFT","seqTimeMod",C::red);
 dial(filterMixKnob,filterMixLabel,"FILTER MIX","filterMix",C::violet);
 clearRoutes.onClick=[this]{for(int i=0;i<16;++i)if(auto* p=processor.params.getParameter("joyRoute"+juce::String(i)))p->setValueNotifyingHost(0.f);};
 sampleLabel.setText(p.getSampleName(),juce::dontSendNotification);
 sampleLabel.setFont(juce::Font(juce::FontOptions(13.f)));
 sampleLabel.setColour(juce::Label::textColourId,C::mint);
 sampleLabel.setColour(juce::Label::backgroundColourId,C::well);
 sampleLabel.setBorderSize(juce::BorderSize<int>(0,12,0,12));
 addAndMakeVisible(sampleLabel);
 helpLabel.setText(juce::CharPointer_UTF8("Drop a WAV / AIFF anywhere  \xc2\xb7  Drag a LINK tag onto the brain: left half = X, right half = Y  \xc2\xb7  Drag the brain to morph"),juce::dontSendNotification);
 helpLabel.setFont(juce::Font(juce::FontOptions(11.5f)));helpLabel.setJustificationType(juce::Justification::centred);
 helpLabel.setColour(juce::Label::textColourId,C::dim);addAndMakeVisible(helpLabel);
 styleLabel(presetLabel,"PRESET");presetLabel.setJustificationType(juce::Justification::centredRight);addAndMakeVisible(presetLabel);
 addAndMakeVisible(loadButton);addAndMakeVisible(factoryButton);addAndMakeVisible(saveButton);addAndMakeVisible(openButton);addAndMakeVisible(presetBox);
 presetBox.setTextWhenNothingSelected("Choose preset...");
 const char* presetNames[]={
  "Classic / Nebula","Classic / Frozen Cloud","Classic / Glitch Storm","Classic / Deep Space","Classic / Tight Pulses",
  "Motion / Breathing Clouds","Motion / Rhythmic Pulses","Motion / Stereo Drift","Motion / Slow Orbit",
  "Pro / Recursive Orbit","Pro / Polyrhythm Engine","Pro / Fractal Drift","Pro / Interlocked Chaos"
 };
 // Item ids stay at index+2 so applyPreset(id-2) keeps its historic mapping.
 for(int i=0;i<13;++i){
  if(i==0)presetBox.addSectionHeading("Classic");
  if(i==5)presetBox.addSectionHeading("Motion");
  if(i==9)presetBox.addSectionHeading("Pro");
  presetBox.addItem(juce::String(presetNames[i]).fromFirstOccurrenceOf(" / ",false,false),i+2);
 }
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
 const char* names[]={"SIZE","DENSITY","POSITION","SPRAY","PITCH","PITCH RAND","REVERSE","STEREO","GAIN","LFO RATE","LFO DEPTH","CUTOFF","RESONANCE","FILTER TYPE","MOTION","GLITCH"};
 for(int i=0;i<16;++i){
  const auto accent=i<9?C::mint:i<11?C::cyan:i<15?C::violet:C::red;
  auto s=std::make_unique<juce::Slider>();s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,16);
  s->setColour(juce::Slider::rotarySliderFillColourId,accent);
  addAndMakeVisible(*s);
  attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,knobIds[i],*s));
  formatKnob(*s,knobIds[i]);
  sliders.push_back(std::move(s));
  auto l=std::make_unique<juce::Label>();styleLabel(*l,names[i]);addAndMakeVisible(*l);labels.push_back(std::move(l));
  auto handle=std::make_unique<LinkHandle>(knobIds[i],processor.params.getParameter("joyRoute"+juce::String(i)));
  addAndMakeVisible(*handle);
  dragHandles.push_back(std::move(handle));
 }
 // The filter type combo replaces its knob; the hidden knob keeps its attachment and LINK tag.
 sliders[13]->setVisible(false);labels[13]->setVisible(false);
 // Children were added after setLookAndFeel(), so let them rebuild (e.g. slider text boxes) with it.
 sendLookAndFeelChange();
 timerCallback();
 startTimerHz(15);
 setSize(1100,764); // All child controls exist before resized() runs.
}
GrainBrainAudioProcessorEditor::~GrainBrainAudioProcessorEditor(){setLookAndFeel(nullptr);}
void GrainBrainAudioProcessorEditor::timerCallback(){
 auto on=[this](const char* id){auto* v=processor.params.getRawParameterValue(id);return v&&v->load()>.5f;};
 freezeButton.setToggleState(on("freeze"),juce::dontSendNotification);
 recordButton.setToggleState(on("joyRecord"),juce::dontSendNotification);
 playButton.setToggleState(on("joyPlay"),juce::dontSendNotification);
 seqButton.setToggleState(on("seqOn"),juce::dontSendNotification);
 lfoSyncButton.setToggleState(on("lfoSync"),juce::dontSendNotification);
 for(auto& h:dragHandles)h->repaint();
}
juce::Rectangle<int> GrainBrainAudioProcessorEditor::addSection(juce::Rectangle<int> bounds,const juce::String& title,juce::Colour accent){
 sections.push_back({bounds,title,accent});
 return bounds.withTrimmedTop(title.isEmpty()?0:30).reduced(10,4);
}
void GrainBrainAudioProcessorEditor::placeKnob(juce::Slider& slider,juce::Label& label,juce::Component* handle,juce::Rectangle<int> cell){
 label.setBounds(cell.removeFromTop(14));
 auto bottom=cell.removeFromBottom(18);
 if(handle)handle->setBounds(bottom.withSizeKeepingCentre(58,16));
 slider.setBounds(cell);
}
void GrainBrainAudioProcessorEditor::paint(juce::Graphics& g){
 g.setGradientFill(juce::ColourGradient(juce::Colour(0xff111827),0,0,C::background,0,float(getHeight()),false));g.fillAll();
 // Wordmark with a small grain-cloud glyph.
 const float lx=30.f,ly=31.f;
 const float dots[][3]={{0,0,5},{9,-6,3.5f},{10,6,3},{-8,7,2.5f},{-9,-6,2.2f},{17,0,2},{3,-12,1.8f},{2,12,1.6f}};
 for(const auto& d:dots){g.setColour(C::mint.withAlpha(d[2]>3?1.f:.6f));g.fillEllipse(lx+d[0]-d[2],ly+d[1]-d[2],2*d[2],2*d[2]);}
 const auto titleFont=juce::Font(juce::FontOptions(26.f).withStyle("Bold")).withExtraKerningFactor(.04f);
 g.setFont(titleFont);g.setColour(C::text);
 g.drawText("GRAIN",56,10,200,32,juce::Justification::centredLeft);
 const int grainWidth=int(std::ceil(juce::GlyphArrangement::getStringWidth(titleFont,"GRAIN")));
 g.setColour(C::mint);g.drawText("BRAIN",56+grainWidth+2,10,200,32,juce::Justification::centredLeft);
 g.setFont(juce::Font(juce::FontOptions(11.f)).withExtraKerningFactor(.12f));g.setColour(C::dim);
 g.drawText("GRANULAR RANDOMIZER  /  v"+juce::String(JucePlugin_VersionString),57,38,320,16,juce::Justification::centredLeft);
 for(const auto& s:sections){
  const auto b=s.bounds.toFloat();
  g.setColour(C::panel);g.fillRoundedRectangle(b,10.f);
  g.setColour(C::panelEdge);g.drawRoundedRectangle(b.reduced(.5f),10.f,1.f);
  if(s.title.isEmpty())continue;
  g.setColour(s.accent);g.fillRoundedRectangle(b.getX()+12.f,b.getY()+12.f,3.f,11.f,1.5f);
  g.setFont(juce::Font(juce::FontOptions(11.f).withStyle("Bold")).withExtraKerningFactor(.14f));g.setColour(C::text.withAlpha(.9f));
  g.drawText(s.title,int(b.getX())+21,int(b.getY())+9,int(b.getWidth())-30,17,juce::Justification::centredLeft);
 }
 if(fileDragActive){
  g.setColour(C::mint.withAlpha(.08f));g.fillAll();
  g.setColour(C::mint);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.f),12.f,2.f);
 }
}
void GrainBrainAudioProcessorEditor::resized(){
 sections.clear();
 presetBox.setBounds(getWidth()-16-240,16,240,28);presetLabel.setBounds(presetBox.getX()-70,16,62,28);
 // Sample / preset bar.
 auto bar=addSection({16,64,getWidth()-32,44},{},C::mint).withSizeKeepingCentre(getWidth()-52,28);
 loadButton.setBounds(bar.removeFromLeft(112));bar.removeFromLeft(6);
 factoryButton.setBounds(bar.removeFromLeft(112));bar.removeFromLeft(10);
 openButton.setBounds(bar.removeFromRight(106));bar.removeFromRight(6);
 saveButton.setBounds(bar.removeFromRight(106));bar.removeFromRight(16);
 for(auto* b:{&recallB,&storeB,&recallA,&storeA}){b->setBounds(bar.removeFromRight(64));bar.removeFromRight(b==&storeB?12:4);}
 sampleLabel.setBounds(bar);
 const int top=120,mainH=440,lowerY=top+mainH+12,lowerH=148;
 const int leftX=16,leftW=330,midX=358,midW=384,rightX=754,rightW=330;
 // Grain engine: 3 x 3 knobs.
 auto grain=addSection({leftX,top,leftW,mainH},"GRAIN ENGINE",C::mint);
 const int rowH=grain.getHeight()/3,colW=grain.getWidth()/3;
 for(int i=0;i<9;++i){
  auto cell=juce::Rectangle<int>(grain.getX()+(i%3)*colW,grain.getY()+(i/3)*rowH,colW,rowH).withSizeKeepingCentre(colW,106);
  placeKnob(*sliders[i],*labels[i],dragHandles[i].get(),cell);
 }
 // Brain joystick.
 auto joy=addSection({midX,top,midW,mainH},"BRAIN JOYSTICK",C::cyan);
 orb.setBounds(joy.removeFromTop(318));joy.removeFromTop(8);
 auto joyRow=joy.removeFromTop(28);const int bw=(joyRow.getWidth()-18)/4;
 for(auto* b:{&freezeButton,&recordButton,&playButton,&resetJoyButton}){b->setBounds(joyRow.removeFromLeft(bw));joyRow.removeFromLeft(6);}
 joy.removeFromTop(8);clearRoutes.setBounds(joy.removeFromTop(26).withSizeKeepingCentre(140,26));
 // Filter and vowel bank.
 auto filter=addSection({rightX,top,rightW,288},"FILTER + VOWEL",C::violet);
 auto typeRow=filter.removeFromTop(26);
 modeLabel.setBounds(typeRow.removeFromLeft(40));
 dragHandles[13]->setBounds(typeRow.removeFromRight(64).withSizeKeepingCentre(58,16));typeRow.removeFromRight(8);
 filterModeBox.setBounds(typeRow);
 filter.removeFromTop(4);
 const int fw=filter.getWidth()/3;
 juce::Slider* filterKnobs[]={sliders[11].get(),sliders[12].get(),sliders[14].get(),&filterMixKnob,&vowelKnob,&vowelShiftKnob};
 juce::Label* filterLabels[]={labels[11].get(),labels[12].get(),labels[14].get(),&filterMixLabel,&vowelLabel,&shiftLabel};
 juce::Component* filterHandles[]={dragHandles[11].get(),dragHandles[12].get(),dragHandles[14].get(),nullptr,nullptr,nullptr};
 for(int i=0;i<6;++i)placeKnob(*filterKnobs[i],*filterLabels[i],filterHandles[i],{filter.getX()+(i%3)*fw,filter.getY()+(i/3)*112,fw,106});
 // Ambient delay.
 auto delay=addSection({rightX,top+300,rightW,mainH-300},"SPACE DELAY",C::pink);
 placeKnob(delayMixKnob,delayLabel,nullptr,delay.withSizeKeepingCentre(fw*2,106).removeFromLeft(fw));
 placeKnob(delayFeedbackKnob,feedbackLabel,nullptr,delay.withSizeKeepingCentre(fw*2,106).removeFromRight(fw));
 // LFO modulation.
 auto mod=addSection({leftX,lowerY,leftW,lowerH},"MODULATION",C::cyan);
 placeKnob(*sliders[9],*labels[9],dragHandles[9].get(),mod.removeFromLeft(100).withSizeKeepingCentre(100,106));
 placeKnob(*sliders[10],*labels[10],dragHandles[10].get(),mod.removeFromLeft(100).withSizeKeepingCentre(100,106));
 auto modCol=mod.reduced(4,0).withSizeKeepingCentre(mod.getWidth()-8,80);
 lfoSyncButton.setBounds(modCol.removeFromTop(28));modCol.removeFromTop(8);
 lfoBeatLabel.setBounds(modCol.removeFromTop(16));lfoDivisionBox.setBounds(modCol.removeFromTop(26));
 // Glitch sequencer.
 auto glitch=addSection({midX,lowerY,midW,lowerH},"GLITCH SEQUENCER",C::red);
 placeKnob(*sliders[15],*labels[15],dragHandles[15].get(),glitch.removeFromLeft(82).withSizeKeepingCentre(82,106));
 placeKnob(seqDepthKnob,seqLabel,nullptr,glitch.removeFromLeft(82).withSizeKeepingCentre(82,106));
 placeKnob(timeMorphKnob,timeLabel,nullptr,glitch.removeFromLeft(82).withSizeKeepingCentre(82,106));
 auto seqCol=glitch.reduced(4,0).withSizeKeepingCentre(glitch.getWidth()-8,80);
 seqButton.setBounds(seqCol.removeFromTop(28));seqCol.removeFromTop(8);
 beatLabel.setBounds(seqCol.removeFromTop(16));beatDivisionBox.setBounds(seqCol.removeFromTop(26));
 // Patch randomizer.
 auto rnd=addSection({rightX,lowerY,rightW,lowerH},"RANDOMIZER",C::amber);
 placeKnob(randomAmount,randomLabel,nullptr,rnd.removeFromLeft(100).withSizeKeepingCentre(100,106));
 auto rndCol=rnd.reduced(8,0).withSizeKeepingCentre(rnd.getWidth()-16,72);
 randomButton.setBounds(rndCol.removeFromTop(32));rndCol.removeFromTop(8);randomSafeButton.setBounds(rndCol.removeFromTop(32));
 helpLabel.setBounds(16,lowerY+lowerH+6,getWidth()-32,20);
}
bool GrainBrainAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files){
 for(auto& f:files)if(juce::File(f).hasFileExtension("wav;aif;aiff"))return true;return false;
}
void GrainBrainAudioProcessorEditor::filesDropped(const juce::StringArray& files,int,int){
 fileDragActive=false;repaint();
 if(!files.isEmpty()){auto f=juce::File(files[0]);sampleLabel.setText(processor.loadSample(f)?processor.getSampleName():"Invalid audio file",juce::dontSendNotification);}
}

void GrainBrainOrb::paint(juce::Graphics& g){
 auto area=getLocalBounds().toFloat().reduced(12.f);
 const auto cx=area.getCentreX(),cy=area.getCentreY(),r=juce::jmin(area.getWidth(),area.getHeight())*.45f;
 const auto tint=dragHighlight?C::mint:C::cyan;
 g.setGradientFill(juce::ColourGradient(tint.withAlpha(dragHighlight?.3f:.13f),cx,cy,tint.withAlpha(0.f),cx+r*1.3f,cy,true));
 g.fillEllipse(cx-r*1.3f,cy-r*1.3f,2.6f*r,2.6f*r);
 g.setGradientFill(juce::ColourGradient(dragHighlight?juce::Colour(0xff2f6a5e):juce::Colour(0xff26405c),cx-r*.35f,cy-r*.45f,juce::Colour(0xff0d1420),cx+r*.75f,cy+r*.85f,true));
 g.fillEllipse(cx-r,cy-r,2*r,2*r);
 for(int i=1;i<=3;++i){float rr=r*i/4.f;g.setColour(tint.withAlpha(.14f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,1.f);}
 g.setColour(tint.withAlpha(.1f));g.drawLine(cx-r,cy,cx+r,cy,1.f);g.drawLine(cx,cy-r,cx,cy+r,1.f);
 for(int side=-1;side<=1;side+=2)for(int k=0;k<5;++k){juce::Path fold;float yy=cy-51.f+k*25.f;fold.startNewSubPath(cx+side*7.f,yy);fold.cubicTo(cx+side*35.f,yy-21.f,cx+side*73.f,yy+23.f,cx+side*58.f,yy+8.f);g.setColour(C::mint.withAlpha(.28f));g.strokePath(fold,juce::PathStrokeType(1.4f));}
 g.setColour(tint.withAlpha(.45f));g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);
 auto raw=[&](const char* id){return params.getRawParameterValue(id)->load();};
 float x=juce::jlimit(0.f,1.f,raw("joyX"));
 float y=juce::jlimit(0.f,1.f,raw("joyY"));
 float px=cx+(x-.5f)*2.f*r*.84f,py=cy+(.5f-y)*2.f*r*.84f;
 // Show active routing cables, cyan for X and magenta for Y.
 const juce::String routeNames[]={"SIZE","DENSITY","POSITION","SPRAY","PITCH","PITCH RAND","REVERSE","SPREAD","GAIN","LFO RATE","LFO DEPTH","CUTOFF","RESONANCE","TYPE","MOTION","GLITCH"};
 int nx=0,ny=0;
 g.setFont(juce::Font(juce::FontOptions(9.5f).withStyle("Bold")));
 for(int i=0;i<16;++i){
  auto id="joyRoute"+juce::String(i);
  const auto* p=params.getRawParameterValue(id);
  if(!p)continue;
  float route=p->load();
  if(route<.5f)continue;
  const bool xAxis=route<1.5f;
  const int slot=xAxis?nx++:ny++;
  const float yy=22.f+float(slot%8)*20.f;
  const float tx=xAxis?60.f:float(getWidth()-60);
  const float startX=xAxis?cx-r*.7f:cx+r*.7f;
  const auto colour=xAxis?C::cyan:C::pink;
  juce::Path cable;cable.startNewSubPath(startX,cy+(slot%4-2)*11.f);cable.cubicTo(cx+(xAxis?-r:r),yy,tx+(xAxis?40.f:-40.f),yy,tx,yy);
  g.setColour(colour.withAlpha(.55f));
  g.strokePath(cable,juce::PathStrokeType(1.3f));
  g.fillEllipse(tx-2.5f,yy-2.5f,5.f,5.f);
  g.setColour(colour);
  g.drawFittedText(routeNames[i],juce::Rectangle<int>(xAxis?0:getWidth()-56,int(yy-8),56,16),xAxis?juce::Justification::centredLeft:juce::Justification::centredRight,1);
 }
 const auto visual=processor.getGrainVisuals();
 for(const auto& gr:visual)if(gr.energy>.001f){
  const float xx=cx+(gr.position-.5f)*r*1.55f, yy=cy-gr.pan*r*.58f;
  const float a=juce::jlimit(0.f,.85f,gr.energy);
  g.setColour(C::mint.withAlpha(a*.25f));g.fillEllipse(xx-5.f,yy-5.f,10.f,10.f);
  g.setColour(C::mint.withAlpha(a));g.fillEllipse(xx-2.f,yy-2.f,4.f,4.f);
 }
 g.setColour(C::mint.withAlpha(.5f));g.drawLine(cx,cy,px,py,1.5f);
 g.setColour(C::mint.withAlpha(.18f));g.fillEllipse(px-24,py-24,48,48);
 juce::ColourGradient gradient(juce::Colour(0xffc4fff0),px-10,py-12,juce::Colour(0xff1687ab),px+16,py+18,true);
 g.setGradientFill(gradient);g.fillEllipse(px-14,py-14,28,28);
 g.setColour(juce::Colours::white.withAlpha(.9f));g.drawEllipse(px-14,py-14,28,28,1.5f);
 g.setColour(C::dim);g.setFont(juce::Font(juce::FontOptions(10.f)).withExtraKerningFactor(.1f));
 g.drawFittedText("X "+juce::String(juce::roundToInt(x*100))+"   Y "+juce::String(juce::roundToInt(y*100)),getLocalBounds().removeFromBottom(14),juce::Justification::centred,1);
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
 for(const auto* id:knobIds)if(details.description.toString()==id)return true;
 return false;
}
void GrainBrainOrb::itemDropped(const SourceDetails& details){
 dragHighlight=false;
 for(int i=0;i<16;++i)if(details.description.toString()==knobIds[i]){
  if(auto* p=params.getParameter("joyRoute"+juce::String(i))){
   const float axis=details.localPosition.x<getWidth()/2?1.f:2.f;
   const float old=p->convertFrom0to1(p->getValue());
   p->setValueNotifyingHost(p->convertTo0to1(std::abs(old-axis)<.1f?0.f:axis));
  }
  break;
 }
 repaint();
}
