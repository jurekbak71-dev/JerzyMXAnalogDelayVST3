#include "jerzy_vst_gui_kit.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cfont.h"
#include <algorithm>
#include <cmath>

using namespace VSTGUI;
namespace JerzyAudio {
namespace {

JerzyGuiTheme activeTheme = jerzyThemePetrol();

void ellipse(CDrawContext* c,double x,double y,double radius,CColor fill,CColor frame){
    c->setFillColor(fill); c->setFrameColor(frame); c->setLineWidth(1.0);
    c->drawEllipse({x-radius,y-radius,x+radius,y+radius},kDrawFilledAndStroked);
}
void gradientRect(CDrawContext* c,const CRect& r,CColor a,CColor b){
    auto* path=c->createGraphicsPath();
    if(!path)return;
    path->addRect(r);
    auto* gradient=CGradient::create(0,1,a,b);
    if(gradient){c->fillLinearGradient(path,*gradient,{r.left,r.top},{r.left,r.bottom});gradient->forget();}
    path->forget();
}
void label(CDrawContext* c,const char* s,const CRect& r,double size,CColor color){
    c->setFont(kNormalFont,size);c->setFontColor(color);c->drawString(s,r,kCenterText);
}

} // anonymous

const JerzyGuiTheme& jerzyThemePetrol(){
    static const JerzyGuiTheme t{
        CColor(18,66,64),CColor(5,27,29),CColor(16,44,43),CColor(7,24,25),
        CColor(241,166,73),CColor(239,226,196),CColor(112,132,117),CColor(68,220,96),CColor(241,166,73)};
    return t;
}
const JerzyGuiTheme& jerzyThemeBrass(){
    static const JerzyGuiTheme t{
        CColor(140,100,37),CColor(24,19,11),CColor(73,54,21),CColor(29,22,12),
        CColor(242,188,92),CColor(241,224,190),CColor(151,118,64),CColor(68,220,96),CColor(242,188,92)};
    return t;
}
const JerzyGuiTheme& jerzyThemeSteel(){
    static const JerzyGuiTheme t{
        CColor(57,74,88),CColor(13,20,25),CColor(32,44,53),CColor(14,23,29),
        CColor(115,200,232),CColor(216,229,233),CColor(117,142,156),CColor(83,220,118),CColor(115,200,232)};
    return t;
}
const JerzyGuiTheme& jerzyThemeConsole(){
    static const JerzyGuiTheme t{
        CColor(83,84,86),CColor(29,30,32),CColor(47,48,50),CColor(15,16,18),
        CColor(241,166,73),CColor(232,237,240),CColor(139,144,148),CColor(68,220,96),CColor(241,166,73)};
    return t;
}
const JerzyGuiTheme& jerzyThemeBurgundy(){
    static const JerzyGuiTheme t{
        CColor(110,37,33),CColor(22,12,10),CColor(53,24,21),CColor(28,13,12),
        CColor(230,148,67),CColor(236,215,185),CColor(139,80,61),CColor(85,220,110),CColor(230,148,67)};
    return t;
}
const JerzyGuiTheme& jerzyThemeViolet(){
    static const JerzyGuiTheme t{
        CColor(65,56,109),CColor(16,14,30),CColor(36,32,63),CColor(22,19,40),
        CColor(169,148,237),CColor(229,224,241),CColor(105,93,148),CColor(89,222,133),CColor(104,198,216)};
    return t;
}
void setJerzyGuiTheme(const JerzyGuiTheme& t){activeTheme=t;}
const JerzyGuiTheme& getJerzyGuiTheme(){return activeTheme;}

JerzyKnob::JerzyKnob(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){
    setStartAngle(static_cast<float>(Constants::pi*.75));setRangeAngle(static_cast<float>(Constants::pi*1.5));setWheelInc(.01f);
}
void JerzyKnob::draw(CDrawContext* c){
    const CRect r(getViewSize());const auto tag=getTag();const bool b=tag>=1400&&tag<1700;
    const auto& a=*guiAssets;
    drawJerzyAsset(c,b?a.scaleCyan.get():a.scaleAmber.get(),r);
    drawJerzyAsset(c,a.shadow,r);
    drawJerzyAsset(c,b?a.cyan.get():a.amber.get(),r,0,1,.12f+.25f*getValueNormalized());
    drawJerzyAsset(c,r.getWidth()<60?a.smallKnob.get():a.knob.get(),r,int(std::lround(std::clamp(getValueNormalized(),0.f,1.f)*100)),101);
    // Fixed reference marker: centred for signed gain/pan, minimum for unipolar controls.
    const bool centre=tag==1001||tag==1002||tag==1107||tag==1407||(tag>=1110&&tag<=1113)||(tag>=1410&&tag<=1413);
    const double n=tag==1002?.6:centre?.5:0.;const double angle=(135+270*n)*Constants::pi/180.;
    const auto p=r.getCenter();const double radius=std::min(r.getWidth(),r.getHeight())*.49;
    c->setFrameColor(CColor(237,238,220));c->setLineWidth(2);
    c->drawLine({p.x+radius*.87*std::cos(angle),p.y+radius*.87*std::sin(angle)},{p.x+radius*std::cos(angle),p.y+radius*std::sin(angle)});
    setDirty(false);
}
CMouseEventResult JerzyKnob::onMouseDown(CPoint& where,const CButtonState& buttons){
    if(buttons.isRightButton()){beginEdit();setValue(getDefaultValue());valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
    return CKnob::onMouseDown(where,buttons);
}

void JerzyConsoleButton::draw(CDrawContext* c){
    const bool on=getValueNormalized()>.5f;
    drawJerzyAsset(c,on?(getTag()==9201?guiAssets->buttonCyan.get():guiAssets->buttonAmber.get()):guiAssets->buttonOff.get(),getViewSize());
    c->setFont(getFont());c->setFontColor(on?getTextColorHighlighted():getTextColor());
    c->drawString(getTitle().data(),getViewSize(),kCenterText);setDirty(false);
}

JerzyLedBarMeter::JerzyLedBarMeter(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void JerzyLedBarMeter::draw(CDrawContext* c){
    const CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    c->setFillColor(CColor(4,10,13));c->setFrameColor(CColor(71,82,84));c->setLineWidth(1);
    c->drawRect(r,kDrawFilledAndStroked);
    constexpr int count=24;
    const double pad=4.0,gap=2.0;
    const bool vertical=r.getHeight()>r.getWidth();
    const double length=vertical?r.getHeight():r.getWidth();
    const double width=std::max(1.0,(length-2*pad-(count-1)*gap)/count);
    const double amplitude=std::clamp(double(getValueNormalized()),0.0,1.0);
    // 60 dB display range; linear amplitude enters from the audio processor.
    const double db=20.0*std::log10(std::max(1e-9,amplitude));
    const int lit=amplitude>0?std::clamp(int(std::ceil((db+60.0)/60.0*count)),0,count):0;
    for(int n=0;n<count;++n){
        const double x=r.left+pad+n*(width+gap);
        const double y=r.bottom-pad-(n+1)*width-n*gap;
        const CRect led=vertical?CRect(r.left+4,y,r.right-4,y+width):CRect(x,r.top+4,x+width,r.bottom-4);
        const bool on=n<lit;
        const CColor color=n<17?(on?CColor(56,231,107):CColor(14,48,31)):
            n<21?(on?CColor(255,188,53):CColor(57,39,16)):
                  (on?CColor(255,68,46):CColor(62,20,16));
        c->setFillColor(color);c->drawRect(led,kDrawFilled);
    }
    setDirty(false);
}

JerzyAnalogMeter::JerzyAnalogMeter(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void JerzyAnalogMeter::draw(CDrawContext* c){
    const auto& t=getJerzyGuiTheme();const CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);c->setFillColor(CColor(8,18,18));c->setFrameColor(t.edge);c->setLineWidth(2);c->drawRect(r,kDrawFilledAndStroked);
    CRect face(r);face.inset(6,6);gradientRect(c,face,CColor(157,90,35),CColor(255,225,154));
    const double cx=face.getCenter().x,cy=face.bottom-7,rad=std::min(face.getWidth()*.48,face.getHeight()*.86);
    for(int i=0;i<=10;++i){const double a=(210+12*i)*Constants::pi/180.0;c->setFrameColor(i>=8?CColor(191,47,25):CColor(61,37,18));c->setLineWidth(i%2?1:1.6);c->drawLine({cx+rad*.77*std::cos(a),cy+rad*.77*std::sin(a)},{cx+rad*.94*std::cos(a),cy+rad*.94*std::sin(a)});}
    label(c,"-20     -10      -3      0   +3",{face.left,face.top+6,face.right,face.top+19},9,CColor(60,34,14));label(c,"VU",{face.left,face.bottom-27,face.right,face.bottom-11},13,CColor(60,34,14));
    const double a=(210+120*std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0))*Constants::pi/180.0;c->setFrameColor(CColor(81,27,15));c->setLineWidth(2);c->drawLine({cx,cy},{cx+rad*.87*std::cos(a),cy+rad*.87*std::sin(a)});ellipse(c,cx,cy,3,CColor(63,38,18),CColor(63,38,18));setDirty(false);
}

JerzyToggle::JerzyToggle(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMin(0);setMax(1);setWheelInc(1);setWantsIdle(true);}
CMouseEventResult JerzyToggle::onMouseDown(CPoint&,const CButtonState& buttons){
    if(buttons.isRightButton()){beginEdit();setValue(getDefaultValue());valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;}
    if(!buttons.isLeftButton())return kMouseEventNotHandled;beginEdit();setValueNormalized(getValueNormalized()<.5f?1.f:0.f);valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}
void JerzyToggle::onIdle(){
    if(visualPosition<0)visualPosition=getValueNormalized();
    const float delta=getValueNormalized()-visualPosition;
    if(std::abs(delta)>.001f){visualPosition+=delta*.45f;invalid();}
    else visualPosition=getValueNormalized();
}
void JerzyToggle::draw(CDrawContext* c){
    if(visualPosition<0)visualPosition=getValueNormalized();
    drawJerzyAsset(c,guiAssets->toggle,getViewSize(),int(std::lround(visualPosition*16)),17);setDirty(false);
}
JerzyThreeWay::JerzyThreeWay(const CRect& r,IControlListener* l,int32_t tag):JerzyToggle(r,l,tag){setMax(2);}
CMouseEventResult JerzyThreeWay::onMouseDown(CPoint& where,const CButtonState& buttons){
    if(buttons.isRightButton())return JerzyToggle::onMouseDown(where,buttons);if(!buttons.isLeftButton())return kMouseEventNotHandled;const int current=static_cast<int>(std::lround(getValueNormalized()*2.f));beginEdit();setValueNormalized(static_cast<float>((current+1)%3)*.5f);valueChanged();invalid();endEdit();return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}
void JerzyThreeWay::draw(CDrawContext* c){JerzyToggle::draw(c);}
void JerzyLedToggle::draw(CDrawContext* c){JerzyToggle::draw(c);const auto& t=getJerzyGuiTheme();const auto r=getViewSize();const bool on=getValueNormalized()>=.5f;ellipse(c,r.left+8,r.top+8,5,on?t.ledOn:CColor(42,58,64),CColor(18,25,28));if(on)ellipse(c,r.left+8,r.top+8,2.4,CColor(255,255,210),CColor(255,255,210));setDirty(false);}
void JerzyLedThreeWay::draw(CDrawContext* c){JerzyThreeWay::draw(c);const auto& t=getJerzyGuiTheme();const auto r=getViewSize();const int pos=std::clamp((int)std::lround(getValueNormalized()*2.f),0,2);for(int i=0;i<3;++i)ellipse(c,r.left+8.0+i*10.0,r.top+8,3.6,i==pos?t.display:CColor(33,51,58),CColor(15,23,27));setDirty(false);}
void JerzyBypassButton::draw(CDrawContext* c){JerzyToggle::draw(c);const auto r=getViewSize();ellipse(c,r.left+7,r.top+7,3,getValueNormalized()>.5?CColor(255,75,40):CColor(40,48,42),CColor(8,10,11));}

JerzyHardwarePanel::JerzyHardwarePanel(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void JerzyHardwarePanel::draw(CDrawContext* c){
    const CRect r(getViewSize());drawJerzyAsset(c,guiAssets->panel,r);
    for(auto p:{CPoint(r.left+10,r.top+10),CPoint(r.right-10,r.top+10),CPoint(r.left+10,r.bottom-10),CPoint(r.right-10,r.bottom-10)})
        drawJerzyAsset(c,guiAssets->screw,CRect(p.x-6,p.y-6,p.x+6,p.y+6));
    setDirty(false);
}
void JerzyGlassPanel::draw(CDrawContext* c){drawJerzyAsset(c,guiAssets->inset,getViewSize());drawJerzyAsset(c,guiAssets->glass,getViewSize());setDirty(false);}
void JerzyConsoleInsetPanel::draw(CDrawContext* c){drawJerzyAsset(c,guiAssets->inset,getViewSize());setDirty(false);}
JerzyChassis::JerzyChassis(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void JerzyChassis::draw(CDrawContext* c){drawJerzyAsset(c,guiAssets->chassis,getViewSize());setDirty(false);}
} // namespace JerzyAudio
