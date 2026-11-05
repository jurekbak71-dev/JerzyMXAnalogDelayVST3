#pragma once
// Adapted from the original project engines. Each A/B slot owns independent state.
// Memory: JerzyMemoryDelay; Reel: JerzyReelEchoVST3; Oil: JerzyOilDrumEcho.
#include "mxdelay_params.h"
#include "engines/memory/dsp.h"
#include "engines/reel/TapeEngine.h"
#include "engines/oil/Engine.h"
#include <algorithm>
#include <cmath>
namespace JerzyAudio {
template<typename T> class MXOriginalEngines {
    Jerzy::Engine memory;
    jerzy_reel::TapeEngine reel;
    jerzy_oil::Engine oil;
    Jerzy::Parameters memoryParams;
    jerzy_reel::Values reelParams=jerzy_reel::defaults();
    jerzy_oil::Settings oilParams;
    static double clamp01(double v){return std::clamp(std::isfinite(v)?v:0.0,0.0,1.0);}
public:
    void prepare(double sr){memory.prepare(sr);reel.prepare(sr);oil.prepare(sr);}
    void reset(){memory.reset();reel.reset();oil.prepare(sampleRate);}
    double sampleRate=48000;
    void initialize(double sr){sampleRate=sr;prepare(sr);}
    void process(int algorithm,T& l,T& r,const SlotParams& s,double bpm){
        if(s.enable<.5){l=r=T{};return;}
        const double time=slotDelayMs(s,bpm);
        const double fb=clamp01(s.feedback);
        const auto& c=s.c[algorithm];
        if(algorithm==(int)DelayAlgorithm::MemoryDelay){
            memoryParams.set(Jerzy::Blend,1);
            memoryParams.set(Jerzy::Level,clamp01(s.level));
            memoryParams.set(Jerzy::Feedback,fb);
            memoryParams.set(Jerzy::Delay,clamp01((time-20.0)/530.0));
            memoryParams.set(Jerzy::TempoSync,s.sync);
            memoryParams.set(Jerzy::Division,s.division);
            memoryParams.set(Jerzy::Model,c[0]);
            memoryParams.set(Jerzy::Depth,c[1]);
            memoryParams.set(Jerzy::ModRate,c[2]);
            memoryParams.set(Jerzy::Wave,c[3]);
            memoryParams.set(Jerzy::Boost,c[4]);
            memoryParams.set(Jerzy::Squelch,c[5]);
            memoryParams.set(Jerzy::ModEnable,c[6]);
            double in[2]={double(l),double(r)},out[2]{};
            memory.tick(in,out,2,memoryParams,bpm);
            l=T(out[0]);r=T(out[1]);
        }else if(algorithm==(int)DelayAlgorithm::ReelEcho){
            reelParams[jerzy_reel::enabled]=1;
            reelParams[jerzy_reel::wetOnly]=1;
            reelParams[jerzy_reel::level]=float(clamp01(s.level)*100);
            reelParams[jerzy_reel::feedback]=float(fb*95);
            reelParams[jerzy_reel::timeMs]=float(time);
            reelParams[jerzy_reel::sync]=s.sync>=.5?1.f:0.f;
            reelParams[jerzy_reel::division]=float(normIndex(s.division,8));
            reelParams[jerzy_reel::wow]=float(c[0]*100);
            reelParams[jerzy_reel::flutter]=float(c[1]*100);
            reelParams[jerzy_reel::age]=float(c[2]*100);
            reelParams[jerzy_reel::motor]=float((1-c[3])*100);
            reelParams[jerzy_reel::saturation]=float(c[4]*24);
            reelParams[jerzy_reel::dropout]=float(c[5]*100);
            reelParams[jerzy_reel::splice]=float(c[6]*100);
            reelParams[jerzy_reel::hiss]=-100.f;
            reelParams[jerzy_reel::hum]=-100.f;
            reel.set(reelParams,bpm);
            float fl=float(l),fr=float(r);
            reel.process(&fl,&fr,1);
            l=T(fl);r=T(fr);
        }else if(algorithm==(int)DelayAlgorithm::OilDrum){
            using namespace jerzy_oil;
            oilParams.v[key::master]=0;
            oilParams.v[key::time]=float(time);
            oilParams.v[key::sync]=s.sync>=.5?1.f:0.f;
            oilParams.v[key::division]=float(normIndex(s.division,8));
            oilParams.v[key::feedback]=float(fb*.96);
            oilParams.v[key::dry]=0;
            oilParams.v[key::wet]=float(clamp01(s.level));
            oilParams.v[key::oil]=float(c[0]);
            oilParams.v[key::age]=float(c[1]);
            oilParams.v[key::wow]=float(c[2]);
            oilParams.v[key::flutter]=float(c[3]);
            oilParams.v[key::springmix]=float(c[4]);
            oilParams.v[key::springon]=c[4]>.001?1.f:0.f;
            oilParams.v[key::mechmix]=float(c[5]);
            oilParams.v[key::mechanicson]=c[5]>.001?1.f:0.f;
            oilParams.v[key::heads]=c[6]>.5?1.f:0.f;
            float fl=float(l),fr=float(r);
            oil.process(&fl,&fr,1,oilParams,bpm);
            l=T(fl);r=T(fr);
        }
        if(!std::isfinite(double(l)))l=T{};
        if(!std::isfinite(double(r)))r=T{};
    }
};
}
