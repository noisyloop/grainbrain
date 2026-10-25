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
 return p;
}
GrainBrainAudioProcessor::GrainBrainAudioProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PARAMETERS",makeLayout()){useFactoryWave();}
bool GrainBrainAudioProcessor::isBusesLayoutSupported(const BusesLayout& b) const{return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
void GrainBrainAudioProcessor::prepareToPlay(double sr,int){sampleRateHz=sr;for(auto& g:grains)g.active=false;for(auto& n:notes)n.active=false;}
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
void GrainBrainAudioProcessor::startGrain(Note& note,float position,float spray,float size,float pitch,float pitchRand){
 auto it=std::find_if(grains.begin(),grains.end(),[](const Grain& g){return !g.active;});if(it==grains.end())return;
 auto& g=*it;int n=sample.getNumSamples();if(n<16)return;
 g.length=juce::jmax(16,int(size*.001*sampleRateHz));g.age=0;
 double offset=(random01()*2-1)*spray;g.position=juce::jlimit(0.0,1.0,double(position+offset))*(n-1);
 double semis=(note.number-60)+pitch+(random01()*2-1)*pitchRand;
 g.step=(44100.0/sampleRateHz)*std::pow(2.0,semis/12.0);
 if(random01()<params.getRawParameterValue("reverse")->load())g.step=-g.step;
 g.amp=note.velocity;g.pan=(random01()*2-1)*params.getRawParameterValue("spread")->load();g.active=true;
}
void GrainBrainAudioProcessor::processBlock(juce::AudioBuffer<float>& out,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;out.clear();juce::ScopedTryLock lock(sampleLock);if(!lock.isLocked())return;
 auto get=[&](const char* id){return params.getRawParameterValue(id)->load();};
 float size=get("size"),density=get("density"),pos=get("position"),spray=get("spray"),pitch=get("pitch"),pitchRand=get("pitchRand"),gain=get("gain"),rate=get("lfoRate"),depth=get("lfoDepth");
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
  float modPos=juce::jlimit(0.f,1.f,pos+float(std::sin(lfoPhase*2*pi))*depth*.5f);lfoPhase+=rate/sampleRateHz;if(lfoPhase>=1)lfoPhase-=1;
  for(auto& note:notes)if(note.active){note.spawnClock+=density/sampleRateHz;if(note.spawnClock>=1){note.spawnClock-=1;startGrain(note,modPos,spray,size,pitch,pitchRand);}}
  float left=0,right=0;
  for(auto& g:grains)if(g.active){
   double wrapped=std::fmod(g.position,double(n));if(wrapped<0)wrapped+=n;int a=int(wrapped),b=(a+1)%n;
   float s=juce::jmap(float(wrapped-a),sample.getSample(0,a),sample.getSample(0,b));
   float window=.5f-.5f*std::cos(float(2*pi*g.age/juce::jmax(1,g.length-1)));float v=s*window*g.amp;
   left+=v*std::sqrt(.5f*(1-g.pan));right+=v*std::sqrt(.5f*(1+g.pan));g.position+=g.step;if(++g.age>=g.length)g.active=false;
  }
  out.setSample(0,i,std::tanh(left*gain*.35f));out.setSample(1,i,std::tanh(right*gain*.35f));
 }
}
void GrainBrainAudioProcessor::applyPreset(int index){
 const float presets[][11]={{110,18,.35f,.18f,0,1,.12f,.8f,.65f,.4f,.2f},{280,35,.5f,.65f,-12,5,.4f,1,.6f,.15f,.6f},{35,60,.25f,.8f,12,9,.65f,1,.45f,4,.5f},{420,9,.65f,.2f,-7,.2f,.1f,.9f,.7f,.06f,.7f},{75,28,.1f,.05f,0,0,0,.3f,.8f,1.7f,.15f}};
 if(index<0||index>=5)return;const char* ids[]={"size","density","position","spray","pitch","pitchRand","reverse","spread","gain","lfoRate","lfoDepth"};
 for(int i=0;i<11;++i){auto* p=params.getParameter(ids[i]);p->setValueNotifyingHost(p->convertTo0to1(presets[index][i]));}
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
