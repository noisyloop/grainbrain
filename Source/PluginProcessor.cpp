#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
namespace { constexpr double pi = juce::MathConstants<double>::pi; }
juce::AudioProcessorValueTreeState::ParameterLayout GrainBrainAudioProcessor::makeLayout() {
 using P=juce::AudioParameterFloat;
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float def){p.add(std::make_unique<P>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
 add("size","Grain size ms",15,500,110);add("density","Grains per second",1,70,18);add("position","Sample position",0,1,.35f);
 add("spray","Position random",0,1,.18f);add("pitch","Pitch semitones",-24,24,0);add("pitchRand","Pitch random",0,12,1);
 add("reverse","Reverse probability",0,1,.12f);add("spread","Stereo spread",0,1,.8f);add("gain","Output gain",0,1,.65f);
 add("lfoRate","LFO Hz",.02f,15,.4f);add("lfoDepth","LFO position depth",0,1,.2f);
 add("cutoff","Filter cutoff",80,18000,9000);add("resonance","Filter resonance",.1f,1.4f,.3f);
 add("filterMode","Filter type (0 LP 1 HP 2 BP 3 Notch 4 Vowel)",0,4,0);
 add("filterMotion","Filter LFO depth",0,1,0);
 add("vowel","Vowel morph A E I O U",0,4,0);
 add("vowelShift","Vowel shift semitones",-12,12,0);
 add("glitch","Glitch amount",0,1,0);
 add("freeze","Freeze grain position",0,1,0);
 add("delayMix","Ambient delay wet",0,1,0);
 add("delayFeedback","Ambient delay feedback",0,.85f,.35f);
 add("seqOn","Glitch sequencer enabled",0,1,0);
 add("seqDepth","Sequencer amount",0,1,.65f);
 add("seqDivision","Sequencer beat division",0,5,3);
 add("seqTimeMod","Sequencer time modulation",0,1,0);
 add("lfoSync","LFO tempo sync",0,1,0);
 add("lfoDivision","LFO beat division",0,5,2);
 add("joyRecord","Joystick record",0,1,0);
 add("joyPlay","Joystick loop playback",0,1,0);
 add("joyX","Brain Joystick X",0,1,.5f);add("joyY","Brain Joystick Y",0,1,.5f);
 for(int i=0;i<16;++i){juce::String id="joyRoute"+juce::String(i);p.add(std::make_unique<P>(juce::ParameterID{id,1},"Joystick route "+juce::String(i+1),juce::NormalisableRange<float>(0,2,1),0));}
 return p;
}
GrainBrainAudioProcessor::GrainBrainAudioProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PARAMETERS",makeLayout()){useFactoryWave();}
bool GrainBrainAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const{return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
void GrainBrainAudioProcessor::prepareToPlay(double sr,int){
 sampleRateHz=juce::jmax(1.0,sr);
 for(auto& g:grains)g.active=false;
 for(auto& n:notes)n.active=false;
 filterLP={0,0};filterBP={0,0};glitchCounter=0;glitchHeld={0,0};vowelZ1={};vowelZ2={};
 delayLine.setSize(2,juce::jmax(1,int(sr*2.0)));delayLine.clear();delayIndex=0;
 sequencePhase=0;sequenceStep=-1;joyCaptureClock=0;joyPlaybackPosition=0;
}
float GrainBrainAudioProcessor::random01(){return std::generate_canonical<float,24>(rng);}
void GrainBrainAudioProcessor::useFactoryWave(){
 juce::ScopedLock lock(sampleLock);const int n=88200;sample.setSize(1,n);
 for(int i=0;i<n;++i){double t=double(i)/44100.;double env=.55+.45*std::sin(2*pi*.31*t);sample.setSample(0,i,float(env*(.5*std::sin(2*pi*220*t)+.25*std::sin(2*pi*330*t)+.15*std::sin(2*pi*440*t))));}
 sampleName="Factory harmonic drone";
}
bool GrainBrainAudioProcessor::loadSample(const juce::File& f){
 juce::AudioFormatManager fm;fm.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(f));
 if(!reader||reader->sampleRate<=0||reader->lengthInSamples<16||reader->lengthInSamples>reader->sampleRate*180)return false;
 const int count=int(reader->lengthInSamples),chs=juce::jmin(2,int(reader->numChannels));if(chs<1)return false;
 juce::AudioBuffer<float> input(chs,count);if(!reader->read(&input,0,count,0,true,true))return false;
 int target=juce::jmax(16,int(std::round(count*44100.0/reader->sampleRate)));juce::AudioBuffer<float> converted(1,target);
 for(int i=0;i<target;++i){double x=i*reader->sampleRate/44100.0;int a=juce::jlimit(0,count-1,int(x)),b=juce::jmin(a+1,count-1);float v=0;
  for(int c=0;c<chs;++c)v+=juce::jmap(float(x-a),input.getSample(c,a),input.getSample(c,b));converted.setSample(0,i,v/chs);}
 {juce::ScopedLock lock(sampleLock);sample=std::move(converted);sampleName=f.getFileName();for(auto& g:grains)g.active=false;}
 return true;
}
juce::String GrainBrainAudioProcessor::getSampleName() const{juce::ScopedLock lock(const_cast<juce::CriticalSection&>(sampleLock));return sampleName;}
void GrainBrainAudioProcessor::startGrain(Note& note,float position,float spray,float size,float pitch,float pitchRand,float reverse,float spread){
 auto it=std::find_if(grains.begin(),grains.end(),[](const Grain& g){return !g.active;});if(it==grains.end())return;
 auto& g=*it;int n=sample.getNumSamples();if(n<16)return;
 g.length=juce::jmax(16,int(size*.001*sampleRateHz));g.age=0;
 double offset=(random01()*2-1)*spray;g.position=juce::jlimit(0.0,1.0,double(position+offset))*(n-1);
 double semis=(note.number-60)+pitch+(random01()*2-1)*pitchRand;
 g.step=(44100.0/sampleRateHz)*std::pow(2.0,semis/12.0);
 if(random01()<reverse)g.step=-g.step;
 g.amp=note.velocity;g.pan=(random01()*2-1)*spread;g.active=true;
}
void GrainBrainAudioProcessor::processBlock(juce::AudioBuffer<float>& out,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;out.clear();juce::ScopedTryLock lock(sampleLock);if(!lock.isLocked())return;
 auto get=[&](const char* id){return params.getRawParameterValue(id)->load();};
 const char* targets[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch"};
 const float minV[]={15,1,0,0,-24,0,0,0,0,.02f,0,80,.1f,0,0,0};
 const float maxV[]={500,70,1,1,24,12,1,1,1,15,1,18000,1.4f,3,1,1};
 std::array<float,16> baseValues{},routes{};
 for(int k=0;k<16;++k){
  baseValues[k]=get(targets[k]);
  const auto routeName="joyRoute"+juce::String(k);
  routes[k]=get(routeName.toRawUTF8());
 }
 float joyX=get("joyX"),joyY=get("joyY");
 auto effective=[&](int k){
  const float route=routes[k];
  if(route<.5f)return baseValues[k];
  const float axis=route<1.5f?joyX:joyY;
  return juce::jlimit(minV[k],maxV[k],baseValues[k]+(axis-.5f)*(maxV[k]-minV[k])*.85f);
 };

 float size=effective(0),density=effective(1),pos=effective(2),spray=effective(3),pitch=effective(4),pitchRand=effective(5),reverse=effective(6),spread=effective(7),gain=effective(8),rate=effective(9),depth=effective(10);
 float cutoff=effective(11),resonance=effective(12),filterMode=effective(13),filterMotion=effective(14),glitch=effective(15);
 const bool freeze=get("freeze")>.5f,recording=get("joyRecord")>.5f,playing=get("joyPlay")>.5f,sequenceEnabled=get("seqOn")>.5f;
 const float delayWet=get("delayMix"),feedback=get("delayFeedback"),sequenceDepth=get("seqDepth");
 const float division=get("seqDivision"),timeMod=get("seqTimeMod"),vowel=get("vowel"),vowelShift=get("vowelShift");
 const bool lfoSync=get("lfoSync")>.5f;
 const float lfoDivision=get("lfoDivision");
 const float subdivisions[6]={.5f,1.f,2.f,4.f,8.f,16.f};
 if(auto* playHead=getPlayHead()){
  if(auto posInfo=playHead->getPosition()){
   if(auto bpm=posInfo->getBpm())currentBpm=juce::jlimit(30.f,300.f,float(*bpm));
  }
 }
 // Capture XY at 30 Hz while recording, and loop it automatically in audio playback.
 if(recording && !wasRecording){joyRecordedCount=0;joyCaptureClock=0;recordedProgress.store(0);}
 wasRecording=recording;
 if(playing && !wasPlaying)joyPlaybackPosition=0;
 wasPlaying=playing;
 int n=sample.getNumSamples();if(n<16)return;
 juce::MidiBuffer::Iterator events(midi);juce::MidiMessage msg;int eventPos=0;bool hasEvent=events.getNextEvent(msg,eventPos);
 for(int i=0;i<out.getNumSamples();++i){
  while(hasEvent&&eventPos<=i){
   if(msg.isNoteOn()){Note* slot=nullptr;for(auto& x:notes)if(x.active&&x.number==msg.getNoteNumber()){slot=&x;break;}
    if(!slot)for(auto& x:notes)if(!x.active){slot=&x;break;}if(!slot)slot=&notes[0];*slot={msg.getNoteNumber(),msg.getFloatVelocity(),true,0};}
   else if(msg.isNoteOff()){for(auto& x:notes)if(x.number==msg.getNoteNumber())x.active=false;}
   else if(msg.isAllNotesOff()||msg.isAllSoundOff()){for(auto& x:notes)x.active=false;}
   hasEvent=events.getNextEvent(msg,eventPos);
  }
  float playX=joyX,playY=joyY;
  if(recording && joyRecordedCount<(int)joyRecording.size()){
   joyCaptureClock+=30.0/sampleRateHz;
   if(joyCaptureClock>=1.0){joyCaptureClock-=1.0;joyRecording[joyRecordedCount++]={joyX,joyY};recordedProgress.store(float(joyRecordedCount)/float(joyRecording.size()));}
  }
  if(playing && joyRecordedCount>1){
   auto frame=int(joyPlaybackPosition)%joyRecordedCount;
   playX=joyRecording[frame][0];playY=joyRecording[frame][1];
   joyPlaybackPosition+=30.0/sampleRateHz;
   if(joyPlaybackPosition>=joyRecordedCount)joyPlaybackPosition-=joyRecordedCount;
  }
  joyX=playX;joyY=playY;
  // Re-evaluate joystick-routed parameters at audio rate for motion playback.
  size=effective(0);density=effective(1);pos=effective(2);spray=effective(3);
  pitch=effective(4);pitchRand=effective(5);reverse=effective(6);spread=effective(7);
  gain=effective(8);rate=effective(9);depth=effective(10);
  cutoff=effective(11);resonance=effective(12);filterMode=effective(13);
  filterMotion=effective(14);glitch=effective(15);
  float modPos=juce::jlimit(0.f,1.f,pos+float(std::sin(lfoPhase*2*pi))*depth*.5f);
  if(freeze)modPos=float(freezePosition);else freezePosition=modPos;
  const float lfoFrequency=lfoSync?float(currentBpm/60.0)*subdivisions[juce::jlimit(0,5,int(std::round(lfoDivision)))]:rate;
  lfoPhase+=lfoFrequency/sampleRateHz;if(lfoPhase>=1)lfoPhase-=1;
  float seqGlitch=0.f,seqPitch=0.f;
  if(sequenceEnabled){
   constexpr int pattern[16]={1,0,0,1,0,1,0,0,1,1,0,0,1,0,1,0};
   const float shape=std::sin(float(lfoPhase*2*pi*.5));
   const float divisionFactor=subdivisions[juce::jlimit(0,5,int(std::round(division)))];
   const float dynamicRate=divisionFactor*std::pow(2.f,timeMod*shape*2.f);
   sequencePhase+=currentBpm*dynamicRate/(60.0*sampleRateHz);
   if(sequencePhase>=16.0)sequencePhase-=16.0;
   const int newStep=int(sequencePhase);
   if(newStep!=sequenceStep)sequenceStep=newStep;
   if(pattern[newStep]){seqGlitch=sequenceDepth;seqPitch=(newStep%4==0?12.f:0.f)*sequenceDepth;}
  }
  for(auto& note:notes)if(note.active){note.spawnClock+=density/sampleRateHz;if(note.spawnClock>=1){note.spawnClock-=1;startGrain(note,modPos,spray,size,pitch+seqPitch,pitchRand,reverse,spread);}}
  float left=0,right=0;
  for(auto& g:grains)if(g.active){
   double wrapped=std::fmod(g.position,double(n));if(wrapped<0)wrapped+=n;int a=int(wrapped),b=(a+1)%n;
   float s=juce::jmap(float(wrapped-a),sample.getSample(0,a),sample.getSample(0,b));
   float window=.5f-.5f*std::cos(float(2*pi*g.age/juce::jmax(1,g.length-1)));float v=s*window*g.amp;
   left+=v*std::sqrt(.5f*(1-g.pan));right+=v*std::sqrt(.5f*(1+g.pan));g.position+=g.step;if(++g.age>=g.length)g.active=false;
  }
  // The second oscillator follows LFO 1 at a 3:2 ratio. Pro patches move their filter automatically.
  const float motion=std::sin(float(lfoPhase*2*pi*1.5));
  const float hz=juce::jlimit(60.f,float(sampleRateHz*.42),cutoff*std::pow(2.f,filterMotion*motion*4.f));
  const float f=juce::jmin(.65f,2.f*std::sin(float(pi)*hz/float(sampleRateHz))); // Stable SVF range
  const float damping=juce::jlimit(.2f,1.95f,2.f-1.25f*resonance);
  float channels[2]={left,right};
  for(int ch=0;ch<2;++ch){
   const float hp=channels[ch]-filterLP[ch]-damping*filterBP[ch];
   filterBP[ch]+=f*hp;
   filterLP[ch]+=f*filterBP[ch];
   const int mode=juce::jlimit(0,4,int(std::round(filterMode)));
   if(mode==4){
    // Three resonant bands with interpolated formant centres for A/E/I/O/U.
    // Poles remain inside the unit circle; no allocations in the audio callback.
    constexpr float vowels[5][3]={{800,1150,2900},{400,1700,2600},{350,2100,3000},{450,800,2830},{325,700,2530}};
    const int lower=juce::jlimit(0,4,int(vowel)),upper=juce::jmin(4,lower+1);
    const float blend=vowel-float(lower);
    const float shift=std::pow(2.f,vowelShift/12.f);
    float result=0.f;
    for(int band=0;band<3;++band){
     const float center=juce::jlimit(80.f,float(sampleRateHz*.43),
       juce::jmap(blend,vowels[lower][band],vowels[upper][band])*shift);
     const float bandwidth=juce::jmax(70.f,center*(.09f+.12f*(1.f-juce::jlimit(0.f,1.f,resonance))));
     const float radius=std::exp(float(-pi)*bandwidth/float(sampleRateHz));
     const float omega=float(2*pi)*center/float(sampleRateHz);
     const float a1=2.f*radius*std::cos(omega),a2=radius*radius;
     // Equivalent: y[n] = x[n] + 2r cos(w) y[n-1] - r² y[n-2]
     const float output=juce::jlimit(-100.f,100.f,channels[ch]+a1*vowelZ1[ch][band]-a2*vowelZ2[ch][band]);
     vowelZ2[ch][band]=vowelZ1[ch][band];vowelZ1[ch][band]=output;
     result+=output*(1.f-radius)*(.85f/(1.f+float(band)));
    }
    channels[ch]=result;
   }else{
    channels[ch]=mode==0?filterLP[ch]:mode==1?hp:mode==2?filterBP[ch]:hp+filterLP[ch];
   }
  }
  // Sample-and-hold / stutter glitch, deterministic in a held sample period.
  const float liveGlitch=juce::jlimit(0.f,1.f,glitch+seqGlitch);
  if(liveGlitch>.001f){
   const int interval=juce::jmax(2,int(sampleRateHz/(12.f+liveGlitch*125.f)));
   if(++glitchCounter>=interval){glitchCounter=0;glitchHeld={channels[0],channels[1]};}
   const float mix=liveGlitch*.85f;
   channels[0]=channels[0]*(1.f-mix)+glitchHeld[0]*mix;
   channels[1]=channels[1]*(1.f-mix)+glitchHeld[1]*mix;
  }else glitchCounter=0;
  if(delayLine.getNumSamples()>0){
   const int length=delayLine.getNumSamples();
   const int taps=juce::jlimit(1,length-1,int(sampleRateHz*60.0/(currentBpm*2.0)));
   const int read=(delayIndex+length-taps)%length;
   for(int ch=0;ch<2;++ch){
    const float echo=delayLine.getSample(ch,read);
    const float clean=std::tanh(channels[ch]*gain*.35f);
    delayLine.setSample(ch,delayIndex,std::tanh(clean+echo*feedback));
    channels[ch]=clean*(1.f-delayWet)+echo*delayWet;
   }
   if(++delayIndex>=length)delayIndex=0;
  }else for(auto& ch:channels)ch=std::tanh(ch*gain*.35f);
  out.setSample(0,i,channels[0]);out.setSample(1,i,channels[1]);
 }
 for(size_t k=0;k<visualPosition.size();++k){
  const auto& gr=grains[k*8];
  visualPosition[k].store(gr.active?float(gr.position/juce::jmax(1,n)):0.f,std::memory_order_relaxed);
  visualPan[k].store(gr.pan,std::memory_order_relaxed);
  visualEnergy[k].store(gr.active?gr.amp*float(1.0-double(gr.age)/juce::jmax(1,gr.length)):0.f,std::memory_order_relaxed);
 }
}
std::array<GrainBrainAudioProcessor::GrainVisual,32> GrainBrainAudioProcessor::getGrainVisuals() const {
 std::array<GrainVisual,32> visuals{};
 for(size_t i=0;i<visuals.size();++i)visuals[i]={visualPosition[i].load(std::memory_order_relaxed),visualPan[i].load(std::memory_order_relaxed),visualEnergy[i].load(std::memory_order_relaxed)};
 return visuals;
}
float GrainBrainAudioProcessor::getRecordedJoystickProgress() const{return recordedProgress.load();}
void GrainBrainAudioProcessor::applyPreset(int index){
 // Classic (0-4), Motion (5-8), Pro (9-12). All banks are free and editable.
 const float presets[][11]={
  {110,18,.35f,.18f,0,1,.12f,.8f,.65f,.4f,.2f},
  {280,35,.5f,.65f,-12,5,.4f,1,.6f,.15f,.6f},
  {35,60,.25f,.8f,12,9,.65f,1,.45f,4,.5f},
  {420,9,.65f,.2f,-7,.2f,.1f,.9f,.7f,.06f,.7f},
  {75,28,.1f,.05f,0,0,0,.3f,.8f,1.7f,.15f},
  {260,22,.4f,.24f,-5,1,.1f,.85f,.65f,.22f,.7f},
  {95,42,.5f,.12f,0,2,.18f,1,.58f,2.5f,.55f},
  {175,31,.6f,.38f,7,3,.32f,1,.52f,.7f,.85f},
  {310,14,.2f,.5f,-12,4,.45f,.9f,.7f,.08f,.95f},
  {220,45,.48f,.75f,0,6,.5f,1,.6f,1.3f,.9f},
  {62,68,.3f,.55f,12,8,.4f,1,.48f,5.2f,.75f},
  {390,12,.65f,.9f,-19,10,.65f,1,.72f,.035f,1.f},
  {140,54,.15f,.7f,5,12,.8f,1,.5f,8.0f,.95f}
 };
 if(index<0||index>=13)return;
 const char* ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth"};
 for(int i=0;i<11;++i){auto* p=params.getParameter(ids[i]);p->setValueNotifyingHost(p->convertTo0to1(presets[index][i]));}
 const char* extraIds[]={"cutoff","resonance","filterMode","filterMotion","glitch"};
 const float extras[][5]={{14000,.3f,0,0,0},{4800,.5f,0,.15f,0},{9000,.7f,2,.45f,.7f},{1800,.65f,3,.3f,0},{12000,.3f,1,0,.15f},{4500,.6f,0,.65f,0},{7500,.45f,1,.7f,.35f},{3200,.8f,2,.8f,.2f},{1900,.6f,3,.9f,0},{4200,.85f,3,.95f,.3f},{2600,.7f,2,1.f,.7f},{1400,1.f,3,1.f,.45f},{6000,.95f,1,1.f,.9f}};
 for(int i=0;i<5;++i){auto* p=params.getParameter(extraIds[i]);p->setValueNotifyingHost(p->convertTo0to1(extras[index][i]));}
 if(auto* p=params.getParameter("vowel"))p->setValueNotifyingHost(p->convertTo0to1(float(index%5)));
 if(auto* p=params.getParameter("seqDivision"))p->setValueNotifyingHost(p->convertTo0to1(float(index>=9?4:3)));
 if(auto* p=params.getParameter("seqTimeMod"))p->setValueNotifyingHost(p->convertTo0to1(index>=9?.5f:0.f));
 // The advanced factory patches activate the new performance and atmosphere sections.
 const char* fxIds[]={"freeze","delayMix","delayFeedback","seqOn","seqDepth","joyRecord","joyPlay"};
 const float advanced[][5]={
  {0,.0f,.35f,0,.55f},{0,.12f,.38f,0,.45f},{0,.18f,.43f,1,.8f},
  {0,.38f,.55f,0,.5f},{0,.08f,.3f,0,.55f},{0,.3f,.6f,0,.5f},
  {0,.18f,.38f,1,.6f},{0,.4f,.55f,0,.6f},{1,.5f,.68f,0,.5f},
  {0,.48f,.65f,1,.8f},{0,.16f,.48f,1,1.f},{1,.55f,.7f,0,.65f},{0,.28f,.6f,1,1.f}
 };
 for(int i=0;i<7;++i){
  float v=i<5?advanced[index][i]:0.f;
  if(auto* p=params.getParameter(fxIds[i]))p->setValueNotifyingHost(p->convertTo0to1(v));
 }
 // Factory patch joystick routing: Classic stays neutral; Motion and Pro expose performance macros.
 for(int i=0;i<16;++i){
  float axis=0.f;
  if(index>=5&&index<=8){
   if(i==2||i==11)axis=1.f; // X: sample position and filter
   if(i==1||i==15)axis=2.f; // Y: density and glitch
  }else if(index>=9){
   if(i==2||i==3||i==11||i==13)axis=1.f; // X: texture and timbre
   if(i==0||i==1||i==7||i==15)axis=2.f; // Y: clouds, width, stutter
  }
  if(auto* p=params.getParameter("joyRoute"+juce::String(i)))p->setValueNotifyingHost(p->convertTo0to1(axis));
 }
}
bool GrainBrainAudioProcessor::saveUserPreset(const juce::File& file) {
 juce::MemoryBlock data; getStateInformation(data);
 return file.replaceWithData(data.getData(), data.getSize());
}
bool GrainBrainAudioProcessor::loadUserPreset(const juce::File& file) {
 if (!file.existsAsFile() || file.getSize() <= 0 || file.getSize() > 1024 * 1024) return false;
 juce::MemoryBlock data;
 if (!file.loadFileAsData(data)) return false;
 auto xml = std::unique_ptr<juce::XmlElement>(getXmlFromBinary(data.getData(), (int)data.getSize()));
 if (!xml || !xml->hasTagName(params.state.getType())) return false;
 setStateInformation(data.getData(), (int)data.getSize());
 return true;
}
void GrainBrainAudioProcessor::getStateInformation(juce::MemoryBlock& b){auto state=params.copyState();std::unique_ptr<juce::XmlElement> xml(state.createXml());copyXmlToBinary(*xml,b);}
void GrainBrainAudioProcessor::setStateInformation(const void* data,int size){std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size));if(xml&&xml->hasTagName(params.state.getType()))params.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessorEditor* GrainBrainAudioProcessor::createEditor(){return new GrainBrainAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new GrainBrainAudioProcessor();}

void GrainBrainAudioProcessor::randomizePatch(float intensity,bool includeTiming){
 intensity=juce::jlimit(0.f,1.f,intensity);
 juce::Random random(juce::Time::getMillisecondCounter());
 const char* ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread",
  "lfoRate","lfoDepth","cutoff","resonance","filterMode","filterMotion","glitch","vowel","vowelShift",
  "delayMix","delayFeedback","seqDepth","seqDivision","seqTimeMod","lfoSync","lfoDivision"};
 for(int i=0;i<24;++i){
  if(!includeTiming && i>=20)continue;
  if(auto* p=params.getParameter(ids[i])){
   const float old=p->getValue();
   const float novelty=random.nextFloat();
   const float target=juce::jlimit(0.f,1.f,old+(novelty-old)*intensity);
   p->beginChangeGesture();p->setValueNotifyingHost(target);p->endChangeGesture();
  }
 }
}
