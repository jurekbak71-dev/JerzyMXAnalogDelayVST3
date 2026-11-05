#pragma once
#include "vstgui/lib/cbitmap.h"
#include <memory>
namespace JerzyAudio {
struct Jerzy3DAssets {
    VSTGUI::SharedPointer<VSTGUI::CBitmap> knob,smallKnob,shadow,amber,cyan,scaleAmber,scaleCyan,toggle,panel,inset,chassis,screw,glass,buttonOff,buttonAmber,buttonCyan,ledOff,ledAmber,ledCyan,ledGreen;
    Jerzy3DAssets();
    bool loaded() const;
};
std::shared_ptr<Jerzy3DAssets> acquireJerzy3DAssets();
void drawJerzyAsset(VSTGUI::CDrawContext*,VSTGUI::CBitmap*,const VSTGUI::CRect&,int frame=0,int frames=1,float alpha=1.f);
}
