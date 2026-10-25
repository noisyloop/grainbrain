#include "PluginEditor.h"
GrainBrainAudioProcessorEditor::GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor& p):AudioProcessorEditor(&p),processor(p){
 heading.setText("GRAIN BRAIN / GRANULAR RANDOMIZER",juce::dontSendNotification);
 heading.setFont(juce::Font(juce::FontOptions(23.0f).withStyle("Bold")));
 heading.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(heading);
 sampleLabel.setText(p.getSampleName(),juce::dontSendNotification);sampleLabel.setColour(juce::Label::textColourId,juce::Colours::lightgreen);addAndMakeVisible(sampleLabel);
 helpLabel.setText("Drop WAV / AIFF here  |  Play MIDI notes  |  Select a preset",juce::dontSendNotification);
 helpLabel.setColour(juce::Label::textColourId,juce::Colours::lightgrey);addAndMakeVisible(helpLabel);
 addAndMakeVisible(loadButton);addAndMakeVisible(factoryButton);addAndMakeVisible(presetBox);
 presetBox.addItem("Choose preset...",1);presetBox.addItem("Nebula",2);presetBox.addItem("Frozen Cloud",3);presetBox.addItem("Glitch Storm",4);presetBox.addItem("Deep Space",5);presetBox.addItem("Tight Pulses",6);presetBox.setSelectedId(1,juce::dontSendNotification);
 presetBox.onChange=[this]{int id=presetBox.getSelectedId();if(id>=2)processor.applyPreset(id-2);};
 loadButton.onClick=[this]{chooser=std::make_unique<juce::FileChooser>("Choose a sample",juce::File{},"*.wav;*.aif;*.aiff");
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c){auto f=c.getResult();if(f.existsAsFile())sampleLabel.setText(processor.loadSample(f)?processor.getSampleName():"Could not load sample",juce::dontSendNotification);});};
 factoryButton.onClick=[this]{processor.useFactoryWave();sampleLabel.setText(processor.getSampleName(),juce::dontSendNotification);};
 const char* ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth"};
 const char* names[]={"SIZE ms","DENSITY","POSITION","SPRAY","PITCH","PITCH RAND","REVERSE","STEREO","GAIN","LFO RATE","LFO DEPTH"};
 for(int i=0;i<11;++i){
  auto s=std::make_unique<juce::Slider>();s->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,75,18);
  s->setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff8af0cb));s->setColour(juce::Slider::thumbColourId,juce::Colours::white);
  addAndMakeVisible(*s);
  attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.params,ids[i],*s));
  sliders.push_back(std::move(s));
  auto l=std::make_unique<juce::Label>();l->setText(names[i],juce::dontSendNotification);l->setJustificationType(juce::Justification::centred);
  l->setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(*l);labels.push_back(std::move(l));
 }
 setSize(780,490); // resized() accesses sliders/labels: initialize them first.
}
void GrainBrainAudioProcessorEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff111622));g.setColour(juce::Colour(0xff25384a));
 g.fillRoundedRectangle(16,85,float(getWidth()-32),55,10);
 g.setColour(juce::Colour(0xff283444));g.drawRoundedRectangle(16,85,float(getWidth()-32),55,10,1);
}
void GrainBrainAudioProcessorEditor::resized(){
 heading.setBounds(20,14,550,42);presetBox.setBounds(580,25,180,28);
 loadButton.setBounds(25,96,130,32);factoryButton.setBounds(165,96,130,32);
 sampleLabel.setBounds(310,96,440,32);helpLabel.setBounds(25,450,720,24);
 for(int i=0;i<11;++i){int col=i%6,row=i/6;int x=18+col*125,y=165+row*140;
  sliders[i]->setBounds(x,y,115,105);labels[i]->setBounds(x,y+104,115,24);}
}
bool GrainBrainAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files){
 for(auto& f:files)if(juce::File(f).hasFileExtension("wav;aif;aiff"))return true;return false;
}
void GrainBrainAudioProcessorEditor::filesDropped(const juce::StringArray& files,int,int){
 if(!files.isEmpty()){auto f=juce::File(files[0]);sampleLabel.setText(processor.loadSample(f)?processor.getSampleName():"Invalid audio file",juce::dontSendNotification);}
}
