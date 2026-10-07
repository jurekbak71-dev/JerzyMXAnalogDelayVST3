#include "mxdelay_gui_views.h"
#include "vstgui/uidescription/uiviewfactory.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/iviewcreator.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cfont.h"
#include <algorithm>
#include <cmath>

using namespace VSTGUI;
namespace JerzyAudio {
namespace {
const CColor amber(241,166,73),cream(239,226,196),black(8,18,18),edge(112,132,117);
void ellipse(CDrawContext* c,double x,double y,double radius,CColor fill,CColor frame){
    c->setFillColor(fill); c->setFrameColor(frame); c->setLineWidth(1.0);
    c->drawEllipse({x-radius,y-radius,x+radius,y+radius},kDrawFilledAndStroked);
}
void gradientRect(CDrawContext* c,const CRect& r,CColor a,CColor b){
    auto* path=c->createGraphicsPath();
    if(!path)return;
    path->addRect(r);
    auto* gradient=CGradient::create(0,1,a,b);
    if(gradient){ c->fillLinearGradient(path,*gradient,{r.left,r.top},{r.left,r.bottom}); gradient->forget(); }
    path->forget();
}
void text(CDrawContext* c,const char* s,const CRect& r,double size,CColor color){
    c->setFont(kNormalFont,size); c->setFontColor(color); c->drawString(s,r,kCenterText);
}
void metalKnob(CDrawContext* c,const CRect& r,double value){
    const double x=r.getCenter().x,y=r.getCenter().y;
    const double radius=std::min(r.getWidth(),r.getHeight())*0.36;
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);

    // Amber scale dots / ticks, like a high-end hardware pedal.
    for(int i=0;i<=20;++i){
        const double a=(135.0+13.5*i)*Constants::pi/180.0;
        const double rr=(i%5==0?1.8:1.15);
        ellipse(c,x+std::cos(a)*radius*1.34,y+std::sin(a)*radius*1.34,rr,
                i%5==0?amber:CColor(205,150,74),CColor(80,57,27));
    }

    // Deep drop shadow.
    ellipse(c,x+2.8,y+4.2,radius+3.2,CColor(0,0,0,150),CColor(0,0,0,80));

    // Knurled black outer ring.
    ellipse(c,x,y,radius+1.7,CColor(17,20,18),CColor(125,112,88));
    for(int i=0;i<28;++i){
        const double a=2.0*Constants::pi*i/28.0;
        const double x1=x+radius*.92*std::cos(a),y1=y+radius*.92*std::sin(a);
        const double x2=x+radius*1.10*std::cos(a),y2=y+radius*1.10*std::sin(a);
        c->setFrameColor(i%2?CColor(31,32,29):CColor(77,72,61));c->setLineWidth(1.0);
        c->drawLine({x1,y1},{x2,y2});
    }

    // Brushed metal cap rendered as radial wedges.
    const double cap=radius*.78;
    CDrawContext::PointList wedge;
    for(int i=0;i<144;++i){
        const double a=2.0*Constants::pi*i/144.0;
        const double next=a+2.0*Constants::pi/144.0;
        const double light=.58+.30*std::cos(a-.75)+.10*std::cos(5*a);
        const auto shade=static_cast<uint8_t>(std::clamp(70.0+145.0*light,72.0,225.0));
        c->setFillColor(CColor(shade,static_cast<uint8_t>(shade*.97),static_cast<uint8_t>(shade*.88)));
        wedge={{x,y},{x+cap*std::cos(a),y+cap*std::sin(a)},
                       {x+cap*std::cos(next),y+cap*std::sin(next)}};
        c->drawPolygon(wedge,kDrawFilled);
    }
    ellipse(c,x,y,cap,CColor(0,0,0,0),CColor(222,210,183));
    ellipse(c,x-cap*.25,y-cap*.28,cap*.20,CColor(255,255,255,34),CColor(255,255,255,0));

