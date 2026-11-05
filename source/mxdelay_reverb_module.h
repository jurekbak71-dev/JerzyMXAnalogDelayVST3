#pragma once
#include "mxdelay_params.h"
#include "mxdelay_reverb_fdn.h"
#include "mxdelay_ir_loader.h"
namespace JerzyAudio {
template<class T> class MXReverbModule {
    MXReverbFDN<T> fdn;MXIRLoader loader;MXPartitionedConvolver* active=nullptr;MXPartitionedConvolver* old=nullptr;
    std::array<std::vector<float>,2> pre;size_t prePos=0,preValid=0;
    double sr=48000,a=.001,enabled=0,wet=0,irWeight=0,fdnWeight=1,width=1,delay=0,fade=1;
    double irTone[2]{},lastIrDamp=-1,lastLow=-1,lowHz=20,irAlpha=1,irAlphaTarget=1;bool first=true,quiescent=false,irWasActive=false;
    static double safe(double v,double fallback=0){return std::isfinite(v)?v:fallback;}
public:
    ~MXReverbModule(){loader.stop();delete active;delete old;}
    void prepare(double rate,const ReverbParams* initial=nullptr){
        loader.stop();delete active;delete old;active=old=nullptr;sr=std::clamp(safe(rate,48000),8000.,384000.);a=1-std::exp(-1/(.025*sr));
        fdn.prepare(sr);for(auto& c:pre)c.assign(size_t(sr*.25)+4,0);
        ReverbParams defaults;const auto& cfg=initial?*initial:defaults;
        std::string failure;MXIRAudio ir;
        try {auto source=cfg.external>=.5?loadMXIRWav(cfg.irPath):factoryMXIR(normIndex(cfg.ir,3),sr);ir=prepareMXIR(source,sr,.02+7.98*cfg.irLength,cfg.normalize>=.5);}
        catch(const std::exception& e){failure=e.what();ir=prepareMXIR(factoryMXIR(0,sr),sr,8.,true);}
        active=new MXPartitionedConvolver(ir);if(!cfg.irPath.empty())loader.requestPath(cfg.irPath);
        loader.start(sr,MXIRLoader::key(normIndex(cfg.ir,3),cfg.external>=.5,cfg.irLength,cfg.normalize>=.5));loader.initialStatus(ir.name,failure);reset();
    }
    void reset(){fdn.reset();if(active)active->reset();if(old)old->reset();prePos=preValid=0;irTone[0]=irTone[1]=0;first=true;quiescent=false;lastIrDamp=lastLow=-1;irAlpha=1;irWasActive=false;}
    void requestPath(const std::string& path,bool select=true){loader.requestPath(path,select);}
    MXIRLoader::Status status()const{return loader.status();}
    void process(T& l,T& r,const ReverbParams& p,T sourceL,T sourceR){
        if(!active)return;
        loader.requestOptions(MXIRLoader::key(normIndex(p.ir,3),p.external>=.5,p.irLength,p.normalize>=.5));
        const double targetEnable=p.enable>=.5?1:0,targetWet=std::clamp(safe(p.wet),0.,1.);
        const int mode=normIndex(p.mode,3);const double balance=std::clamp(safe(p.hybrid,.5),0.,1.);
        const double targetIR=mode==1?1:mode==2?balance:0,targetFDN=1-targetIR;
        if(first){enabled=targetEnable;wet=targetWet;irWeight=targetIR;fdnWeight=targetFDN;width=2*std::clamp(safe(p.width,.5),0.,1.);delay=240*std::clamp(safe(p.preDelay),0.,1.)*.001*sr;first=false;}
        enabled+=a*(targetEnable-enabled);wet+=a*(targetWet-wet);irWeight+=a*(targetIR-irWeight);fdnWeight+=a*(targetFDN-fdnWeight);
        width+=a*(2*std::clamp(safe(p.width,.5),0.,1.)-width);delay+=a*(240*std::clamp(safe(p.preDelay),0.,1.)*.001*sr-delay);
        if(old&&fade>=1&&loader.retire(old))old=nullptr;
        if(!old)if(auto* next=loader.take()){old=active;active=next;fade=enabled<1e-5?1:0;}
        const double dryL=safe(double(l)),dryR=safe(double(r));
        if(enabled<1e-5&&!targetEnable){if(!quiescent){fdn.reset();active->reset();if(old)old->reset();prePos=preValid=0;quiescent=true;}l=T(dryL);r=T(dryR);return;}
        quiescent=false;
        const double excitation[2]={safe(double(sourceL)),safe(double(sourceR))};float input[2]{};
        for(int c=0;c<2;++c){pre[c][prePos]=float(std::clamp(excitation[c],-32.,32.)*targetEnable);const size_t d=size_t(delay);const double f=delay-d;
            auto read=[&](size_t lag){return lag>preValid?0.f:pre[c][(prePos+pre[c].size()-lag)%pre[c].size()];};input[c]=float(read(d)+(read(d+1)-read(d))*f);}
        prePos=(prePos+1)%pre[0].size();preValid=std::min(preValid+1,pre[0].size()-1);
        T fl=T(input[0]),fr=T(input[1]);typename MXReverbFDN<T>::Params f;
        f.enabled=true;f.wet=1;f.preDelayMs=0;f.decay=p.decay;f.damping=p.damping;f.width=1;f.freeze=p.freeze>=.5;if(p.lowCut!=lastLow){lowHz=20*std::pow(25.,std::clamp(safe(p.lowCut),0.,1.));lastLow=p.lowCut;}f.lowCutHz=lowHz;f.diffusion=p.diffusion;f.early=p.early;
        fdn.processWet(fl,fr,f);
        float il=input[0],ir=input[1];
        if(irWeight>1e-6||targetIR>0){irWasActive=true;active->process(il,ir);if(old&&fade<1){float ol=input[0],orr=input[1];old->process(ol,orr);il=float((1-fade)*ol+fade*il);ir=float((1-fade)*orr+fade*ir);}}
        else {il=ir=0;if(irWasActive){active->reset();if(old)old->reset();irWasActive=false;}}
        if(fade<1)fade=std::min(1.,fade+1/(.050*sr));
        const double damping=std::clamp(safe(p.irDamping),0.,1.);if(damping!=lastIrDamp){irAlphaTarget=damping==0?1:1-std::exp(-6.28318530718*(20000*std::pow(.04,damping))/sr);lastIrDamp=damping;}
        irAlpha+=a*(irAlphaTarget-irAlpha);
        irTone[0]+=irAlpha*(il-irTone[0]);irTone[1]+=irAlpha*(ir-irTone[1]);
        double wl=fdnWeight*double(fl)+irWeight*irTone[0],wr=fdnWeight*double(fr)+irWeight*irTone[1];
        const double mid=(wl+wr)*.5,side=(wl-wr)*.5*width;wl=mid+side;wr=mid-side;
        const double mix=enabled*wet;
        // When delays are disabled, source can be dry input even if legacy Mix=100%.
        const double baseL=dryL+enabled*(safe(double(sourceL))-dryL),baseR=dryR+enabled*(safe(double(sourceR))-dryR);
        const double outL=(1-mix)*baseL+mix*wl,outR=(1-mix)*baseR+mix*wr;
        l=T(std::isfinite(outL)?std::clamp(outL,-32.,32.):0.);r=T(std::isfinite(outR)?std::clamp(outR,-32.,32.):0.);
    }
};
}
