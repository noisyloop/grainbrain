#pragma once
#include "PluginProcessor.h"
namespace GrainBrainColours {
 const juce::Colour background{0xff0c1018},panel{0xff141c29},panelEdge{0xff223047},well{0xff1c2738};
 const juce::Colour text{0xffe7eef7},dim{0xff8494aa};
 const juce::Colour mint{0xff5ff2c8},cyan{0xff65d8f7},violet{0xffa98af5},amber{0xffffbd74},pink{0xfff38be7},red{0xffff6b7d};
}
class GrainBrainLookAndFeel : public juce::LookAndFeel_V4 {
public:
 GrainBrainLookAndFeel();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
 void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
 void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
 void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
 juce::Font getComboBoxFont(juce::ComboBox&) override {return juce::Font(juce::FontOptions(13.f));}
 juce::Font getPopupMenuFont() override {return juce::Font(juce::FontOptions(14.f));}
 juce::Label* createSliderTextBox(juce::Slider&) override;
};
class GrainBrainOrb : public juce::Component, public juce::DragAndDropTarget, private juce::Timer {
public:
 explicit GrainBrainOrb(GrainBrainAudioProcessor& p):processor(p),params(p.params){startTimerHz(30);}
 void paint(juce::Graphics&) override;
 void mouseDown(const juce::MouseEvent& e) override {mouseDrag(e);}
 void mouseDrag(const juce::MouseEvent&) override;
 bool isInterestedInDragSource(const SourceDetails&) override;
 void itemDropped(const SourceDetails&) override;
 void itemDragEnter(const SourceDetails&) override {dragHighlight=true;repaint();}
 void itemDragExit(const SourceDetails&) override {dragHighlight=false;repaint();}
private:
 void timerCallback() override {repaint();}
 GrainBrainAudioProcessor& processor;
 juce::AudioProcessorValueTreeState& params;
 bool dragHighlight=false;
};
class GrainBrainAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::DragAndDropContainer, private juce::Timer {
public:
 explicit GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor&);
 ~GrainBrainAudioProcessorEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
 void fileDragEnter(const juce::StringArray&,int,int) override {fileDragActive=true;repaint();}
 void fileDragExit(const juce::StringArray&) override {fileDragActive=false;repaint();}
private:
 struct Section {juce::Rectangle<int> bounds;juce::String title;juce::Colour accent;};
 void timerCallback() override;
 void placeKnob(juce::Slider&,juce::Label&,juce::Component*,juce::Rectangle<int>);
 juce::Rectangle<int> addSection(juce::Rectangle<int>,const juce::String&,juce::Colour);
 GrainBrainLookAndFeel lookAndFeel;
 GrainBrainAudioProcessor& processor;
 GrainBrainOrb orb;
 std::vector<Section> sections;
 bool fileDragActive=false;
 juce::TextButton clearRoutes{"CLEAR LINKS"},freezeButton{"FREEZE"},recordButton{"REC XY"},playButton{"LOOP XY"},seqButton{"GLITCH STEPS"},resetJoyButton{"CENTER XY"};
 juce::TextButton storeA{"SAVE A"},storeB{"SAVE B"},recallA{"LOAD A"},recallB{"LOAD B"};
 juce::MemoryBlock snapshotA,snapshotB;
 juce::TextButton randomButton{"RANDOMIZE"},randomSafeButton{"RANDOMIZE + TIME"},lfoSyncButton{"LFO SYNC"};
 juce::ComboBox beatDivisionBox,lfoDivisionBox,filterModeBox;
 juce::Slider randomAmount, timeMorphKnob, vowelKnob, vowelShiftKnob, filterMixKnob;
 juce::Label randomLabel,timeLabel,vowelLabel,shiftLabel,beatLabel,lfoBeatLabel,modeLabel,filterMixLabel;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> newAttachments;
 juce::Slider delayMixKnob,delayFeedbackKnob,seqDepthKnob;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment,delayFeedbackAttachment,seqDepthAttachment;
 juce::Label delayLabel,feedbackLabel,seqLabel;

 juce::Label sampleLabel,helpLabel,presetLabel;
 juce::TextButton loadButton{"LOAD SAMPLE"},factoryButton{"FACTORY WAVE"},saveButton{"SAVE PRESET"},openButton{"OPEN PRESET"};
 juce::ComboBox presetBox;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Component>> dragHandles;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::FileChooser> chooser;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainBrainAudioProcessorEditor)
};
