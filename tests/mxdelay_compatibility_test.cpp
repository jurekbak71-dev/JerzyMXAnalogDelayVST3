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
        check(std::abs(p.slot[0].algorithm-.75)<1e-6,"legacy algorithm index changed");
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
            MXDelayParams q;check(!readMXState(cut,q),"truncated state accepted");
        }
    }
    for(int version:{0,6}){Stream bad;bad.writeInt32(version);MXDelayParams p;check(!readMXState(bad,p),"unknown version accepted");}
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
