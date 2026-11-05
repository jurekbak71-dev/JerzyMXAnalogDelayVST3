#pragma once
// True zero-latency stereo FIR convolution. IRs are deterministic built-in
// synthetic responses; external WAV IR loading is not implemented.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace JerzyAudio {
template<typename T, int N=512> class MXReverbConvolution {
    std::array<T,N> ringL{},ringR{};
    std::array<std::array<double,N>,3> impulse{};
    int pos=0;
    static double noise(uint32_t& state){
        state^=state<<13;state^=state>>17;state^=state<<5;
        return (double(state)/4294967295.0)*2.0-1.0;
    }
public:
    void prepare(double sampleRate){
        const double sr=std::clamp(sampleRate,8000.0,384000.0);
        const double seconds[3]={.18,.35,.52};
        for(int preset=0;preset<3;++preset){
            uint32_t seed=0x935c71u+uint32_t(preset)*7919u;
            double sum=0;
            for(int i=0;i<N;++i){
                const double t=double(i)/sr;
                const double env=std::exp(-6.91*t/seconds[preset]);
                const double early=(i==0?.35:0.0)+(i==int(sr*.003)?0.2:0.0)+(i==int(sr*.011)?0.13:0.0);
                double v=env*noise(seed)*.07+early;
                if(preset==1)v*=.7+.3*std::cos(2.0*3.141592653589793*t*600);
                impulse[preset][i]=v;
                sum+=std::abs(v);
            }
            const double scale=1.0/std::max(1.0,sum*.17);
            for(auto& v:impulse[preset])v*=scale;
        }
        reset();
    }
    void reset(){ringL.fill(T{});ringR.fill(T{});pos=0;}
    void process(T& l,T& r,int preset){
        const auto& ir=impulse[std::clamp(preset,0,2)];
        ringL[pos]=l;ringR[pos]=r;
        double a=0,b=0;
        int idx=pos;
        for(int i=0;i<N;++i){
            a+=double(ringL[idx])*ir[i];
            b+=double(ringR[idx])*ir[i];
            if(--idx<0)idx=N-1;
        }
        if(++pos==N)pos=0;
        l=T(std::isfinite(a)?std::clamp(a,-8.0,8.0):0.0);
        r=T(std::isfinite(b)?std::clamp(b,-8.0,8.0):0.0);
    }
};
}
