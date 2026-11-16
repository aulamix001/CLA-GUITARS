#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "PitchShifter.h"

namespace guitar {
constexpr float pi = 3.14159265358979323846f;
inline float gain(float db) { return std::pow(10.0f, db / 20.0f); }
struct Settings {
    float input = 0, output = 0, bass = 0, treble = 0, compress = 0, reverb = 0, delay = 0, pitch = 0;
    bool reamp = true;
    int amp = 0; // Clean, Crunch, Heavy, Bypass
    std::array<int, 6> color {0, 0, 0, 0, 0, 0};
    std::array<bool, 6> enabled {true, true, true, true, true, true};
};
struct Biquad {
    float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void shelf(double sr, float hz, float db, bool high) {
        float A=std::pow(10.0f,db/40), w=2*pi*hz/static_cast<float>(sr), c=std::cos(w), s=std::sin(w);
        float alpha=s*0.70710678f, t=2*std::sqrt(A)*alpha, a0;
        if (!high) {
            b0=A*((A+1)-(A-1)*c+t); b1=2*A*((A-1)-(A+1)*c); b2=A*((A+1)-(A-1)*c-t);
            a0=(A+1)+(A-1)*c+t; a1=-2*((A-1)+(A+1)*c); a2=(A+1)+(A-1)*c-t;
        } else {
            b0=A*((A+1)+(A-1)*c+t); b1=-2*A*((A-1)+(A+1)*c); b2=A*((A+1)+(A-1)*c-t);
            a0=(A+1)-(A-1)*c+t; a1=2*((A-1)-(A+1)*c); a2=(A+1)-(A-1)*c-t;
        }
        b0/=a0; b1/=a0; b2/=a0; a1/=a0; a2/=a0;
    }
    float tick(float x) { float y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; }
};
struct DelayLine {
    std::vector<float> data; size_t head=0;
    void prepare(size_t n) { data.assign(n,0); head=0; }
    float read(float samples) const {
        float pos=static_cast<float>(head)-std::clamp(samples,1.0f,static_cast<float>(data.size()-2));
        if(pos<0) pos+=static_cast<float>(data.size());
        auto i=static_cast<size_t>(pos); float f=pos-static_cast<float>(i);
        return data[i]*(1-f)+data[(i+1)%data.size()]*f;
    }
    void push(float x) { data[head]=x; head=(head+1)%data.size(); }
};
class Engine {
    double sr=48000; std::array<Biquad,2> low,high;
    std::array<DelayLine,2> echoes;
    PitchShifter pitchShifter;
    float pitchMix = 0, smoothing = 0;
    std::array<std::array<DelayLine,4>,2> room;
    std::array<float,2> env{},cab{},echoTone{};
    Settings current{},target{};
    float envelopeAttack=0,envelopeRelease=0;
    int coeffCountdown=0;
public:
    float inputPeak=0,outputPeak=0;
    void prepare(double rate) {
        sr=rate; low={}; high={}; env={}; cab={}; echoTone={}; coeffCountdown=0;
        pitchShifter.prepare(rate); pitchMix=0;
        smoothing=1-std::exp(-1.0f/static_cast<float>(sr*0.01));
        current=target=Settings{}; inputPeak=outputPeak=0;
        envelopeAttack=std::exp(-1.0f/static_cast<float>(sr*0.005));
        envelopeRelease=std::exp(-1.0f/static_cast<float>(sr*0.12));
        for(int c=0;c<2;++c) {
            echoes[c].prepare(static_cast<size_t>(sr*2.1));
            for(int k=0;k<4;++k) room[c][k].prepare(static_cast<size_t>(sr*0.15));
        }
    }
    int latencySamples() const { return pitchShifter.latencySamples(); }
    void set(const Settings& s) { target=s; }
    void process(float* left,float* right,int count) {
        inputPeak=outputPeak=0;
        for(int i=0;i<count;++i) {
            // Smooth continuously automated gains and amounts over about 10 ms.
            float a=smoothing;
            auto smooth=[a](float& x,float t){x+=a*(t-x);};
            smooth(current.input,target.input); smooth(current.output,target.output);
            smooth(current.bass,target.bass); smooth(current.treble,target.treble); smooth(current.compress,target.compress);
            smooth(current.reverb,target.reverb); smooth(current.delay,target.delay); smooth(current.pitch,target.pitch);
            if(--coeffCountdown<=0) {
                constexpr float bassHz[]={160,100,65},trebleHz[]={2500,4000,6500};
                for(int c=0;c<2;++c) {
                    low[c].shelf(sr,bassHz[std::clamp(target.color[0],0,2)],target.enabled[0]?current.bass:0,false);
                    high[c].shelf(sr,trebleHz[std::clamp(target.color[1],0,2)],target.enabled[1]?current.treble:0,true);
                }
                coeffCountdown=32;
            }
            float in[2]={left[i],right[i]};
            std::array<float,2> beforePitch{};
            for(int c=0;c<2;++c) {
                float x=in[c]*gain(current.input); inputPeak=std::max(inputPeak,std::abs(x));
                if(target.reamp && target.amp!=3) {
                    constexpr float drive[]={1.4f,4.0f,11.0f}; float d=drive[std::clamp(target.amp,0,2)];
                    x=std::tanh(x*d)/std::sqrt(d);
                    float k=1-std::exp(-2*pi*5800/static_cast<float>(sr)); cab[c]+=k*(x-cab[c]); x=cab[c];
                }
                x=high[c].tick(low[c].tick(x));
                float e=std::abs(x),coef=e>env[c]?envelopeAttack:envelopeRelease;
                env[c]=coef*env[c]+(1-coef)*e;
                if(target.enabled[2]) {
                    float amount=std::clamp((current.compress+10)/20,0.0f,1.0f);
                    float threshold=-8-22*amount,ratio=2+target.color[2]*2.0f;
                    float over=std::max(0.0f,20*std::log10(std::max(env[c],1e-8f))-threshold);
                    x*=gain(-over*(1-1/ratio)+amount*4);
                }
                constexpr float times[]={0.25f,0.125f,0.0625f};
                float delayMs=times[std::clamp(target.color[4],0,2)]*(c?1.012f:1);
                float echo=echoes[c].read(delayMs*static_cast<float>(sr));
                echoTone[c]+=0.25f*(echo-echoTone[c]);
                echoes[c].push(x+(target.enabled[4]?echoTone[c]*0.32f:0));
                if(target.enabled[4]) x+=echo*gain(current.delay)*0.18f;
                float wet=0;
                constexpr float lengths[]={0.0297f,0.0371f,0.0411f,0.0437f};
                for(int k=0;k<4;++k) {
                    float size=1.0f+target.color[3]*0.35f;
                    float v=room[c][k].read(lengths[k]*size*(c?1.033f:1)*static_cast<float>(sr));
                    room[c][k].push(x+(target.enabled[3]?v*(0.68f+0.06f*target.color[3]):0)); wet+=v*0.25f;
                }
                if(target.enabled[3]) x+=wet*gain(current.reverb)*0.22f;
                beforePitch[static_cast<size_t>(c)]=x;
            }
            // Preserve the original fader's wet-gain semantics and Color detune ratios.
            // Fixed latency also applies when disabled: no timeline jumps on automation.
            constexpr double ratios[]={0.975,0.95,0.9};
            std::array<float,2> wet{},dry{};
            pitchShifter.process(beforePitch,12.0*std::log2(ratios[std::clamp(target.color[5],0,2)]),wet,dry);
            smooth(pitchMix,target.enabled[5]?gain(current.pitch)*0.18f:0.f);
            for(int c=0;c<2;++c) {
                float x=(dry[static_cast<size_t>(c)]+wet[static_cast<size_t>(c)]*pitchMix)*gain(current.output);
                if(!std::isfinite(x))x=0;
                outputPeak=std::max(outputPeak,std::abs(x));
                (c?right:left)[i]=x;
            }
        }
    }
};
}
