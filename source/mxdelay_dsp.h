#pragma once
#include "mxdelay_params.h"
#include "mxdelay_mechanics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace JerzyAudio {

template<class T> class MXDelayDSP {
    static constexpr double pi=3.14159265358979323846;
    struct Stereo { T l{},r{}; };

    struct SlotDSP {
        double sr=44100.0;
        std::vector<T> l,r;
        size_t w=0;
        T lpL{},lpR{},hpMemL{},hpMemR{},wetLpL{},wetLpR{},duckEnv{},admPredL{},admPredR{};
        T tapeMemL{},tapeMemR{},tubeEnvL{},tubeEnvR{};
        double phase1=0.0,phase2=0.0,clockPhase=0.0;
        double admStepL=1.0/128.0,admStepR=1.0/128.0;
        double delayMsSm=-1.0,feedbackSm=-1.0,levelSm=-1.0;
        double timeSmoothA=1.0,controlSmoothA=1.0;
        bool enabledLast=true;
        uint32_t seed=0x12345678u;
        AnalogRng noiseRng;
        MechanicalTransport transport;

        void prepare(double sampleRate,uint32_t slotSeed){
            sr=std::max(8000.0,sampleRate);
            seed=slotSeed?slotSeed:0x12345678u;
            const size_t n=(size_t)std::ceil(sr*6.0)+16;
            l.assign(n,T{});r.assign(n,T{});
            timeSmoothA=1.0-std::exp(-1.0/(0.035*sr));
            controlSmoothA=1.0-std::exp(-1.0/(0.008*sr));
            transport.prepare(sr,seed^0x8f6c2a1du);
            reset();
        }
        void clearAudio(){
            std::fill(l.begin(),l.end(),T{});
            std::fill(r.begin(),r.end(),T{});
            w=0;
            lpL=lpR=hpMemL=hpMemR=wetLpL=wetLpR=duckEnv=admPredL=admPredR=T{};
            tapeMemL=tapeMemR=tubeEnvL=tubeEnvR=T{};
            phase1=phase2=clockPhase=0.0;
            admStepL=admStepR=1.0/128.0;
            delayMsSm=feedbackSm=levelSm=-1.0;
            noiseRng.seed(seed^0xc2b2ae35u);
            transport.reset();
        }
        void reset(){clearAudio();enabledLast=true;}

        static T sat(T x,double drive=1.0,double asym=0.0){
            const double v=(double)x*drive+asym*0.08;
            const double y=std::tanh(v)-std::tanh(asym*0.08);
            return(T)(y/std::max(0.25,std::tanh(std::max(0.25,drive))));
        }
        T tapeSat(T x,double drive,double bias,double age,T& mem){
            const double xv=double(x);
            const double a=1.0-std::exp(-1.0/(std::max(0.001,0.010+0.035*age)*sr));
            mem=(T)(double(mem)+a*(xv-double(mem)));
            const double hysteresis=xv+(0.05+0.20*age)*double(mem);
            const double asym=(std::clamp(bias,0.0,1.0)*2.0-1.0)*(0.10+0.24*age);
            return sat((T)hysteresis,1.0+2.8*drive+0.8*age,asym);
        }
        T tubeStage(T x,double pre,double record,double bias,T& env){
            const double av=std::abs(double(x));
            const double a=1.0-std::exp(-1.0/(0.025*sr));
            env=(T)(double(env)+a*(av-double(env)));
            const double drive=1.0+3.4*pre+2.2*record;
            const double sag=1.0/(1.0+(0.25+0.75*pre)*double(env)*2.8);
            const double shaped=double(x)*(0.72+0.28*sag);
            return sat((T)shaped,drive,(bias*2.0-1.0)*0.32);
        }
        static T compandCompress(T x,double amount){
            const double a=std::clamp(amount,0.0,1.0);
            const double v=double(x);
            const double mag=std::abs(v);
            const double c=mag/(1.0+(1.8+5.0*a)*mag);
            return(T)std::copysign((1.0-a)*mag+a*c*(1.8+3.2*a),v);
        }
        static T compandExpand(T x,double amount){
            const double a=std::clamp(amount,0.0,1.0);
            const double v=double(x);
            const double mag=std::abs(v);
            const double e=mag*(1.0+(0.35+1.6*a)*mag);
            return(T)std::copysign((1.0-a)*mag+a*e,v);
        }

        T read(const std::vector<T>& b,double ds)const{
            if(b.empty())return T{};
            ds=std::clamp(ds,2.0,double(b.size()-4));
            double p=double(w)-ds;
            while(p<0)p+=b.size();
            while(p>=double(b.size()))p-=b.size();
            const long i1=(long)std::floor(p);
            const double f=p-double(i1);
            auto at=[&](long idx)->double{
                const long n=(long)b.size();
                idx%=n;if(idx<0)idx+=n;
                return double(b[(size_t)idx]);
            };
            const double y0=at(i1-1),y1=at(i1),y2=at(i1+1),y3=at(i1+2);
            const double c0=y1;
            const double c1=0.5*(y2-y0);
            const double c2=y0-2.5*y1+2.0*y2-0.5*y3;
            const double c3=0.5*(y3-y0)+1.5*(y1-y2);
            return(T)(((c3*f+c2)*f+c1)*f+c0);
        }
        static double lpCoeff(double hz,double sampleRate){
            return 1.0-std::exp(-2.0*pi*std::clamp(hz,20.0,0.48*sampleRate)/sampleRate);
        }
        T lowpass(T x,T& z,double hz){double a=lpCoeff(hz,sr);z=(T)(double(z)+a*(double(x)-double(z)));return z;}
        T highpass(T x,T& z,double hz){double a=lpCoeff(hz,sr);z=(T)(double(z)+a*(double(x)-double(z)));return(T)(double(x)-double(z));}
        double smooth(double target,double& state,double a){if(state<0.0||!std::isfinite(state))state=target;else state+=(target-state)*a;return state;}

        double periodicMod(double depth,double rate){
            phase1+=2*pi*rate/sr;if(phase1>2*pi)phase1-=2*pi;
            phase2+=2*pi*(rate*1.731+0.37)/sr;if(phase2>2*pi)phase2-=2*pi;
            return depth*(0.68*std::sin(phase1)+0.32*std::sin(phase2));
        }
        Stereo digitalColor(Stereo x,int type){
            if(type==0)return x;
            if(type==2){
                const double q=2048.0;
                x.l=(T)(std::round(double(x.l)*q)/q);
                x.r=(T)(std::round(double(x.r)*q)/q);
                return x;
            }
            auto adm=[&](T in,T& pred,double& step){
                double d=double(in)-double(pred),sg=d>=0?1.0:-1.0;
                pred=(T)(double(pred)+sg*step);
                if(std::abs(d)>step*1.7)step*=1.08;else step*=0.995;
                step=std::clamp(step,1.0/2048.0,0.10);
                return pred;
            };
            x.l=adm(x.l,admPredL,admStepL);x.r=adm(x.r,admPredR,admStepR);return x;
        }
        static std::array<double,4> volanteRatios(double spacing){
            static constexpr double m[4][4]={{.25,.50,.75,1.0},{1.0/6.0,1.0/3.0,2.0/3.0,1.0},{.236,.382,.618,1.0},{.172,.414,.707,1.0}};
            double x=std::clamp(spacing,0.0,1.0)*3.0;int a=std::min(2,(int)x);double t=x-a;
            std::array<double,4> z{};for(int i=0;i<4;++i)z[i]=m[a][i]+(m[a+1][i]-m[a][i])*t;return z;
        }
        void applyPhysicalFrame(Stereo& wet,Stereo& fbs,const MechanicalFrame& m,double cutoff){
            const double c=std::clamp(cutoff*m.tone,280.0,0.46*sr);
            wet.l=lowpass((T)(double(wet.l)*m.gain+m.impulse+0.30*m.noise),wetLpL,c);
            wet.r=lowpass((T)(double(wet.r)*m.gain-0.47*m.impulse+0.21*m.noise),wetLpR,c*0.985);
            fbs.l=(T)(double(fbs.l)*m.gain+m.impulse+0.62*m.noise);
            fbs.r=(T)(double(fbs.r)*m.gain-0.41*m.impulse+0.49*m.noise);
        }

        Stereo process(T inL,T inR,const SlotParams& p,double bpm,double machineCondition){
            const double condition=std::clamp(machineCondition,0.0,1.0);
            const double damage=condition*condition*(0.82+0.18*condition);
            auto aged=[&](double local,double amount){return std::clamp(local+damage*amount*(1.0-0.28*local),0.0,1.0);};
            if(l.empty())return{};
            if(p.enable<.5){
                if(enabledLast)clearAudio();
                enabledLast=false;
                return{};
            }
            enabledLast=true;

            const int algo=normIndex(p.algorithm,kAlgorithmCount);
            auto& c=p.c[algo];
            const double targetMs=slotDelayMs(p,bpm);
            double ms=smooth(targetMs,delayMsSm,timeSmoothA);
            double fb=smooth(std::clamp(p.feedback*1.075,0.0,1.075),feedbackSm,controlSmoothA);
            double level=smooth(std::clamp(p.level*1.25,0.0,1.25),levelSm,controlSmoothA);
            const double duckAmt=std::clamp(p.duck,0.0,1.0);
            const double ia=std::max(std::abs((double)inL),std::abs((double)inR));
            const double atk=1.0-std::exp(-1.0/(0.008*sr)),rel=1.0-std::exp(-1.0/(0.180*sr));
            duckEnv=(T)(double(duckEnv)+(ia-double(duckEnv))*(ia>double(duckEnv)?atk:rel));
            const double duckGain=1.0-duckAmt*std::clamp(double(duckEnv)*2.0,0.0,0.85);
            Stereo wet{},fbs{};

            if(algo==(int)DelayAlgorithm::Volante){
                ms=std::clamp(ms,100.0,4000.0);
                const double mechanics=c[0],wear=c[1],spread=c[4],drive=c[5];
                const double globalWear=aged(wear,0.92),globalMechanics=aged(mechanics,0.78);
                auto mech=transport.tick(globalMechanics,aged(0.32*mechanics+0.22*wear,0.72),globalWear,aged(std::max(mechanics,wear),0.94),aged(0.30*wear,0.48));
                auto ratios=volanteRatios(c[2]);double sp=0,sf=0;Stereo feedback{};
                for(int h=0;h<4;++h){
                    if(c[6+h]<0.5&&c[10+h]<0.5)continue;
                    const double local=1.0+mech.pitch*(1.0+0.055*h)+0.00035*h*noiseRng.bipolar();
                    const double ds=sr*ms*ratios[h]/1000.0*local;
                    const double mono=.5*(double(read(l,ds))+double(read(r,ds)));
                    const double pos=std::clamp((p.headPan[h]*2.0-1.0)*spread,-1.0,1.0);
                    const double gl=std::sqrt(.5*(1-pos)),gr=std::sqrt(.5*(1+pos));
                    if(c[6+h]>=.5){wet.l+=(T)(mono*gl);wet.r+=(T)(mono*gr);sp++;}
                    if(c[10+h]>=.5){feedback.l+=(T)(mono*gl);feedback.r+=(T)(mono*gr);sf++;}
                }
                if(sp>0){wet.l=(T)(double(wet.l)/std::sqrt(sp));wet.r=(T)(double(wet.r)/std::sqrt(sp));}
                if(sf>0){feedback.l=(T)(double(feedback.l)/sf);feedback.r=(T)(double(feedback.r)/sf);}
                const double cutoff=11000-7600*globalWear;
                fbs.l=lowpass(tapeSat(feedback.l,drive,.58,globalWear,tapeMemL),lpL,cutoff);
                fbs.r=lowpass(tapeSat(feedback.r,drive,.43,globalWear,tapeMemR),lpR,cutoff*.97);
                const double lc=35+450*c[3];
                fbs.l=highpass(fbs.l,hpMemL,lc);fbs.r=highpass(fbs.r,hpMemR,lc);
                applyPhysicalFrame(wet,fbs,mech,15000-9000*globalWear);
            }else if(algo==(int)DelayAlgorithm::ElCapistan){
                ms=std::clamp(ms,35.0,2500.0);
                const double age=c[0],wow=c[1],flutter=c[2],crinkle=c[3],bias=c[4],lowContour=c[5],spring=c[6];
                const double effectiveAge=aged(age,0.78),effectiveCrinkle=aged(crinkle,0.96);
                const double wear=std::clamp(0.52*effectiveAge+0.78*effectiveCrinkle,0.0,1.0);
                auto mech=transport.tick(aged(wow,0.66),aged(flutter,0.74),wear,aged(std::clamp(0.28*wow+0.30*flutter+0.95*crinkle,0.0,1.0),0.92),aged(0.22*age+0.38*crinkle,0.62));
                const double base=sr*ms/1000.0*(1.0+mech.pitch);
                const int mode=normIndex(c[7],3);
                auto tap=[&](double rr){return Stereo{read(l,base*rr),read(r,base*rr)};};
                if(mode==0)wet=tap(1);
                else if(mode==1){auto a=tap(.5),b=tap(1);wet={(T)(.64*a.l+.64*b.l),(T)(.64*a.r+.64*b.r)};}
                else{auto a=tap(.72),b=tap(1.0);wet={(T)(.38*a.l+.78*b.l),(T)(.38*a.r+.78*b.r)};}
                if(spring>.001){
                    auto s1=tap(.071),s2=tap(.103),s3=tap(.137);
                    wet.l+=(T)(spring*(.19*double(s1.r)-.13*double(s2.l)+.08*double(s3.r)));
                    wet.r+=(T)(spring*(.19*double(s1.l)-.13*double(s2.r)+.08*double(s3.l)));
                }
                const double cutoff=12500-8600*effectiveAge;
                fbs.l=lowpass(tapeSat(wet.l,.30+.78*bias,bias,effectiveAge,tapeMemL),lpL,cutoff);
                fbs.r=lowpass(tapeSat(wet.r,.30+.78*bias,bias*.94,effectiveAge,tapeMemR),lpR,cutoff*.98);
                fbs.l=highpass(fbs.l,hpMemL,45+350*lowContour);fbs.r=highpass(fbs.r,hpMemR,45+350*lowContour);
                applyPhysicalFrame(wet,fbs,mech,14500-9300*effectiveAge);
            }else if(algo==(int)DelayAlgorithm::Olivera){
                ms=std::clamp(ms,155.0,620.0);
                const double wear=c[0],visc=c[1],statik=c[2],hm=c[3],tone=c[4],dr=c[5];
                const double effectiveWear=aged(wear,0.88),effectiveStatic=aged(statik,0.80),effectiveDrift=aged(dr,0.74);
                auto mech=transport.tick(effectiveDrift*(1.0-0.48*visc),0.16*effectiveDrift,effectiveWear,aged(std::clamp(dr+0.42*wear,0.0,1.0),0.90),effectiveStatic);
                const double base=sr*ms/1000.0*(1.0+mech.pitch*(0.55+0.45*(1.0-visc)));
                auto sh=Stereo{read(l,base*.47),read(r,base*.47)};
                auto lh=Stereo{read(l,base),read(r,base)};
                auto disc=Stereo{read(l,std::min(base*1.29,double(l.size()-4))),read(r,std::min(base*1.29,double(r.size()-4)))};
                auto smearA=Stereo{read(l,base*(.985-.010*visc)),read(r,base*(.989-.006*visc))};
                auto smearB=Stereo{read(l,base*(1.014+.012*visc)),read(r,base*(1.010+.009*visc))};
                wet.l=(T)((1-hm)*double(sh.l)+hm*double(lh.l)+.18*double(disc.l)+.16*visc*(double(smearA.l)+double(smearB.l)));
                wet.r=(T)((1-hm)*double(sh.r)+hm*double(lh.r)+.18*double(disc.r)+.16*visc*(double(smearA.r)+double(smearB.r)));
                const double cutoff=1800+5200*tone-900*effectiveWear;
                fbs.l=lowpass(tapeSat(wet.l,.35+.65*effectiveWear,.54,effectiveWear,tapeMemL),lpL,cutoff);
                fbs.r=lowpass(tapeSat(wet.r,.35+.65*effectiveWear,.46,effectiveWear,tapeMemR),lpR,cutoff*.96);
                applyPhysicalFrame(wet,fbs,mech,4200+2900*tone-1200*effectiveWear);
            }else if(algo==(int)DelayAlgorithm::EC1){
                ms=std::clamp(ms,40.0,2500.0);
                const double mechAmt=c[0],age=c[1],bias=c[2],pre=c[3],rec=c[4],st=c[5];
                const double effectiveAge=aged(age,0.76),effectiveMech=aged(mechAmt,0.78);
                auto mech=transport.tick(effectiveMech,0.34*effectiveMech,effectiveAge,aged(std::clamp(mechAmt+0.34*age,0.0,1.0),0.90),aged(0.12*age,0.42));
                const double base=sr*ms/1000.0*(1.0+mech.pitch);
                wet={read(l,base*(1-.0025*st)),read(r,base*(1+.0025*st))};
                const double cutoff=13000-8200*effectiveAge;
                fbs.l=lowpass(tubeStage(wet.l,pre,rec,bias,tubeEnvL),lpL,cutoff);
                fbs.r=lowpass(tubeStage(wet.r,pre,rec,1.0-bias,tubeEnvR),lpR,cutoff*.985);
                applyPhysicalFrame(wet,fbs,mech,15500-9000*effectiveAge);
            }else if(algo==(int)DelayAlgorithm::Brig){
                const int voice=normIndex(c[0],3);
                const bool sync=p.sync>=.5;
                const double maxMs=sync?2000.0:(voice==0?300.0:1000.0),minMs=voice==0?30.0:100.0;
                ms=std::clamp(ms,minMs,maxMs);
                const double filter=c[1],md=c[2],mr=c[3],comp=c[4],noise=c[5];
                const double effectiveNoise=aged(noise,0.72),effectiveMod=aged(md,0.38);
                const double jitter=periodicMod((.00025+.0065*effectiveMod)*(voice==0?1.20:1.0),.22+3.4*mr)+noiseRng.bipolar()*(.00022*effectiveMod+.00035*damage);
                const double base=sr*ms/1000.0*(1.0+jitter);
                if(voice==2){auto a=Stereo{read(l,base),read(r,base*.618)};wet=a;fbs={(T)((1-c[1]*.25)*double(a.r)),(T)((1-c[1]*.25)*double(a.l))};}
                else{wet={read(l,base),read(r,base)};fbs=wet;}
                const double stages=voice==2?8192.0:4096.0;
                const double physicalBw=std::clamp((voice==0?.28:.40)*stages*1000.0/ms,850.0,voice==0?5200.0:9800.0);
                const double cutoff=physicalBw*(.58+.68*filter);
                wet.l=compandExpand(wet.l,comp*.52);wet.r=compandExpand(wet.r,comp*.52);
                fbs.l=lowpass(compandCompress(sat(fbs.l,voice==0?2.2:1.55,.03),comp),lpL,cutoff);
                fbs.r=lowpass(compandCompress(sat(fbs.r,voice==0?2.2:1.55,-.03),comp),lpR,cutoff*.97);
                const double clockHz=std::clamp(stages*500.0/ms,1800.0,0.44*sr);
                clockPhase+=2*pi*clockHz/sr;if(clockPhase>2*pi)clockPhase-=2*pi;
                const double clockBleed=std::sin(clockPhase)*(0.000015+0.00022*effectiveNoise+0.00012*damage);
                const double n=noiseRng.bipolar()*effectiveNoise*(voice==0?.0010:.00048);
                wet.l=(T)(double(wet.l)+.22*n+.22*clockBleed);wet.r=(T)(double(wet.r)-.18*n+.17*clockBleed);
                fbs.l=(T)(double(fbs.l)+n+clockBleed);fbs.r=(T)(double(fbs.r)-.79*n+.71*clockBleed);
            }else if(algo==(int)DelayAlgorithm::Deco){
                ms=std::clamp(ms,.3,500.0);
                const double saturation=c[0],wobble=c[1],blend=c[2],width=c[4],flange=c[5];
                const double effectiveWobble=aged(wobble,0.72),effectiveWear=aged(.16*saturation,0.56);
                auto mech=transport.tick(effectiveWobble,.22*effectiveWobble,effectiveWear,aged(.42*wobble,0.78),aged(.06*saturation,0.30));
                double d=sr*ms/1000.0*(1.0+mech.pitch);
                const int type=normIndex(c[3],3);
                const double flangeSweep=periodicMod(1.0,.055+.20*wobble);
                if(flange>0&&ms<20)d=std::max(2.0,d+sr*.0035*flange*flangeSweep);
                auto lag=Stereo{read(l,d*(1-.002*width)),read(r,d*(1+.002*width))};
                lag.l=tapeSat(lag.l,saturation,.56,aged(.18+.28*wobble,.48),tapeMemL);
                lag.r=tapeSat(lag.r,saturation,.44,aged(.18+.28*wobble,.48),tapeMemR);
                if(type==1){lag.l=(T)-lag.l;lag.r=(T)-lag.r;}
                if(type==2){T x=lag.r;lag.r=(T)(.25*double(lag.l));lag.l=x;}
                wet.l=(T)((1-blend)*double(inL)+blend*double(lag.l));
                wet.r=(T)((1-blend)*double(inR)+blend*double(lag.r));
                fbs={};fb=0;
                applyPhysicalFrame(wet,fbs,mech,16500-4200*saturation-2800*damage);
            }else{
                ms=std::clamp(ms,20.0,3200.0);
                const int type=normIndex(c[0],3);
                const double ratio=.5+c[1],md=c[2],cross=c[3],dyn=c[4],tone=c[5];
                const double mod=periodicMod(.0002+.004*md+.0012*damage,.25+2.2*md)+noiseRng.bipolar()*.00012*damage;
                const double d1=sr*ms/1000.0*(1+mod),d2=std::clamp(d1*ratio,2.0,double(l.size()-4));
                Stereo a{read(l,d1),read(r,d1)},b{read(l,d2),read(r,d2)};
                a=digitalColor(a,type);b=digitalColor(b,type);
                wet={(T)(.72*double(a.l)+.62*double(b.l)),(T)(.72*double(a.r)+.62*double(b.r))};
                const double dg=1-dyn*std::clamp(double(duckEnv)*1.4,0.0,.65);
                fbs.l=(T)(dg*((1-cross)*double(wet.l)+cross*double(wet.r)));
                fbs.r=(T)(dg*((1-cross)*double(wet.r)+cross*double(wet.l)));
                const double cutoff=5500+12500*tone;
                fbs.l=lowpass((T)(double(fbs.l)+noiseRng.bipolar()*.00012*damage),lpL,cutoff*(1.0-.24*damage));fbs.r=lowpass((T)(double(fbs.r)+noiseRng.bipolar()*.00010*damage),lpR,cutoff*(1.0-.22*damage));
            }

            T recL=sat((T)(double(inL)+fb*double(fbs.l)),1.05,.01);
            T recR=sat((T)(double(inR)+fb*double(fbs.r)),1.05,-.01);
            l[w]=recL;r[w]=recR;w=(w+1)%l.size();

            const double pan=std::clamp(p.pan*2-1.0,-1.0,1.0);
            const double gl=std::sqrt(.5*(1-pan))*1.41421356237,gr=std::sqrt(.5*(1+pan))*1.41421356237;
            wet.l=(T)(double(wet.l)*level*duckGain*gl);
            wet.r=(T)(double(wet.r)*level*duckGain*gr);
            return wet;
        }
    };

    SlotDSP s[2];
    double sr=44100.0;
public:
    void prepare(double sampleRate,int=2){
        sr=sampleRate;
        s[0].prepare(sampleRate,0x13579bdfu);
        s[1].prepare(sampleRate,0x2468ace1u);
    }
    void reset(){for(auto& x:s)x.reset();}

    template<class Sample> void process(Sample** in,Sample** out,int channels,int n,const MXDelayParams& p,double bpm,double& peak){
        const double inGain=gainFromNorm(p.inputTrim,-18,18);
        const double outGain=gainFromNorm(p.outputTrim,-18,12);
        const double mix=std::clamp(p.mix,0.0,1.0);
        const double dryG=std::cos(mix*pi*.5),wetG=std::sin(mix*pi*.5);
        const int routing=normIndex(p.routing,3);
        const bool bypass=p.bypass>=.5,trails=p.spill>=.5;
        peak=0;

        if(bypass&&!trails){s[0].reset();s[1].reset();}

        for(int i=0;i<n;++i){
            const T rawL=(T)(in&&in[0]?in[0][i]:0);
            const T rawR=(channels>1&&in&&in[1])?(T)in[1][i]:rawL;

            if(bypass&&!trails){
                if(out&&out[0])out[0][i]=(Sample)rawL;
                if(channels>1&&out&&out[1])out[1][i]=(Sample)rawR;
                peak=std::max({peak,std::abs((double)rawL),std::abs((double)rawR)});
                continue;
            }

            const T inL=(T)(bypass?0.0:double(rawL)*inGain);
            const T inR=(T)(bypass?0.0:double(rawR)*inGain);
            Stereo a{},b{},wet{};

            if(routing==(int)DelayRouting::Parallel){
                a=s[0].process(inL,inR,p.slot[0],bpm,p.machineCondition);
                b=s[1].process(inL,inR,p.slot[1],bpm,p.machineCondition);
                wet={(T)(double(a.l)+double(b.l)),(T)(double(a.r)+double(b.r))};
            }else if(routing==(int)DelayRouting::Series){
                a=s[0].process(inL,inR,p.slot[0],bpm,p.machineCondition);
                b=s[1].process((T)(double(inL)+.72*double(a.l)),(T)(double(inR)+.72*double(a.r)),p.slot[1],bpm,p.machineCondition);
                wet={(T)(.48*double(a.l)+double(b.l)),(T)(.48*double(a.r)+double(b.r))};
            }else{
                a=s[0].process(inL,inL,p.slot[0],bpm,p.machineCondition);
                b=s[1].process(inR,inR,p.slot[1],bpm,p.machineCondition);
                wet={a.l,b.r};
            }

            T yL{},yR{};
            if(bypass&&trails){
                yL=(T)(double(rawL)+wetG*double(wet.l)*outGain);
                yR=(T)(double(rawR)+wetG*double(wet.r)*outGain);
            }else{
                yL=(T)((dryG*double(inL)+wetG*double(wet.l))*outGain);
                yR=(T)((dryG*double(inR)+wetG*double(wet.r))*outGain);
            }
            if(out&&out[0])out[0][i]=(Sample)yL;
            if(channels>1&&out&&out[1])out[1][i]=(Sample)yR;
            peak=std::max({peak,std::abs((double)yL),std::abs((double)yR)});
        }
    }
};
} // namespace JerzyAudio
