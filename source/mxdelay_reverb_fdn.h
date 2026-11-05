#pragma once
// MX Delay 2: allocation-free stereo algorithmic reverb core.
// This is the algorithmic foundation, NOT an IR/convolution implementation.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace JerzyAudio {
template<typename T> class MXReverbFDN {
    struct Line {
        std::vector<T> buffer;
        std::size_t pos=0;
        double damp=0;
        void prepare(std::size_t n){buffer.assign(std::max<std::size_t>(n,8),T{});pos=0;damp=0;}
        void reset(){std::fill(buffer.begin(),buffer.end(),T{});pos=0;damp=0;}
        double read() const {return buffer.empty()?0.0:double(buffer[pos]);}
        void write(double v){
            if(buffer.empty())return;
            buffer[pos]=T(std::isfinite(v)?std::clamp(v,-8.0,8.0):0.0);
            pos=(pos+1)%buffer.size();
        }
    };
    std::array<Line,8> lines{};
    std::array<Line,2> pre{};
    double sr=48000;
    double wetSm=0,feedbackSm=0;
    static constexpr std::array<double,8> delays{{0.0297,0.0371,0.0411,0.0437,0.0313,0.0359,0.0399,0.0473}};
public:
    struct Params {
        bool enabled=false;
        double wet=0.25, decay=0.6, damping=0.5, width=1.0, preDelayMs=18.0;
        bool freeze=false;
    };
    void prepare(double sampleRate){
        sr=std::clamp(std::isfinite(sampleRate)?sampleRate:48000.0,8000.0,384000.0);
        for(std::size_t i=0;i<lines.size();++i)
            lines[i].prepare(std::size_t(std::ceil(sr*delays[i]))+1);
        for(auto& p:pre)p.prepare(std::size_t(std::ceil(sr*0.25))+2);
        reset();
    }
    void reset(){for(auto& l:lines)l.reset();for(auto& l:pre)l.reset();wetSm=feedbackSm=0;}
    // Reverb processes stereo input and returns dry/wet output. No allocations in process.
    void process(T& left,T& right,const Params& p){
        if(!p.enabled)return;
        const double dryL=std::isfinite(double(left))?double(left):0.0;
        const double dryR=std::isfinite(double(right))?double(right):0.0;
        const double a=1.0-std::exp(-1.0/(0.015*sr));
        wetSm+=a*(std::clamp(p.wet,0.0,1.0)-wetSm);
        const double decay=std::clamp(p.decay,0.0,1.0);
        const double t60=0.35+12.0*decay*decay;
        const double damp=std::clamp(p.damping,0.0,1.0);
        const double inputGain=p.freeze?0.0:0.22;
        const double inL=inputGain*dryL,inR=inputGain*dryR;
        // Predelay is a ring with variable read position; preallocation is at prepare().
        const std::size_t preSamples=std::size_t(std::clamp(p.preDelayMs,0.0,240.0)*sr/1000.0);
        double preOut[2]{};
        const double inputs[2]={inL,inR};
        for(int ch=0;ch<2;++ch){
            auto& l=pre[ch];
            if(l.buffer.empty())continue;
            const auto idx=(l.pos+l.buffer.size()-std::min(preSamples,l.buffer.size()-1))%l.buffer.size();
            preOut[ch]=preSamples?double(l.buffer[idx]):inputs[ch];
            l.write(inputs[ch]);
        }
        double taps[8]{},sum=0;
        for(int i=0;i<8;++i){taps[i]=lines[i].read();sum+=taps[i];}
        const double house=sum*0.25; // Householder feedback: y_i = 2/N * sum(y) - y_i.
        for(int i=0;i<8;++i){
            const double mixed=house-taps[i];
            const double target=p.freeze?0.9995:std::pow(10.0,-3.0*delays[i]/t60);
            const double alpha=1.0-std::exp(-1.0/(sr*(0.0003+0.008*damp)));
            lines[i].damp+=alpha*(mixed-lines[i].damp);
            const double injection=(i%2?preOut[1]:preOut[0])*(i<4?1.0:-1.0);
            lines[i].write(injection+target*((1.0-damp*0.35)*mixed+damp*0.35*lines[i].damp));
        }
        const double l=(taps[0]-taps[1]+taps[2]-taps[3]+taps[4]-taps[5]+taps[6]-taps[7])*0.25;
        const double r=(taps[0]+taps[1]-taps[2]-taps[3]-taps[4]-taps[5]+taps[6]+taps[7])*0.25;
        const double width=std::clamp(p.width,0.0,2.0);
        const double mid=(l+r)*0.5,side=(l-r)*0.5*width;
        left=T((1.0-wetSm)*dryL+wetSm*(mid+side));
        right=T((1.0-wetSm)*dryR+wetSm*(mid-side));
    }
};
} // namespace JerzyAudio
