#include "mxdelay_state.h"
#include "mxdelay_editor_geometry.h"
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace JerzyAudio;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
// Explicit little-endian bytes, independent of the production IBStreamer wrapper.
struct Stream {
    std::vector<unsigned char> bytes; size_t cursor=0;
    bool writeInt32(int32_t value){for(int i=0;i<4;++i)bytes.push_back(static_cast<unsigned char>(uint32_t(value)>>(i*8)));return true;}
    bool readInt32(int32_t& value){if(cursor+4>bytes.size())return false;uint32_t u=0;for(int i=0;i<4;++i)u|=uint32_t(bytes[cursor++])<<(i*8);std::memcpy(&value,&u,4);return true;}
    bool writeFloat(float value){int32_t bits;std::memcpy(&bits,&value,4);return writeInt32(bits);}
    bool readFloat(float& value){int32_t bits;if(!readInt32(bits))return false;std::memcpy(&value,&bits,4);return true;}
};
Stream legacy(int version){
    Stream b;b.writeInt32(version);
    for(float x:{.2f,.5f,.6f,1.f,0.f,1.f})b.writeFloat(x);
    if(version>=3)b.writeFloat(.75f);
    if(version>=5){for(int fx=0;fx<4;++fx)for(float x:{.4f,.3f,.2f,.1f,.6f,1.f})b.writeFloat(x);}
    else if(version==4){for(float x:{.4f,.3f,.2f,.1f,.6f,1.f})b.writeFloat(x);}
    for(int slot=0;slot<2;++slot){
        // Last of the original seven algorithms must remain DIG after migration.
        for(float x:{version<4?1.f:.75f,1.f,0.f,.5f,.2f,.42f,.78f,.5f,0.f})b.writeFloat(x);
        for(int a=0;a<(version<4?7:9);++a)for(int c=0;c<16;++c)b.writeFloat(float((a*16+c)%100)/100.f);
        if(version>=2)for(float pan:{.1f,.3f,.7f,.9f})b.writeFloat(pan);
    }
    return b;
}
void stateChecks(){
    for(int version=1;version<=5;++version){
        auto fixture=legacy(version);MXDelayParams p;
        check(readMXState(fixture,p),"legacy state rejected");
        check(fixture.cursor==fixture.bytes.size(),"legacy byte layout changed");
        check(std::abs(p.slot[0].algorithm-(6.0/11.0))<1e-6,"legacy algorithm index changed");
        check(std::abs(p.outputTrim-.6)<1e-6,"legacy gain changed");
        check(std::abs(p.slot[1].c[6][15]-.11)<1e-6,"legacy algorithm controls lost");
        check(std::abs(p.slot[0].headPan[0]-(version>=2?.1:.125))<1e-6,"head pan migration changed");
        check(p.machineCondition==(version>=3?.75:0.),"machine condition migration changed");
        check(p.fx[1].type==(version>=5?double(.4f):0.),"FX migration changed");
        if(version==4)check(p.fx[0].route==0.,"v4 route inversion changed");
        Stream saved;check(writeMXState(saved,p),"save failed");
        MXDelayParams restored;check(readMXState(saved,restored),"round trip rejected");
        Stream again;check(writeMXState(again,restored)&&again.bytes==saved.bytes,"round trip bytes changed");
        for(size_t length=0;length<fixture.bytes.size();++length){
            Stream cut;cut.bytes.assign(fixture.bytes.begin(),fixture.bytes.begin()+length);
            MXDelayParams q;q.mix=.873;q.reverb.wet=.613;check(!readMXState(cut,q),"truncated state accepted");check(q.mix==.873&&q.reverb.wet==.613,"truncated preset partially changed live state");
        }
    }
    {
        MXDelayParams p;p.reverb.enable=1;p.reverb.mode=1;p.reverb.external=1;p.reverb.irPath="C:/IR/room-utf8-\xc5\x82.wav";p.reverb.irLength=.31;p.reverb.normalize=0;p.reverb.lowCut=.4;p.reverb.diffusion=.72;p.reverb.early=.21;p.reverb.hybrid=.37;p.reverb.irDamping=.63;
        Stream saved;check(writeMXState(saved,p),"v9 save failed");MXDelayParams q;check(readMXState(saved,q),"v9 reload failed");check(q.reverb.irPath==p.reverb.irPath&&std::abs(q.reverb.hybrid-.37)<1e-6&&std::abs(q.reverb.irDamping-.63)<1e-6&&q.reverb.external==1,"v9 reverb settings lost");
        for(size_t n=0;n<saved.bytes.size();++n){Stream cut;cut.bytes.assign(saved.bytes.begin(),saved.bytes.begin()+n);MXDelayParams unchanged;unchanged.reverb.irPath="unchanged";check(!readMXState(cut,unchanged)&&unchanged.reverb.irPath=="unchanged","v9 truncation altered live state");}
        // v7/v8 used 12 algorithms: v8 appended mode/profile to the v7 tail.
        for(int version:{7,8}){Stream old=saved;old.bytes[0]=version;old.bytes.resize(saved.bytes.size()-4*p.reverb.irPath.size()-4-8*4-(version==7?8:0));old.cursor=0;MXDelayParams restored;check(readMXState(old,restored)&&restored.reverb.irPath.empty(),"v7/v8 migration failed");}
    }
    {auto old=legacy(5);old.bytes[0]=6;for(int i=0;i<7;++i)old.writeFloat(.4f);MXDelayParams restored;check(readMXState(old,restored)&&std::abs(restored.reverb.decay-.4)<1e-6,"v6 migration failed");}
    for(int version:{0,kMXStateVersion+1}){Stream bad;bad.writeInt32(version);MXDelayParams p;check(!readMXState(bad,p),"unknown version accepted");}
    auto bad=legacy(5);float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(bad.bytes.data()+4,&nan,4);
    MXDelayParams p;check(!readMXState(bad,p),"non-finite state accepted");
}
void geometryChecks(){
    for(double dpi:{1.,1.25,1.5,2.})for(double zoom:{.75,1.,1.25,1.5}){
        auto w=MXDelayEditorGeometry::pixelWidth(zoom,dpi),h=MXDelayEditorGeometry::pixelHeight(zoom,dpi);
        check(std::abs(double(w)/h-1280./790.)<.003,"aspect ratio drift");
        check(std::abs(MXDelayEditorGeometry::zoomForWidth(w,dpi)-zoom)<.002,"DPI zoom mismatch");
    }
    check(MXDelayEditorGeometry::dpi(0)==1.,"invalid DPI fallback changed");
}
}
int main(){try{stateChecks();geometryChecks();std::cout<<"State v1-v5, byte roundtrip, all truncations, invalid states and zoom/DPI passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
