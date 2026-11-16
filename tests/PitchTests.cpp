#include "GuitarDSP.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <new>
#include <atomic>

#include "AllocationChecks.h"
static void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}

static void tone(double rate,double semi) {
    guitar::PitchShifter p;p.prepare(rate);
    const int count=int(rate*3),begin=int(rate);
    std::vector<float> out(static_cast<size_t>(count));
    float maxStereoError=0, settledStereoError=0, maxDryError=0;
    auto start=std::chrono::steady_clock::now();
    allocations=0;watch=true;
    for(int n=0;n<count;++n){
        const float x=.2f*float(std::sin(2*3.141592653589793*440*n/rate));
        std::array<float,2> wet{},dry{};p.process({x,x},semi,wet,dry);
        out[(size_t)n]=wet[0];if(n>=begin)settledStereoError=std::max(settledStereoError,std::abs(wet[0]-wet[1]));maxStereoError=std::max(maxStereoError,std::abs(wet[0]-wet[1]));
        const float expected=n>=p.latencySamples()?.2f*float(std::sin(2*3.141592653589793*440*(n-p.latencySamples())/rate)):0;
        maxDryError=std::max(maxDryError,std::abs(dry[0]-expected));
    }
    watch=false;
    double cpu=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()/3*100;
    require(allocations==0,"Pitch allocated on the processing thread");
    std::cout<<"stereo_error="<<maxStereoError<<" settled_stereo_error="<<settledStereoError<<std::endl; require(maxStereoError<.002f && settledStereoError<.0002f,"Stereo coherence exceeded 1% startup / 0.1% settled amplitude tolerance");
    require(maxDryError<1e-6,"Dry latency is incorrect");
    std::vector<double> crossings;
    for(int n=begin+1;n<count;++n)if(out[(size_t)n]>=0&&out[(size_t)n-1]<0)crossings.push_back(n-out[(size_t)n]/double(out[(size_t)n]-out[(size_t)n-1]));
    require(crossings.size()>100,"Pitch output has insufficient cycles");
    const double freq=(crossings.size()-1)*rate/(crossings.back()-crossings.front());
    const double expected=440*std::pow(2.,semi/12),cents=1200*std::log2(freq/expected);
    // Residual after removing the measured fundamental includes spurs, noise and modulation.
    // Tuning error is measured independently in cents above.
    double a=0,b=0,energy=0,cc=0,ss=0,cs=0;
    for(int n=begin;n<count;++n){double phase=2*3.141592653589793*freq*n/rate;double x=out[(size_t)n],c=std::cos(phase),s=std::sin(phase);a+=x*c;b+=x*s;energy+=x*x;cc+=c*c;ss+=s*s;cs+=c*s;}
    double determinant=cc*ss-cs*cs,A=(a*ss-b*cs)/determinant,B=(b*cc-a*cs)/determinant;
    double residual=0;
    for(int n=begin;n<count;++n){double phase=2*3.141592653589793*freq*n/rate;double error=out[(size_t)n]-A*std::cos(phase)-B*std::sin(phase);residual+=error*error;}
    double residualDb=10*std::log10(std::max(1e-15,residual/energy));
    std::cout<<"rate="<<rate<<" semi="<<semi<<" Hz="<<freq<<" cents="<<cents<<" residual_dB="<<residualDb<<" latency="<<p.latencySamples()<<" CPU_percent="<<cpu<<" allocations="<<allocations<<'\n';
    require(std::abs(cents)<1,"Pitch error exceeds one cent");
    require(residualDb < -25,"Excessive steady-tone modulation/spurs");
}
static std::vector<float> render(int block){
    guitar::Engine e;e.prepare(48000);guitar::Settings s;s.reamp=false;s.enabled.fill(false);s.enabled[5]=true;s.pitch=10;s.color[5]=1;e.set(s);
    std::vector<float> out(96000),r(96000);
    for(size_t i=0;i<out.size();++i)out[i]=r[i]=.2f*std::sin(float(i)*.0575958653f);
    watch=true;allocations=0;
    for(int n=0;n<(int)out.size();n+=block)e.process(out.data()+n,r.data()+n,std::min(block,(int)out.size()-n));
    watch=false;require(allocations==0,"Engine allocated on the processing thread");return out;
}
static void faderAndTransient() {
    double levels[3]{};int level=0;
    for(float amount:{-10.f,0.f,10.f}){
        guitar::Engine e;e.prepare(48000);guitar::Settings s;s.reamp=false;s.enabled.fill(false);s.enabled[5]=true;s.pitch=amount;e.set(s);
        std::vector<float> l(96000),r(96000);
        for(int n=0;n<96000;++n)l[(size_t)n]=r[(size_t)n]=.2f*std::sin(float(n)*.0575958653f);
        auto input=l;e.process(l.data(),r.data(),96000);double energy=0;
        for(int n=48000;n<96000;++n){double wet=l[(size_t)n]-input[(size_t)(n-e.latencySamples())];energy+=wet*wet;}
        levels[level++]=std::sqrt(energy/48000);
    }
    require(std::abs(levels[0]/levels[1]-guitar::gain(-10))<.001,"Pitch minimum wet gain incorrect");
    require(std::abs(levels[2]/levels[1]-guitar::gain(10))<.001,"Pitch maximum wet gain incorrect");
    for(double rate:{44100.,48000.,96000.})for(double semi:{-12.,0.,12.}){
        guitar::PitchShifter p;p.prepare(rate);int peakSample=0;float peak=0;double energy=0;
        for(int n=0;n<int(rate*.5);++n){std::array<float,2> wet{},dry{};p.process({n==0?.2f:0.f,n==0?.2f:0.f},semi,wet,dry);if(std::abs(wet[0])>peak){peak=std::abs(wet[0]);peakSample=n;}energy+=wet[0]*wet[0];if(semi==0)require(std::abs(dry[0]-(n==p.latencySamples()?.2f:0.f))<1e-7,"Impulse dry latency incorrect");}
        require(peak>0&&peak<.5f&&energy>0&&std::isfinite(energy),"Impulse response unstable");
        if(semi==0)require(std::abs(peakSample-p.latencySamples())<=1,"Unity wet impulse peak differs from latency");
        std::cout<<"Transient rate="<<rate<<" semi="<<semi<<" wet_peak_sample="<<peakSample<<" peak="<<peak<<" energy="<<energy<<'\n';
    }
}
int main(){try{
    for(double rate:{44100.,48000.,96000.})for(double semi:{-12.,0.,12.,12*std::log2(.975),12*std::log2(.95),12*std::log2(.9)})tone(rate,semi);
    faderAndTransient();
    auto reference=render(1);
    for(int block:{17,64,127,256,1024}){auto x=render(block);for(size_t n=0;n<x.size();++n)require(x[n]==reference[n],"DSP depends on host block partition");}
    for(double rate:{44100.,48000.,96000.}){
        guitar::PitchShifter p;p.prepare(rate);float previous=0,maxStep=0;
        watch=true;allocations=0;
        for(int n=0;n<int(rate*3);++n){std::array<float,2> wet{},dry{};double semi=n<int(rate)?-12: n<int(rate*2)?12:12*std::sin(n/rate*2);float x=.2f*float(std::sin(2*3.141592653589793*440*n/rate));p.process({x,x*.7f},semi,wet,dry);require(std::isfinite(wet[0])&&std::isfinite(wet[1]),"Automation produced nonfinite output");maxStep=std::max(maxStep,std::abs(wet[0]-previous));previous=wet[0];}
        watch=false;require(allocations==0,"Automation allocated");require(maxStep<.15f,"Automation has a large sample discontinuity");std::cout<<"Automation rate="<<rate<<" max_sample_step="<<maxStep<<'\n';
    }
    std::cout<<"PASS: octave precision, detune ratios, dry latency, stereo coherence, block invariance and automation; no processing allocations\n";
    return 0;
}catch(const std::exception& e){watch=false;std::cerr<<e.what()<<'\n';return 1;}}
