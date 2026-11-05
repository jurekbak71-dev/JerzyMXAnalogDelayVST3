#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace JerzyAudio {
struct MXIRAudio {double rate=48000;std::vector<float> left,right;std::string name;};
inline MXIRAudio loadMXIRWav(const std::string& path){
    std::ifstream file(std::filesystem::path(std::u8string(path.begin(),path.end())),std::ios::binary|std::ios::ate);
    if(!file)throw std::runtime_error("Cannot open WAV IR");
    const auto length=file.tellg();if(length<44||length>64*1024*1024)throw std::runtime_error("WAV size outside 44 bytes..64 MiB");
    std::vector<unsigned char> b(static_cast<size_t>(length));file.seekg(0);if(!file.read(reinterpret_cast<char*>(b.data()),length))throw std::runtime_error("Incomplete WAV read");
    auto u16=[&](size_t p){if(p+2>b.size())throw std::runtime_error("Truncated WAV");return unsigned(b[p])|(unsigned(b[p+1])<<8);};
    auto u32=[&](size_t p){if(p+4>b.size())throw std::runtime_error("Truncated WAV");return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);};
    auto tag=[&](size_t p,const char* s){return p+4<=b.size()&&std::equal(b.begin()+p,b.begin()+p+4,s);};
    if(!tag(0,"RIFF")||!tag(8,"WAVE")||uint64_t(u32(4))+8>b.size())throw std::runtime_error("Not a complete little-endian RIFF/WAVE file");
    size_t data=0,size=0;unsigned format=0,channels=0,bits=0,align=0,rate=0;
    const size_t end=size_t(u32(4))+8;
    for(size_t p=12;p+8<=end;){
        const size_t n=u32(p+4),start=p+8;if(n>end-start)throw std::runtime_error("WAV chunk exceeds file");
        if(tag(p,"fmt ")){if(n<16)throw std::runtime_error("Invalid WAV fmt chunk");format=u16(start);channels=u16(start+2);rate=u32(start+4);align=u16(start+12);bits=u16(start+14);}
        else if(tag(p,"data")&&!data){data=start;size=n;}
        p=start+n+(n&1);
    }
    if((format!=1&&format!=3)||(channels!=1&&channels!=2)||rate<8000||rate>384000||!data||!size)throw std::runtime_error("IR requires mono/stereo PCM or float WAV, 8..384 kHz");
    if((format==3&&bits!=32)||(format==1&&bits!=16&&bits!=24&&bits!=32)||align!=channels*(bits/8)||size%align)throw std::runtime_error("Unsupported or malformed WAV sample format");
    const size_t frames=size/align;if(frames>size_t(rate)*8)throw std::runtime_error("IR exceeds 8 second limit");
    MXIRAudio ir;ir.rate=rate;ir.left.resize(frames);ir.right.resize(frames);
    auto filename=std::filesystem::path(std::u8string(path.begin(),path.end())).filename().u8string();ir.name.assign(reinterpret_cast<const char*>(filename.data()),filename.size());
    auto read=[&](size_t p){
        float v=0;
        if(format==3){const uint32_t x=u32(p);v=std::bit_cast<float>(x);}
        else if(bits==16)v=float(int16_t(u16(p)))/32768.f;
        else if(bits==24){int32_t x=int32_t(b[p])|(int32_t(b[p+1])<<8)|(int32_t(b[p+2])<<16);if(x&0x800000)x-=0x1000000;v=float(x)/8388608.f;}
        else v=float(int32_t(u32(p)))/2147483648.f;
        if(!std::isfinite(v))throw std::runtime_error("IR contains NaN or infinity");return v;
    };
    for(size_t i=0;i<frames;++i){ir.left[i]=read(data+i*align);ir.right[i]=channels==2?read(data+i*align+bits/8):ir.left[i];}
    return ir;
}
inline MXIRAudio factoryMXIR(int profile,double rate){
    MXIRAudio ir;ir.rate=rate;const double durations[]={.18,.45,1.1};profile=std::clamp(profile,0,2);
    ir.name=profile==0?"Factory Room":profile==1?"Factory Plate":"Factory Hall";
    const size_t n=size_t(rate*durations[profile]);ir.left.resize(n);ir.right.resize(n);uint32_t rng=0x935c71u+uint32_t(profile)*7919;
    for(size_t i=0;i<n;++i){
        auto noise=[&](){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return double(rng)/2147483648.-1.;};
        const double env=std::exp(-8.*i/std::max<size_t>(1,n));
        ir.left[i]=float(env*noise()*.06);ir.right[i]=float(env*noise()*.06);
        if(i==size_t(rate*.003)){ir.left[i]+=.35f;ir.right[i]+=.28f;}
        if(i==size_t(rate*.011)){ir.left[i]+=.18f;ir.right[i]-=.13f;}
    }
    return ir;
}
inline MXIRAudio prepareMXIR(const MXIRAudio& input,double rate,double maxSeconds,bool normalize){
    if(!std::isfinite(input.rate)||input.rate<8000||input.rate>384000||input.left.empty()||input.left.size()!=input.right.size()||!std::isfinite(rate)||rate<8000||rate>384000)throw std::runtime_error("Invalid IR or processing rate");
    const size_t n=std::min(size_t(std::ceil(input.left.size()*rate/input.rate)),size_t(rate*std::clamp(maxSeconds,.02,8.)));
    if(n>1048576)throw std::runtime_error("IR exceeds 1048576 resampled-frame memory limit");
    MXIRAudio out;out.rate=rate;out.name=input.name;out.left.resize(n);out.right.resize(n);
    const double cutoff=std::min(1.,rate/input.rate)*.94;
    for(size_t i=0;i<n;++i){
        const double pos=i*input.rate/rate;const int64_t centre=int64_t(std::floor(pos));double a=0,b=0,w=0;
        if(rate==input.rate){a=input.left[i];b=input.right[i];w=1;}
        else for(int k=-16;k<=16;++k){
            const int64_t index=centre+k;const double x=pos-index;
            if(std::abs(x)>16)continue;const double xx=3.141592653589793*x*cutoff;
            const double sinc=std::abs(xx)<1e-12?1:std::sin(xx)/xx;
            const double weight=cutoff*sinc*(.5+.5*std::cos(3.141592653589793*x/16));w+=weight;
            if(index>=0&&uint64_t(index)<input.left.size()){a+=weight*input.left[size_t(index)];b+=weight*input.right[size_t(index)];}
        }
        out.left[i]=float(a/std::max(1e-12,w));out.right[i]=float(b/std::max(1e-12,w));
    }
    double energyL=0,energyR=0,peak=0;
    for(size_t i=0;i<n;++i){if(!std::isfinite(out.left[i])||!std::isfinite(out.right[i]))throw std::runtime_error("Invalid IR samples");energyL+=double(out.left[i])*out.left[i];energyR+=double(out.right[i])*out.right[i];peak=std::max({peak,std::abs(double(out.left[i])),std::abs(double(out.right[i]))});}
    if(peak<1e-12)throw std::runtime_error("IR is silent");
    const double gain=normalize?1/std::max({1e-12,peak,std::sqrt(energyL),std::sqrt(energyR)}):1.;
    for(size_t i=0;i<n;++i){out.left[i]=float(out.left[i]*gain);out.right[i]=float(out.right[i]*gain);}
    return out;
}
// Fixed FFT plan: tables prepared off the audio thread, transform never allocates.
class MXFFT {
    size_t n=0;std::vector<std::complex<float>> twiddle;std::vector<size_t> reverse;
public:
    void prepare(size_t size){n=size;twiddle.resize(n/2);reverse.resize(n);for(size_t i=0;i<n/2;++i){const double a=-6.283185307179586*i/n;twiddle[i]={float(std::cos(a)),float(std::sin(a))};}for(size_t i=0;i<n;++i){size_t r=0,v=i;for(size_t p=n;p>1;p>>=1){r=(r<<1)|(v&1);v>>=1;}reverse[i]=r;}}
    void transform(std::complex<float>* x,bool inverse=false)const{
        for(size_t i=0;i<n;++i)if(i<reverse[i])std::swap(x[i],x[reverse[i]]);
        for(size_t size=2;size<=n;size*=2)for(size_t start=0;start<n;start+=size)for(size_t j=0;j<size/2;++j){const auto w=inverse?std::conj(twiddle[j*n/size]):twiddle[j*n/size];const auto a=x[start+j],b=x[start+j+size/2]*w;x[start+j]=a+b;x[start+j+size/2]=a-b;}
        if(inverse)for(size_t i=0;i<n;++i)x[i]/=float(n);
    }
};
class MXPartitionTier {
    size_t block=0,count=0,pos=0,head=0;uint64_t epoch=1;
    MXFFT fft;
    std::array<std::vector<std::complex<float>>,2> filters,history,work;
    std::array<std::vector<float>,2> input,output,overlap;
    std::vector<uint64_t> valid;
public:
    void prepare(const MXIRAudio& ir,size_t offset,size_t b,size_t end){
        block=b;count=end>offset?(end-offset+b-1)/b:0;fft.prepare(2*b);valid.assign(count,0);
        for(int c=0;c<2;++c){filters[c].assign(count*2*b,{});history[c].assign(count*2*b,{});work[c].assign(2*b,{});input[c].assign(b,0);output[c].assign(b,0);overlap[c].assign(b,0);
            const auto& taps=c?ir.right:ir.left;
            for(size_t q=0;q<count;++q){auto* h=filters[c].data()+q*2*b;for(size_t k=0;k<b&&offset+q*b+k<end;++k)h[k]=taps[offset+q*b+k];fft.transform(h);}}
        reset();
    }
    void reset(){pos=head=0;if(++epoch==0){std::fill(valid.begin(),valid.end(),0);epoch=1;}for(int c=0;c<2;++c){std::fill(input[c].begin(),input[c].end(),0);std::fill(output[c].begin(),output[c].end(),0);std::fill(overlap[c].begin(),overlap[c].end(),0);}}
    void process(float l,float r,float& outL,float& outR){
        outL=outR=0;if(!count)return;
        outL=output[0][pos];outR=output[1][pos];input[0][pos]=l;input[1][pos]=r;
        if(++pos<block)return;pos=0;
        for(int c=0;c<2;++c){auto* x=history[c].data()+head*2*block;for(size_t i=0;i<block;++i)x[i]=input[c][i];std::fill(x+block,x+2*block,std::complex<float>{});fft.transform(x);}
        valid[head]=epoch;
        for(int c=0;c<2;++c){auto& sum=work[c];std::fill(sum.begin(),sum.end(),std::complex<float>{});
            for(size_t q=0;q<count;++q){const size_t index=(head+count-q)%count;if(valid[index]!=epoch)continue;const auto* h=filters[c].data()+q*2*block;const auto* x=history[c].data()+index*2*block;for(size_t i=0;i<2*block;++i)sum[i]+=h[i]*x[i];}
            fft.transform(sum.data(),true);for(size_t i=0;i<block;++i){output[c][i]=sum[i].real()+overlap[c][i];overlap[c][i]=sum[i+block].real();}}
        head=(head+1)%count;
    }
};
// Nonuniform zero-added-latency convolution: direct 64-sample head, 64-sample
// near partitions (offset 64), 1024-sample far partitions (offset 1024).
class MXPartitionedConvolver {
    std::array<std::array<float,64>,2> head{},ring{};size_t pos=0;
    MXPartitionTier near,far;
public:
    std::string name;size_t frames=0;double rate=48000;
    explicit MXPartitionedConvolver(const MXIRAudio& ir):name(ir.name),frames(ir.left.size()),rate(ir.rate){
        for(size_t i=0;i<std::min<size_t>(64,frames);++i){head[0][i]=ir.left[i];head[1][i]=ir.right[i];}
        near.prepare(ir,64,64,std::min<size_t>(1024,frames));far.prepare(ir,1024,1024,frames);reset();
    }
    void reset(){for(auto& c:ring)c.fill(0);pos=0;near.reset();far.reset();}
    void process(float& l,float& r){
        ring[0][pos]=std::isfinite(l)?l:0;ring[1][pos]=std::isfinite(r)?r:0;
        double a=0,b=0;for(size_t i=0;i<64;++i){const size_t k=(pos+64-i)%64;a+=double(ring[0][k])*head[0][i];b+=double(ring[1][k])*head[1][i];}
        float nl,nr,fl,fr;near.process(l,r,nl,nr);far.process(l,r,fl,fr);pos=(pos+1)%64;
        l=float(a+nl+fl);r=float(b+nr+fr);
    }
};
}
