#pragma once
#include "PluginProcessor.h"
class GrainBrainOrb : public juce::Component, public juce::DragAndDropTarget, private juce::Timer {
public:
 explicit GrainBrainOrb(juce::AudioProcessorValueTreeState& p):params(p){startTimerHz(30);}
 void paint(juce::Graphics&) override;
 void mouseDown(const juce::MouseEvent& e) override {mouseDrag(e);}
 void mouseDrag(const juce::MouseEvent&) override;
 bool isInterestedInDragSource(const SourceDetails&) override;
 void itemDropped(const SourceDetails&) override;
 void itemDragEnter(const SourceDetails&) override {dragHighlight=true;repaint();}
 void itemDragExit(const SourceDetails&) override {dragHighlight=false;repaint();}
private:
 void timerCallback() override {repaint();}
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
 juce::TextButton clearRoutes{"CLEAR LINKS"};
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
