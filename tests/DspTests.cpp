#include "GuitarDSP.h"
#include <iostream>
#include <stdexcept>
#include <random>
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
int main(){try{
 for(double sr:{44100.,48000.,96000.}) {
  guitar::Engine e;e.prepare(sr);guitar::Settings s;s.reamp=false;s.enabled.fill(false);e.set(s);
  std::vector<float> l(1024),r(1024);for(int i=0;i<1024;++i)l[i]=r[i]=std::sin(i*0.12f)*0.2f;
  auto ref=l;e.process(l.data(),r.data(),1024);
  for(int i=0;i<1024;++i)require(std::abs(l[i]-ref[i])<1e-5f,"Bypass must be unity");
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
 }
 std::cout<<"PASS: unity bypass, meters, 3 colors stability, delay tail and gain at 44.1/48/96 kHz\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
