#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace JerzyAudio {
// Eight-line Householder FDN. All storage is allocated in prepare; reset is logical.
template<class T> class MXReverbFDN {
    struct Line {
        std::vector<T> buffer;size_t pos=0,written=0;double lp=0,low=0,gain=0;
        void prepare(size_t n){buffer.assign(std::max<size_t>(n,8),T{});reset();}
        void reset(){pos=written=0;lp=low=0;gain=0;}
        double read()const{return written<buffer.size()?0.:double(buffer[pos]);}
        double tap(double delay)const{
            if(buffer.empty())return 0.;
            delay=std::clamp(delay,0.,double(buffer.size()-2));const size_t d=size_t(delay);
            auto at=[&](size_t lag){return lag>=written?0.:double(buffer[(pos+buffer.size()-1-lag)%buffer.size()]);};
            return at(d)+(at(d+1)-at(d))*(delay-d);
        }
        void write(double x){buffer[pos]=T(std::isfinite(x)?std::clamp(x,-16.,16.):0.);pos=(pos+1)%buffer.size();written=std::min(written+1,buffer.size());}
    };
    std::array<Line,8> lines;
    std::array<Line,2> pre;
    std::array<std::array<Line,4>,2> diffuser;
    static constexpr std::array<double,8> delays{{.0297,.0371,.0411,.0437,.0313,.0359,.0399,.0473}};
    double sr=48000,a=.001,wetSm=0,widthSm=1,delaySm=0,diffSm=.6,earlySm=.25,freezeSm=0,hpSm=0,lpSm=1;
    double lastDecay=-1,lastDamp=-1,lastLow=-1,targetLP=1,targetHP=0;
    std::array<double,8> targetGain{};bool first=true,lastFreeze=false;
    static double finite(double x,double fallback=0){return std::isfinite(x)?x:fallback;}
public:
    struct Params {bool enabled=false;double wet=.25,decay=.6,damping=.5,width=1,preDelayMs=18,lowCutHz=20,diffusion=.6,early=.25;bool freeze=false;};
    void prepare(double rate){
        sr=std::clamp(finite(rate,48000),8000.,384000.);a=1-std::exp(-1/(.025*sr));
        for(size_t i=0;i<8;++i)lines[i].prepare(size_t(std::ceil(sr*delays[i]))+1);
        for(auto& l:pre)l.prepare(size_t(sr*.30)+4);
        constexpr double d[]={.0047,.0073,.0113,.0131};
        for(int c=0;c<2;++c)for(int i=0;i<4;++i)diffuser[c][i].prepare(size_t(sr*(d[i]+c*.00017))+1);
        reset();
    }
    void reset(){for(auto& l:lines)l.reset();for(auto& l:pre)l.reset();for(auto& c:diffuser)for(auto& l:c)l.reset();first=true;lastDecay=lastDamp=lastLow=-1;}
    void processWet(T& left,T& right,const Params& p){
        if(lines[0].buffer.empty()){left=right=0;return;}
        const double input[2]={finite(double(left)),finite(double(right))};
        const double decay=std::clamp(finite(p.decay),0.,1.),damp=std::clamp(finite(p.damping),0.,1.);
        const double low=std::clamp(finite(p.lowCutHz,20),20.,500.);
        if(decay!=lastDecay||p.freeze!=lastFreeze){
            const double rt60=.35+12*decay*decay;
            for(size_t i=0;i<8;++i)targetGain[i]=p.freeze?.9995:std::pow(10.,-3*double(lines[i].buffer.size())/(sr*rt60));
            lastDecay=decay;lastFreeze=p.freeze;
        }
        if(damp!=lastDamp){targetLP=1-std::exp(-6.28318530718*(20000*std::pow(.04,damp))/sr);lastDamp=damp;}
        if(low!=lastLow){targetHP=1-std::exp(-6.28318530718*low/sr);lastLow=low;}
        const double targetDelay=std::clamp(finite(p.preDelayMs),0.,240.)*.001*sr;
        if(first){freezeSm=p.freeze?1:0;delaySm=targetDelay;widthSm=std::clamp(finite(p.width,1),0.,2.);diffSm=std::clamp(finite(p.diffusion),0.,1.);earlySm=std::clamp(finite(p.early),0.,1.);lpSm=targetLP;hpSm=targetHP;for(size_t i=0;i<8;++i)lines[i].gain=targetGain[i];first=false;}
        delaySm+=a*(targetDelay-delaySm);widthSm+=a*(std::clamp(finite(p.width,1),0.,2.)-widthSm);
        diffSm+=a*(std::clamp(finite(p.diffusion),0.,1.)-diffSm);earlySm+=a*(std::clamp(finite(p.early),0.,1.)-earlySm);
        lpSm+=a*(targetLP-lpSm);hpSm+=a*(targetHP-hpSm);
        freezeSm+=a*((p.freeze?1.:0.)-freezeSm);
        double injection[2]{},early[2]{};
        for(int c=0;c<2;++c){
            pre[c].write((1-freezeSm)*input[c]);
            double x=pre[c].tap(delaySm);
            early[c]=.52*pre[c].tap(delaySm+sr*(c?.0037:.0031))-.27*pre[1-c].tap(delaySm+sr*.011)+.18*pre[c].tap(delaySm+sr*.021);
            const double g=.18+.52*diffSm;
            for(auto& l:diffuser[c]){const double delayed=l.read();const double y=delayed-g*x;l.write(x+g*y);x=y;}
            injection[c]=x*.22;
        }
        double taps[8]{},sum=0;
        for(int i=0;i<8;++i){taps[i]=lines[i].read();sum+=taps[i];}
        for(int i=0;i<8;++i){
            auto& l=lines[i];const double mixed=.25*sum-taps[i];
            l.lp+=lpSm*(mixed-l.lp);l.low+=hpSm*(l.lp-l.low);l.gain+=a*(targetGain[i]-l.gain);
            const double filtered=(1-freezeSm)*(l.lp-l.low)+freezeSm*mixed;
            l.write((i<4?1.:-1.)*injection[i%2]+l.gain*filtered);
        }
        const double l=.25*(taps[0]-taps[1]+taps[2]-taps[3]+taps[4]-taps[5]+taps[6]-taps[7]);
        const double r=.25*(taps[0]+taps[1]-taps[2]-taps[3]-taps[4]-taps[5]+taps[6]+taps[7]);
        const double mid=(l+r)*.5,side=(l-r)*.5*widthSm;
        left=T(finite((1-.5*earlySm)*(mid+side)+earlySm*early[0]));right=T(finite((1-.5*earlySm)*(mid-side)+earlySm*early[1]));
    }
    void process(T& l,T& r,const Params& p){
        const double dryL=finite(double(l)),dryR=finite(double(r));
        if(!p.enabled)return;
        if(first)wetSm=std::clamp(finite(p.wet),0.,1.);
        wetSm+=a*(std::clamp(finite(p.wet),0.,1.)-wetSm);processWet(l,r,p);
        l=T((1-wetSm)*dryL+wetSm*double(l));r=T((1-wetSm)*dryR+wetSm*double(r));
    }
};
}