    // Pointer.
    const double a=(135.0+270.0*std::clamp(value,0.0,1.0))*Constants::pi/180.0;
    c->setFrameColor(CColor(255,232,185));c->setLineWidth(std::max(2.0,radius*.075));
    c->drawLine({x+radius*.18*std::cos(a),y+radius*.18*std::sin(a)},
                {x+radius*.78*std::cos(a),y+radius*.78*std::sin(a)});
}

ChickenKnob::ChickenKnob(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){
    setStartAngle(static_cast<float>(Constants::pi*0.75));
    setRangeAngle(static_cast<float>(Constants::pi*1.5));
    setWheelInc(0.01f);
}
void ChickenKnob::draw(CDrawContext* c){metalKnob(c,getViewSize(),getValueNormalized());setDirty(false);}
CMouseEventResult ChickenKnob::onMouseDown(CPoint& where,const CButtonState& buttons){
    if(buttons.isRightButton()){
        beginEdit();setValue(getDefaultValue());valueChanged();invalid();endEdit();
        return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
    }
    return CKnob::onMouseDown(where,buttons);
}

AnalogMeter::AnalogMeter(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){
    setMouseEnabled(false);
}
void AnalogMeter::draw(CDrawContext* c){
    const CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    c->setFillColor(black);c->setFrameColor(edge);c->setLineWidth(2);
    c->drawRect(r,kDrawFilledAndStroked);
    CRect face(r);face.inset(6,6);
    gradientRect(c,face,CColor(157,90,35),CColor(255,225,154));
    const double cx=face.getCenter().x,cy=face.bottom-7;
    const double radius=std::min(face.getWidth()*0.48,face.getHeight()*0.86);
    for(int i=0;i<=10;++i){
        const double a=(210+12*i)*Constants::pi/180.0;
        c->setFrameColor(i>=8?CColor(191,47,25):CColor(61,37,18));c->setLineWidth(i%2?1:1.6);
        c->drawLine({cx+radius*0.77*std::cos(a),cy+radius*0.77*std::sin(a)},
                    {cx+radius*0.94*std::cos(a),cy+radius*0.94*std::sin(a)});
    }
    text(c,"-20     -10      -3      0   +3",{face.left,face.top+6,face.right,face.top+19},9,CColor(60,34,14));
    text(c,"VU",{face.left,face.bottom-27,face.right,face.bottom-11},13,CColor(60,34,14));
    const double a=(210+120*std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0))*Constants::pi/180.0;
    c->setFrameColor(CColor(81,27,15));c->setLineWidth(2);
    c->drawLine({cx,cy},{cx+radius*0.87*std::cos(a),cy+radius*0.87*std::sin(a)});
    ellipse(c,cx,cy,3,CColor(63,38,18),CColor(63,38,18));setDirty(false);
}

ToggleSwitch::ToggleSwitch(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMin(0);setMax(1);setWheelInc(1);}
CMouseEventResult ToggleSwitch::onMouseDown(CPoint&,const CButtonState& buttons){
    if(buttons.isRightButton()){
        beginEdit();setValue(getDefaultValue());valueChanged();invalid();endEdit();
        return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
    }
    if(!buttons.isLeftButton())return kMouseEventNotHandled;
    beginEdit();setValueNormalized(getValueNormalized()<0.5f?1.f:0.f);valueChanged();invalid();endEdit();
    return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}
void ToggleSwitch::draw(CDrawContext* c){
    const CRect r(getViewSize());const auto p=r.getCenter();
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    ellipse(c,p.x,p.y,15,CColor(91,80,63),edge);
    ellipse(c,p.x,p.y,11,black,cream);
    const double end=p.y+(getValueNormalized()>=0.5f?-11:11);
    c->setLineWidth(7);c->setFrameColor(CColor(182,167,142));c->drawLine(p,{p.x,end});
    ellipse(c,p.x,end,6,CColor(221,207,179),edge);setDirty(false);
}
ThreeWaySwitch::ThreeWaySwitch(const CRect& r,IControlListener* l,int32_t tag):ToggleSwitch(r,l,tag){setMax(2);}
CMouseEventResult ThreeWaySwitch::onMouseDown(CPoint& where,const CButtonState& buttons){
    if(buttons.isRightButton())return ToggleSwitch::onMouseDown(where,buttons);
    if(!buttons.isLeftButton())return kMouseEventNotHandled;
    const int current=static_cast<int>(std::lround(getValueNormalized()*2.f));
    beginEdit();setValueNormalized(static_cast<float>((current+1)%3)*0.5f);valueChanged();invalid();endEdit();
    return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}
