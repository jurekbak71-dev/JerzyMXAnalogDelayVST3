#pragma once
#include "mxdelay_params.h"
#include <algorithm>
#include <cmath>

namespace JerzyAudio {
constexpr int kMXStateVersion=9;

template<class Stream> bool writeMXState(Stream& b,const MXDelayParams& p){
    if(!b.writeInt32(kMXStateVersion)) return false;
    const double global[]={p.mix,p.inputTrim,p.outputTrim,p.routing,p.bypass,p.spill};
    for(double v:global) if(!b.writeFloat((float)v)) return false;
    if(!b.writeFloat((float)p.machineCondition)) return false;
    for(const auto& fx:p.fx){
        const double values[]={fx.type,fx.amount,fx.rate,fx.depth,fx.drive,fx.route};
        for(double v:values) if(!b.writeFloat((float)v)) return false;
    }
    for(int s=0;s<2;++s){
        const auto& x=p.slot[s];
        const double common[]={x.algorithm,x.enable,x.sync,x.division,x.time,x.feedback,x.level,x.pan,x.duck};
        for(double v:common) if(!b.writeFloat((float)v)) return false;
        for(int a=0;a<kAlgorithmCount;++a) for(int c=0;c<kAlgoControls;++c)
            if(!b.writeFloat((float)x.c[a][c])) return false;
        for(double v:x.headPan) if(!b.writeFloat((float)v)) return false;
    }
    const double rv[]={p.reverb.enable,p.reverb.wet,p.reverb.decay,p.reverb.damping,p.reverb.width,p.reverb.preDelay,p.reverb.freeze,p.reverb.mode,p.reverb.ir};
    for(double v:rv)if(!b.writeFloat((float)v))return false;
    const double extra[]={p.reverb.external,p.reverb.irLength,p.reverb.normalize,p.reverb.lowCut,p.reverb.diffusion,p.reverb.early,p.reverb.hybrid,p.reverb.irDamping};
    for(double v:extra)if(!b.writeFloat(float(v)))return false;
    if(p.reverb.irPath.size()>32768||!b.writeInt32(int32_t(p.reverb.irPath.size())))return false;
    for(unsigned char c:p.reverb.irPath)if(!b.writeInt32(c))return false;
    return true;
}

template<class Stream> bool readMXStateUnchecked(Stream& b,MXDelayParams& p){
    p=MXDelayParams{};
    int32_t version=0; if(!b.readInt32(version)) return false;
    if(version<1 || version>kMXStateVersion) return false;
    auto rd=[&](double& v){ float x=0; if(!b.readFloat(x)||!std::isfinite(x)) return false; v=std::clamp((double)x,0.0,1.0); return true; };
    double* global[]={&p.mix,&p.inputTrim,&p.outputTrim,&p.routing,&p.bypass,&p.spill};
    for(auto* v:global) if(!rd(*v)) return false;
    if(version>=3){ if(!rd(p.machineCondition)) return false; } else p.machineCondition=0.0;
    if(version>=5){
        for(auto& fx:p.fx){
            double* values[]={&fx.type,&fx.amount,&fx.rate,&fx.depth,&fx.drive,&fx.route};
            for(auto* v:values) if(!rd(*v)) return false;
        }
    }else if(version>=4){
        double legacy[6]{};
        for(double& v:legacy) if(!rd(v)) return false;
        p.fx[0].type=legacy[0]; p.fx[0].amount=legacy[1]; p.fx[0].rate=legacy[2];
        p.fx[0].depth=legacy[3]; p.fx[0].drive=legacy[4];
        p.fx[0].route = legacy[5]>.5 ? 0.0 : 1.0;
    }
    for(int s=0;s<2;++s){
        auto& x=p.slot[s];
        double* common[]={&x.algorithm,&x.enable,&x.sync,&x.division,&x.time,&x.feedback,&x.level,&x.pan,&x.duck};
        for(auto* v:common) if(!rd(*v)) return false;
        if(version<4){const int oldIndex=std::clamp((int)std::lround(x.algorithm*6.0),0,6);x.algorithm=double(oldIndex)/double(kAlgorithmCount-1);}
        else if(version<=6){const int oldIndex=std::clamp((int)std::lround(x.algorithm*8.0),0,8);x.algorithm=double(oldIndex)/double(kAlgorithmCount-1);}
        const int storedAlgorithms = version>=7 ? kAlgorithmCount : (version>=4 ? 9 : 7);
        for(int a=0;a<storedAlgorithms;++a) for(int c=0;c<kAlgoControls;++c) if(!rd(x.c[a][c])) return false;
        if(version>=2) for(auto& v:x.headPan) if(!rd(v)) return false;
    }
    if(version>=6){double* rv[]={&p.reverb.enable,&p.reverb.wet,&p.reverb.decay,&p.reverb.damping,&p.reverb.width,&p.reverb.preDelay,&p.reverb.freeze};for(auto* v:rv)if(!rd(*v))return false;}
    if(version>=8){if(!rd(p.reverb.mode)||!rd(p.reverb.ir))return false;}
    if(version>=9){
        double* extra[]={&p.reverb.external,&p.reverb.irLength,&p.reverb.normalize,&p.reverb.lowCut,&p.reverb.diffusion,&p.reverb.early,&p.reverb.hybrid,&p.reverb.irDamping};
        for(auto* v:extra)if(!rd(*v))return false;
        int32_t count=0;if(!b.readInt32(count)||count<0||count>32768)return false;
        p.reverb.irPath.reserve(size_t(count));
        for(int32_t i=0;i<count;++i){int32_t c;if(!b.readInt32(c)||c<=0||c>255)return false;p.reverb.irPath.push_back(char(c));}
    }
    return true;
}
// Transactional load: a truncated or malformed preset never mutates live DSP state.
template<class Stream> bool readMXState(Stream& b,MXDelayParams& p){
    MXDelayParams candidate;
    if(!readMXStateUnchecked(b,candidate))return false;
    p=candidate;
    return true;
}

}
