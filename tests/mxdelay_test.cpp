#include "mxdelay_dsp.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace JerzyAudio;

static double energy(const std::vector<float>& a,const std::vector<float>& b,int start=0){
    double e=0.0;
    for(size_t i=(size_t)std::max(0,start);i<a.size();++i)e+=std::abs(a[i])+std::abs(b[i]);
    return e;
}

int main(){
    constexpr int N=48000;
    std::vector<float> inL(N),inR(N),outL(N),outR(N);
    inL[0]=inR[0]=1.0f;
    float* in[2]={inL.data(),inR.data()};
    float* out[2]={outL.data(),outR.data()};
    MXDelayDSP<float> dsp;
    dsp.prepare(48000,2);
    MXDelayParams p;
    p.mix=1.0;p.outputTrim=0.6;p.inputTrim=0.5;
    double peak=0;

    for(int algo=0;algo<kAlgorithmCount;++algo){
        dsp.reset();
        std::fill(outL.begin(),outL.end(),0);std::fill(outR.begin(),outR.end(),0);
        p=MXDelayParams{};
        p.mix=1.0;p.outputTrim=0.6;p.inputTrim=0.5;
        p.slot[0].algorithm=double(algo)/double(kAlgorithmCount-1);
        p.slot[1].enable=0.0;
        p.slot[0].sync=0.0;
        p.slot[0].time=0.06;
        p.slot[0].feedback=0.45;
        dsp.process(in,out,2,N,p,120.0,peak);
        assert(std::isfinite(peak));
        assert(peak<5.0);
        const double e=energy(outL,outR,1);
        assert(e>1e-4);
        std::cout<<"algo "<<algo<<" peak="<<peak<<" energy="<<e<<"\n";
    }

    SlotParams s;
    s.sync=1.0;s.division=4.0/9.0;
    assert(std::abs(slotDelayMs(s,120.0)-500.0)<1e-6);

    // Each MultiHead Reel head has independent pan.
    dsp.reset();
    std::fill(outL.begin(),outL.end(),0);std::fill(outR.begin(),outR.end(),0);
    p=MXDelayParams{};p.mix=1.0;p.slot[0].sync=0.0;p.slot[0].time=0.03;p.slot[1].enable=0.0;
    for(int h=0;h<4;++h){
        p.slot[0].c[(int)DelayAlgorithm::Volante][6+h]=(h==0)?1.0:0.0;
        p.slot[0].c[(int)DelayAlgorithm::Volante][10+h]=0.0;
    }
    p.slot[0].headPan[0]=0.0;
    p.slot[0].c[(int)DelayAlgorithm::Volante][4]=1.0;
    dsp.process(in,out,2,N,p,120.0,peak);
    double eL=0,eR=0;
    for(int i=1;i<N;++i){eL+=std::abs(outL[i]);eR+=std::abs(outR[i]);}
    assert(eL>eR*2.0);

    // A/B must not produce identical mechanical motion or artifact streams.
    dsp.reset();
    std::fill(outL.begin(),outL.end(),0);std::fill(outR.begin(),outR.end(),0);
    p=MXDelayParams{};
    p.mix=1.0;
    p.routing=1.0; // SPLIT L/R
    for(int slot=0;slot<2;++slot){
        p.slot[slot].algorithm=double((int)DelayAlgorithm::ElCapistan)/double(kAlgorithmCount-1);
        p.slot[slot].sync=0.0;
        p.slot[slot].time=0.055;
        p.slot[slot].feedback=0.58;
        auto& c=p.slot[slot].c[(int)DelayAlgorithm::ElCapistan];
        c[0]=0.85;c[1]=0.90;c[2]=0.88;c[3]=0.92;c[4]=0.58;
    }
    dsp.process(in,out,2,N,p,120.0,peak);
    double lrDifference=0.0;
    for(int i=1;i<N;++i)lrDifference+=std::abs(outL[i]-outR[i]);
    assert(lrDifference>1e-3);

    // Spillover bypass must keep audible tails but must not feed new input into the delay.
    constexpr int B=24000;
    std::vector<float> aL(B),aR(B),bL(B),bR(B);
    aL[0]=aR[0]=1.0f;
    float* aIn[2]={aL.data(),aR.data()};
    float* aOut[2]={bL.data(),bR.data()};
    dsp.reset();
    p=MXDelayParams{};
    p.mix=1.0;
    p.slot[0].algorithm=double((int)DelayAlgorithm::DIG)/double(kAlgorithmCount-1);
    p.slot[0].sync=0.0;
    p.slot[0].time=(100.0-1.0)/2499.0;
    p.slot[0].feedback=0.82;
    p.slot[1].enable=0.0;
    dsp.process(aIn,aOut,2,B,p,120.0,peak);

    std::fill(aL.begin(),aL.end(),0.0f);std::fill(aR.begin(),aR.end(),0.0f);
    std::fill(bL.begin(),bL.end(),0.0f);std::fill(bR.begin(),bR.end(),0.0f);
    p.bypass=1.0;p.spill=1.0;
    dsp.process(aIn,aOut,2,B,p,120.0,peak);
    assert(energy(bL,bR)>1e-4);

    // Hard bypass without spill is true dry and clears the delay memory.
    dsp.reset();
    std::fill(aL.begin(),aL.end(),0.0f);std::fill(aR.begin(),aR.end(),0.0f);
    std::fill(bL.begin(),bL.end(),0.0f);std::fill(bR.begin(),bR.end(),0.0f);
    aL[17]=0.37f;aR[17]=-0.22f;
    p=MXDelayParams{};p.bypass=1.0;p.spill=0.0;p.inputTrim=1.0;p.outputTrim=1.0;
    dsp.process(aIn,aOut,2,B,p,120.0,peak);
    assert(std::abs(bL[17]-aL[17])<1e-7);
    assert(std::abs(bR[17]-aR[17])<1e-7);

    // Global Machine Condition must materially change an otherwise identical tape render.
    constexpr int C=48000;
    std::vector<float> cInL(C),cInR(C),servL(C),servR(C),damL(C),damR(C);
    cInL[0]=cInR[0]=1.0f;
    float* cIn[2]={cInL.data(),cInR.data()};
    float* cServ[2]={servL.data(),servR.data()};
    float* cDam[2]={damL.data(),damR.data()};
    p=MXDelayParams{};
    p.mix=1.0;p.slot[1].enable=0.0;p.slot[0].sync=0.0;p.slot[0].time=0.055;p.slot[0].feedback=0.66;
    p.slot[0].algorithm=double((int)DelayAlgorithm::ElCapistan)/double(kAlgorithmCount-1);
    auto& tc=p.slot[0].c[(int)DelayAlgorithm::ElCapistan];
    tc[0]=0.35;tc[1]=0.28;tc[2]=0.22;tc[3]=0.20;tc[4]=0.55;
    p.machineCondition=0.0;
    dsp.reset();dsp.process(cIn,cServ,2,C,p,120.0,peak);
    p.machineCondition=1.0;
    dsp.reset();dsp.process(cIn,cDam,2,C,p,120.0,peak);
    double conditionDifference=0.0;
    for(int i=1;i<C;++i)conditionDifference+=std::abs(servL[i]-damL[i])+std::abs(servR[i]-damR[i]);
    assert(conditionDifference>0.05);


    // MultiHead must produce a stable delayed signal without pathological sample-to-sample crackle.
    {
        constexpr int M=96000;
        std::vector<float> xL(M),xR(M),yL(M),yR(M);
        for(int i=0;i<M;++i){const float v=0.12f*std::sin(2.0*3.14159265358979323846*220.0*i/48000.0);xL[i]=xR[i]=v;}
        float* xi[2]={xL.data(),xR.data()};float* yo[2]={yL.data(),yR.data()};
        p=MXDelayParams{};p.mix=1.0;p.machineCondition=0.0;p.slot[1].enable=0.0;
        p.slot[0].algorithm=double((int)DelayAlgorithm::Volante)/double(kAlgorithmCount-1);
        p.slot[0].sync=0.0;p.slot[0].time=0.70;p.slot[0].feedback=0.45;
        dsp.reset();dsp.process(xi,yo,2,M,p,120.0,peak);
        double e=energy(yL,yR,12000),maxJump=0.0;
        for(int i=12001;i<M;++i)maxJump=std::max(maxJump,std::max(std::abs(double(yL[i]-yL[i-1])),std::abs(double(yR[i]-yR[i-1]))));
        assert(e>1.0);
        assert(std::isfinite(maxJump));
        assert(maxJump<0.75);
    }

    // Four FX slots can run serially, split between INPUT/PRE and WET ONLY.
    {
        constexpr int F=48000;
        std::vector<float> xL(F),xR(F),yL(F),yR(F);
        xL[0]=xR[0]=0.4f;
        float* xi[2]={xL.data(),xR.data()};float* yo[2]={yL.data(),yR.data()};
        p=MXDelayParams{};p.mix=0.7;p.slot[1].enable=0.0;p.slot[0].sync=0.0;p.slot[0].time=0.65;
        const int types[4]={(int)WetFxType::Overdrive,(int)WetFxType::Chorus,(int)WetFxType::LoFi,(int)WetFxType::Flanger};
        for(int i=0;i<4;++i){p.fx[i].type=double(types[i])/double((int)WetFxType::Count-1);p.fx[i].amount=.35+.1*i;p.fx[i].depth=.45;p.fx[i].drive=.30;p.fx[i].route=i<2?0.0:1.0;}
        dsp.reset();dsp.process(xi,yo,2,F,p,120.0,peak);
        assert(std::isfinite(peak));assert(peak<5.0);assert(energy(yL,yR)>1e-3);
    }

    // Automation extremes: feedback at maximum and fast tempo changes.
    {
        constexpr int K=4096;
        std::vector<float> il(K),ir(K),ol(K),orr(K);
        float *input[2]={il.data(),ir.data()},*output[2]={ol.data(),orr.data()};
        p=MXDelayParams{};p.mix=1.0;p.slot[1].enable=0.0;
        p.slot[0].algorithm=double((int)DelayAlgorithm::Volante)/double(kAlgorithmCount-1);
        p.slot[0].sync=1.0;p.slot[0].division=0.5;
        p.slot[0].feedback=1.0;
        dsp.reset();
        double integrated=0.0,maxPeak=0.0;
        for(int block=0;block<180;++block){
            std::fill(il.begin(),il.end(),0.f);
            std::fill(ir.begin(),ir.end(),0.f);
            if(block==0) il[0]=ir[0]=0.5f;
            const double bpm=block<60?75.0:block<120?147.0:95.0;
            dsp.process(input,output,2,K,p,bpm,peak);
            assert(std::isfinite(peak));
            maxPeak=std::max(maxPeak,peak);
            for(int i=0;i<K;++i){
                assert(std::isfinite(ol[i])&&std::isfinite(orr[i]));
                integrated+=std::abs(ol[i])+std::abs(orr[i]);
            }
        }
        assert(integrated>0.01 && maxPeak<8.0);
    }
    return 0;
}
