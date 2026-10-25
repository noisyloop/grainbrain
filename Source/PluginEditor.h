#pragma once
#include "PluginProcessor.h"
class GrainBrainAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget {
public:
 explicit GrainBrainAudioProcessorEditor(GrainBrainAudioProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 bool isInterestedInFileDrag(const juce::StringArray&) override;
 void filesDropped(const juce::StringArray&,int,int) override;
private:
 GrainBrainAudioProcessor& processor;
 juce::Label heading,sampleLabel,helpLabel;
 juce::TextButton loadButton{"LOAD SAMPLE"},factoryButton{"FACTORY WAVE"},saveButton{"SAVE PRESET"},openButton{"OPEN PRESET"};
 juce::ComboBox presetBox;
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::FileChooser> chooser;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainBrainAudioProcessorEditor)
};
