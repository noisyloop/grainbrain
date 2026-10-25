#pragma once
#include "PluginProcessor.h"
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
class GrainBrainAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::DragAndDropContainer {
public:
 explicit GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
private:
 GrainBrainAudioProcessor& processor;
 GrainBrainOrb orb;
 juce::TextButton clearRoutes{"CLEAR LINKS"},freezeButton{"FREEZE"},recordButton{"REC XY"},playButton{"LOOP XY"},seqButton{"GLITCH STEPS"},resetJoyButton{"CENTER XY"};
 juce::TextButton storeA{"SAVE A"},storeB{"SAVE B"},recallA{"LOAD A"},recallB{"LOAD B"};
 juce::MemoryBlock snapshotA,snapshotB;
 juce::TextButton randomButton{"RANDOMIZE"},randomSafeButton{"RANDOMIZE TIME"},lfoSyncButton{"LFO SYNC"};
 juce::ComboBox beatDivisionBox,lfoDivisionBox,filterModeBox;
 juce::Slider randomAmount, timeMorphKnob, vowelKnob, vowelShiftKnob, filterMixKnob;
 juce::Label randomLabel,timeLabel,vowelLabel,shiftLabel,beatLabel,lfoBeatLabel,modeLabel,filterMixLabel;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> newAttachments;
 juce::Slider delayMixKnob,delayFeedbackKnob,seqDepthKnob;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment,delayFeedbackAttachment,seqDepthAttachment;
 juce::Label delayLabel,feedbackLabel,seqLabel;

 juce::Label heading,sampleLabel,helpLabel;
 juce::TextButton loadButton{"LOAD SAMPLE"},factoryButton{"FACTORY WAVE"},saveButton{"SAVE PRESET"},openButton{"OPEN PRESET"};
 juce::ComboBox presetBox;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Component>> dragHandles;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::FileChooser> chooser;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainBrainAudioProcessorEditor)
};
