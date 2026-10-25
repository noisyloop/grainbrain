#pragma once
#include <JuceHeader.h>
#include <array>
#include <random>
class GrainBrainAudioProcessor : public juce::AudioProcessor {
public:
 GrainBrainAudioProcessor();
 void prepareToPlay(double, int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "GrainBrain"; }
 bool acceptsMidi() const override { return true; }
 bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; }
 double getTailLengthSeconds() const override { return 2.0; }
 int getNumPrograms() override { return 1; }
 int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 bool loadSample(const juce::File&);
 void useFactoryWave();
 void applyPreset(int);
 bool saveUserPreset(const juce::File&);
 bool loadUserPreset(const juce::File&);
 juce::AudioProcessorValueTreeState params;
 juce::String getSampleName() const;
 struct GrainVisual {float position=0,pan=0,energy=0;};
 std::array<GrainVisual,32> getGrainVisuals() const;
 float getRecordedJoystickProgress() const;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
 struct Grain { double position=0, step=1; int age=0, length=0; float amp=0, pan=0; bool active=false; };
 struct Note { int number=-1; float velocity=0; bool active=false; double spawnClock=0; };
 std::array<Grain,256> grains{};
 std::array<std::atomic<float>,32> visualPosition{}, visualPan{}, visualEnergy{};
 std::array<Note,16> notes{};
 juce::AudioBuffer<float> sample;
 juce::CriticalSection sampleLock;
 juce::String sampleName="Factory sine";
 double sampleRateHz=44100, lfoPhase=0;
 std::array<float,2> filterLP{{0,0}},filterBP{{0,0}},glitchHeld{{0,0}};
 int glitchCounter=0;
 double freezePosition=.35, sequencePhase=0, joyCaptureClock=0, joyPlaybackPosition=0;
 int sequenceStep=-1, joyRecordedCount=0;
 std::array<std::array<float,2>,1024> joyRecording{};
 std::array<float,2> delayFeedback{{0,0}};
 juce::AudioBuffer<float> delayLine;
 int delayIndex=0;
 float currentBpm=120.f;
 std::atomic<float> recordedProgress{0};
 std::mt19937 rng{0xC0FFEE};
 float random01();
 void startGrain(Note&,float,float,float,float,float,float,float);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainBrainAudioProcessor)
};
