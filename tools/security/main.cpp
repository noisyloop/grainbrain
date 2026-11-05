// GrainBrain adversarial-input harness. Each case is something a host session, preset file
// or dropped sample could hand the plugin. Build with ASan/UBSan (see CMakeLists.txt).
#include <JuceHeader.h>
#include "PluginProcessor.h"
static int run(GrainBrainAudioProcessor& p,int blocks){
 juce::AudioBuffer<float> buf(2,512);int bad=0;
 for(int b=0;b<blocks;++b){juce::MidiBuffer m;if(b==0)for(int n:{48,60,72})m.addEvent(juce::MidiMessage::noteOn(1,n,(juce::uint8)110),0);
  p.processBlock(buf,m);for(int c=0;c<2;++c)for(int i=0;i<512;++i)if(!std::isfinite(buf.getSample(c,i)))++bad;}
 return bad;
}
static juce::MemoryBlock stateWith(GrainBrainAudioProcessor& p,const char* value,const juce::StringArray& ids){
 juce::MemoryBlock mb;p.getStateInformation(mb);auto xml=juce::AudioProcessor::getXmlFromBinary(mb.getData(),(int)mb.getSize());
 for(auto* c:xml->getChildIterator())if(ids.contains(c->getStringAttribute("id")))c->setAttribute("value",value);
 juce::MemoryBlock out;juce::AudioProcessor::copyXmlToBinary(*xml,out);return out;
}
static juce::File wav(const juce::File& dir,const char* name,juce::uint32 rate,juce::uint32 dataBytes,int actualFrames){
 juce::MemoryOutputStream s;auto u32=[&](juce::uint32 v){s.writeInt((int)v);};auto u16=[&](int v){s.writeShort((short)v);};
 s.write("RIFF",4);u32(36+dataBytes);s.write("WAVE",4);s.write("fmt ",4);u32(16);u16(1);u16(1);u32(rate);u32(rate*2);u16(2);u16(16);
 s.write("data",4);u32(dataBytes);for(int i=0;i<actualFrames;++i)u16((i*37)%2000-1000);
 auto f=dir.getChildFile(name);f.replaceWithData(s.getData(),s.getDataSize());return f;
}
int main(){
 juce::ScopedJuceInitialiser_GUI init;int failures=0;
 const juce::StringArray all{"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","filterMix","vowel","vowelShift","glitch","delayMix","delayFeedback","seqDepth","seqDivision","seqTimeMod","lfoDivision","joyX","joyY"};
 for(const char* v:{"nan","inf","-inf","1e38"}){
  GrainBrainAudioProcessor p;p.prepareToPlay(48000,512);p.applyPreset(12);
  if(auto* s=p.params.getParameter("seqOn"))s->setValueNotifyingHost(1);
  auto st=stateWith(p,v,all);p.setStateInformation(st.getData(),(int)st.getSize());
  int bad=run(p,60);std::cout<<"state value="<<v<<" non-finite samples="<<bad<<"\n";failures+=bad>0;
 }
 {GrainBrainAudioProcessor p;p.prepareToPlay(48000,512);const char junk[]="VC2!\xff\xff\xff\x7f garbage";
  p.setStateInformation(junk,sizeof(junk));p.setStateInformation(nullptr,0);std::cout<<"junk state ok\n";}
 auto dir=juce::File::createTempFile("gbwav");dir.createDirectory();
 struct Case{const char* name;juce::uint32 rate,bytes;int frames;bool expect;};
 for(auto c:{Case{"ok.wav",44100,44100*2,44100,true},Case{"hugeRate.wav",4000000000u,0x7ffffff0u,64,false},Case{"tinyRate.wav",1,4000,2000,false},Case{"lyingLength.wav",48000,0x7ffffff0u,64,false}}){
  GrainBrainAudioProcessor p;p.prepareToPlay(48000,512);bool ok=p.loadSample(wav(dir,c.name,c.rate,c.bytes,c.frames));
  int bad=run(p,10);std::cout<<c.name<<" loaded="<<ok<<" non-finite="<<bad<<"\n";if(ok&&!c.expect)std::cout<<"  (accepted; JUCE clamps length to file size)\n";failures+=bad>0;
 }
 dir.deleteRecursively();std::cout<<(failures?"FAIL":"PASS")<<"\n";return failures?1:0;
}
