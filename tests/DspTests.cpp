#include "GuitarDSP.h"
#include <iostream>
#include <stdexcept>
#include <random>
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
int main(){try{
 require(std::abs(guitar::Engine::delaySeconds(0,120)-.5f)<1e-6f,"Quarter note must follow tempo");
 require(std::abs(guitar::Engine::delaySeconds(1,120)-.375f)<1e-6f,"Edge must be a dotted eighth");
 require(std::abs(guitar::Engine::delaySeconds(2,60)-.5f)<1e-6f,"Eighth note must follow tempo");
 require(std::abs(guitar::Engine::delaySeconds(2,0)-.25f)<1e-6f,"Invalid tempo must use 120 BPM");
 for(double sr:{44100.,48000.,96000.}) {
  guitar::Engine e;e.prepare(sr);guitar::Settings s;s.reamp=false;s.enabled.fill(false);e.set(s);
  std::vector<float> l(static_cast<size_t>(e.latencySamples()+1024)),r(l.size());for(size_t i=0;i<l.size();++i)l[i]=r[i]=std::sin(i*0.12f)*0.2f;
  auto ref=l;e.process(l.data(),r.data(),(int)l.size());
  for(int i=0;i<e.latencySamples();++i)require(l[(size_t)i]==0,"Dry path must have declared latency");
  for(int i=0;i<1024;++i)require(std::abs(l[(size_t)(i+e.latencySamples())]-ref[(size_t)i])<1e-5f,"Bypass must be latency-compensated unity");
  l.resize(1024);r.resize(1024);
  require(e.inputPeak>0.19f && e.outputPeak>0.19f,"Meters must reflect audio");
  s.enabled.fill(true);s.amp=2;s.reamp=true;s.bass=s.treble=s.compress=s.reverb=s.delay=s.pitch=10;
  std::mt19937 rng(42);std::uniform_real_distribution<float> noise(-0.3f,0.3f);
  for(int color=0;color<3;++color){s.color.fill(color);e.set(s);for(int block=0;block<300;++block){for(int i=0;i<1024;++i)l[i]=r[i]=noise(rng);e.process(l.data(),r.data(),1024);for(int i=0;i<1024;++i)require(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<100.f,"Effects must remain finite and stable");}}
  e.prepare(sr);s={};s.reamp=false;s.enabled.fill(false);s.enabled[4]=true;s.delay=10;e.set(s);
  float tail=0;for(int block=0;block<60;++block){std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);if(block==0)l[0]=r[0]=0.5f;e.process(l.data(),r.data(),1024);if(block>2)for(float v:l)tail+=std::abs(v);}
  require(tail>0.01f,"Delay impulse must produce a tail");
  e.prepare(sr);s={};s.reamp=false;s.enabled.fill(false);s.output=-6;e.set(s);
  for(int block=0;block<50;++block){std::fill(l.begin(),l.end(),0.5f);std::fill(r.begin(),r.end(),0.5f);e.process(l.data(),r.data(),1024);}
  require(std::abs(l.back()-0.5f*guitar::gain(-6))<1e-4f,"Output gain must settle correctly");
  s.output=0;s.balance=6;e.set(s);
  for(int block=0;block<50;++block){std::fill(l.begin(),l.end(),.5f);std::fill(r.begin(),r.end(),.5f);e.process(l.data(),r.data(),1024);}
  require(std::abs(l.back()-.5f*guitar::gain(-6))<1e-4f&&std::abs(r.back()-.5f)<1e-4f,"Right balance must attenuate only left channel");
  // Verify actual delayed impulse timing, including the fixed pitch latency.
  for(int color=0;color<3;++color)for(double bpm:{90.,180.}){
   e.prepare(sr);s={};s.enabled.fill(false);s.enabled[4]=true;s.color[4]=color;s.bpm=bpm;e.set(s);
   const int onset=e.latencySamples()+(int)std::floor(guitar::Engine::delaySeconds(color,bpm)*sr);
   std::vector<float> a((size_t)onset+16),b(a.size());a[0]=b[0]=.5f;e.process(a.data(),b.data(),(int)a.size());
   double energy=0;for(int n=onset;n<onset+3;++n)energy+=std::abs(a[(size_t)n]);
   require(energy>.001,"Delay impulse must arrive at the selected musical division");
   for(int n=e.latencySamples()+1;n<onset-1;++n)require(std::abs(a[(size_t)n])<1e-6f,"Delay must not precede the musical division");
  }
 }
 std::cout<<"PASS: latency-aligned unity bypass, meters, 3 colors stability, delay tail and gain at 44.1/48/96 kHz\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
