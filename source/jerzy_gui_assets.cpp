#include "jerzy_gui_assets.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicstransform.h"
#include <mutex>
namespace JerzyAudio {
using namespace VSTGUI;
namespace {
SharedPointer<CBitmap> load(const char* name){return owned(new CBitmap(CResourceDescription(name)));}
std::weak_ptr<Jerzy3DAssets> cache;
std::mutex cacheMutex; // GUI creation only. Never reached by the audio callback.
}
Jerzy3DAssets::Jerzy3DAssets():
    knob(load("gui3d/knob_192.png")),smallKnob(load("gui3d/knob_96.png")),shadow(load("gui3d/knob_shadow.png")),
    amber(load("gui3d/glow_amber.png")),cyan(load("gui3d/glow_cyan.png")),scaleAmber(load("gui3d/scale_amber.png")),scaleCyan(load("gui3d/scale_cyan.png")),
    toggle(load("gui3d/toggle.png")),panel(load("gui3d/panel.png")),inset(load("gui3d/inset.png")),chassis(load("gui3d/chassis.png")),screw(load("gui3d/screw.png")),glass(load("gui3d/glass.png")),buttonOff(load("gui3d/button_off.png")),buttonAmber(load("gui3d/button_amber.png")),buttonCyan(load("gui3d/button_cyan.png")){}
bool Jerzy3DAssets::loaded()const{return knob->isLoaded()&&smallKnob->isLoaded()&&toggle->isLoaded()&&panel->isLoaded()&&inset->isLoaded()&&chassis->isLoaded();}
std::shared_ptr<Jerzy3DAssets> acquireJerzy3DAssets(){
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto shared=cache.lock();if(!shared){shared=std::make_shared<Jerzy3DAssets>();cache=shared;}return shared;
}
void drawJerzyAsset(CDrawContext* c,CBitmap* bitmap,const CRect& r,int frame,int frames,float alpha){
    if(!bitmap||!bitmap->isLoaded()||r.getWidth()<=0||r.getHeight()<=0)return;
    const int columns=frames==101?11:1;
    const int rows=(frames+columns-1)/columns;
    const double width=bitmap->getWidth()/columns,height=bitmap->getHeight()/rows;
    CDrawContext::Transform transform(*c,CGraphicsTransform(r.getWidth()/width,0,0,r.getHeight()/height,r.left,r.top));
    bitmap->draw(c,CRect(0,0,width,height),CPoint((frame%columns)*width,(frame/columns)*height),alpha);
}
}