void ThreeWaySwitch::draw(CDrawContext* c){
    const CRect r(getViewSize());const auto p=r.getCenter();
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    ellipse(c,p.x,p.y,15,CColor(91,80,63),edge);ellipse(c,p.x,p.y,11,black,cream);
    const double end=p.y+11*(1.0-2.0*getValueNormalized());
    c->setLineWidth(7);c->setFrameColor(CColor(182,167,142));c->drawLine(p,{p.x,end});
    ellipse(c,p.x,end,6,CColor(221,207,179),edge);setDirty(false);
}
void LedToggleSwitch::draw(CDrawContext* c){
    ToggleSwitch::draw(c);
    const auto r=getViewSize();const bool on=getValueNormalized()>=0.5f;
    ellipse(c,r.left+8,r.top+8,5,on?CColor(255,194,73):CColor(42,58,64),CColor(18,25,28));
    if(on)ellipse(c,r.left+8,r.top+8,2.4,CColor(255,236,166),CColor(255,236,166));
    setDirty(false);
}
void LedThreeWaySwitch::draw(CDrawContext* c){
    ThreeWaySwitch::draw(c);
    const auto r=getViewSize();const int pos=std::clamp((int)std::lround(getValueNormalized()*2.f),0,2);
    for(int i=0;i<3;++i){
        const double x=r.left+8.0+i*10.0;
        ellipse(c,x,r.top+8,3.6,i==pos?CColor(92,220,235):CColor(33,51,58),CColor(15,23,27));
    }
    setDirty(false);
}
void BypassButton::draw(CDrawContext* c){
    CRect r(getViewSize());const double x=r.getCenter().x,y=r.getCenter().y+7;
    const double radius=std::min(r.getWidth(),r.getHeight())*0.29;
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    const bool bypass=getValueNormalized()>=0.5f;
    ellipse(c,x,r.top+9,7,CColor(85,41,15),edge);
    ellipse(c,x,r.top+9,4,bypass?CColor(255,193,71):CColor(63,42,24),edge);
    metalKnob(c,{x-radius*1.25,y-radius*1.25,x+radius*1.25,y+radius*1.25},0.5);
    setDirty(false);
}
HardwarePanel::HardwarePanel(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void HardwarePanel::draw(CDrawContext* c){
    const CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    gradientRect(c,r,CColor(16,44,43),CColor(7,24,25));
    c->setFrameColor(CColor(181,142,82));c->setLineWidth(1.2);c->drawRect(r,kDrawStroked);
    CRect inner(r);inner.inset(4,4);
    c->setFrameColor(CColor(88,128,120));c->setLineWidth(1.0);c->drawRect(inner,kDrawStroked);
    const CPoint screws[4]={{r.left+10,r.top+10},{r.right-10,r.top+10},{r.left+10,r.bottom-10},{r.right-10,r.bottom-10}};
    for(const auto& p:screws){
        ellipse(c,p.x+1,p.y+1,5.5,CColor(0,0,0,120),CColor(0,0,0,80));
        ellipse(c,p.x,p.y,4.8,CColor(114,113,100),CColor(210,198,166));
        c->setFrameColor(CColor(37,36,32));c->setLineWidth(1.2);c->drawLine({p.x-3,p.y+2},{p.x+3,p.y-2});
    }
    setDirty(false);
}
OxidizedPanel::OxidizedPanel(const CRect& r,IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){setMouseEnabled(false);}
void OxidizedPanel::draw(CDrawContext* c){
    const CRect r(getViewSize());
    c->setDrawMode(kAntiAliasing|kNonIntegralMode);
    gradientRect(c,r,CColor(18,66,64),CColor(5,27,29));
    for(int i=0;i<70;++i){
        const double y=r.top+(i+.5)*r.getHeight()/70.0;
        c->setFrameColor(i%3?CColor(154,202,191,11):CColor(236,196,115,8));c->setLineWidth(.6);
        c->drawLine({r.left+4,y},{r.right-4,y});
    }
    for(int i=0;i<55;++i){
        const double fx=.5+.49*std::sin(5.71*i+.4),fy=.5+.48*std::sin(9.37*i+1.1);
        const double rr=.6+1.6*(.5+.5*std::sin(2.9*i));
        ellipse(c,r.left+fx*r.getWidth(),r.top+fy*r.getHeight(),rr,CColor(190,220,208,18),CColor(0,0,0,0));
    }
    c->setFrameColor(CColor(205,219,199,100));c->setLineWidth(2.0);c->drawRect(r,kDrawStroked);
    CRect inner(r);inner.inset(5,5);c->setFrameColor(CColor(7,18,20));c->setLineWidth(2.0);c->drawRect(inner,kDrawStroked);
    setDirty(false);
}
namespace {
template<class T> class SimpleCreator : public ViewCreatorAdapter {
public:
    SimpleCreator(const char* name):name(name){UIViewFactory::registerViewCreator(*this);}
    IdStringPtr getViewName() const override{return name;}
    IdStringPtr getBaseViewName() const override{return "CKnob";}
    CView* create(const UIAttributes&,const IUIDescription*) const override{return new T(CRect(0,0,100,100),nullptr,-1);}
private:const char* name;
};
SimpleCreator<ChickenKnob> knob("ChickenKnob");SimpleCreator<AnalogMeter> meter("AnalogMeter");
SimpleCreator<ToggleSwitch> toggle("ToggleSwitch");SimpleCreator<ThreeWaySwitch> three("ThreeWaySwitch");
SimpleCreator<LedToggleSwitch> ledToggle("LedToggleSwitch");SimpleCreator<LedThreeWaySwitch> ledThree("LedThreeWaySwitch");
SimpleCreator<BypassButton> bypass("BypassButton");SimpleCreator<HardwarePanel> panel("HardwarePanel");SimpleCreator<OxidizedPanel> oxidePanel("OxidizedPanel");
}
void registerMXDelayViews(){}
}
