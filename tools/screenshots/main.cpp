// Renders the GrainBrain editor offscreen at 2x with a few grains playing, for the README screenshots.
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
static void setParam(GrainBrainAudioProcessor& p,const char* id,float v){if(auto* x=p.params.getParameter(id))x->setValueNotifyingHost(x->convertTo0to1(v));}
static void play(GrainBrainAudioProcessor& p,std::initializer_list<int> notes,int blocks){
 juce::AudioBuffer<float> buf(2,512);
 for(int b=0;b<blocks;++b){juce::MidiBuffer m;if(b==0)for(int n:notes)m.addEvent(juce::MidiMessage::noteOn(1,n,(juce::uint8)100),0);buf.clear();p.processBlock(buf,m);}
}
static void shoot(GrainBrainAudioProcessor& p,const juce::File& out){
 std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
 auto img=ed->createComponentSnapshot(ed->getLocalBounds(),true,2.0f);
 out.deleteFile();juce::FileOutputStream os(out);juce::PNGImageFormat().writeImageToStream(img,os);
 std::cout<<"wrote "<<out.getFullPathName()<<"\n";
}
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 juce::File dir(argv[1]);dir.createDirectory();
 {GrainBrainAudioProcessor p;p.prepareToPlay(48000,512);
  p.applyPreset(0);setParam(p,"joyX",.38f);setParam(p,"joyY",.64f);play(p,{48,55,60,64},40);
  shoot(p,dir.getChildFile("grainbrain-main.png"));}
 {GrainBrainAudioProcessor p;p.prepareToPlay(48000,512);
  p.applyPreset(12);setParam(p,"joyX",.72f);setParam(p,"joyY",.3f);setParam(p,"seqOn",1);setParam(p,"lfoSync",1);setParam(p,"filterMode",4);setParam(p,"vowel",1.5f);setParam(p,"delayMix",.35f);
  play(p,{45,52,57,64,69},40);
  shoot(p,dir.getChildFile("grainbrain-modulation.png"));}
 return 0;
}
