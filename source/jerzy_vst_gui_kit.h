#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/ccolor.h"
#include "jerzy_gui_assets.h"
#include "vstgui/lib/controls/cbuttons.h"

namespace JerzyAudio {

struct JerzyGuiTheme {
    VSTGUI::CColor chassisTop;
    VSTGUI::CColor chassisBottom;
    VSTGUI::CColor panelTop;
    VSTGUI::CColor panelBottom;
    VSTGUI::CColor accent;
    VSTGUI::CColor text;
    VSTGUI::CColor edge;
    VSTGUI::CColor ledOn;
    VSTGUI::CColor display;
};

const JerzyGuiTheme& jerzyThemePetrol();
const JerzyGuiTheme& jerzyThemeBrass();
const JerzyGuiTheme& jerzyThemeSteel();
const JerzyGuiTheme& jerzyThemeConsole();
const JerzyGuiTheme& jerzyThemeBurgundy();
const JerzyGuiTheme& jerzyThemeViolet();

void setJerzyGuiTheme(const JerzyGuiTheme&);
const JerzyGuiTheme& getJerzyGuiTheme();

class JerzyKnob : public VSTGUI::CKnob {
public:
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    JerzyKnob(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
};

// Segmented horizontal peak meter. The control value is linear amplitude (0..1).
class JerzyLedBarMeter : public VSTGUI::CKnob {
public:
    JerzyLedBarMeter(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyConsoleButton : public VSTGUI::CTextButton {
public:
    using VSTGUI::CTextButton::CTextButton;
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzySignalLED : public VSTGUI::CKnob {
public:
    JerzySignalLED(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t t):CKnob(r,l,t,nullptr,nullptr){setMouseEnabled(false);}
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyAnalogMeter : public VSTGUI::CKnob {
public:
    JerzyAnalogMeter(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyToggle : public VSTGUI::CKnob {
    float visualPosition=-1.f;
public:
    void onIdle() override;
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    JerzyToggle(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
};

class JerzyThreeWay : public JerzyToggle {
public:
    JerzyThreeWay(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,const VSTGUI::CButtonState&) override;
};

class JerzyLedToggle : public JerzyToggle {
public:
    using JerzyToggle::JerzyToggle;
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyLedThreeWay : public JerzyThreeWay {
public:
    using JerzyThreeWay::JerzyThreeWay;
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyBypassButton : public JerzyToggle {
public:
    using JerzyToggle::JerzyToggle;
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyHardwarePanel : public VSTGUI::CKnob {
public:
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    JerzyHardwarePanel(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyGlassPanel : public JerzyHardwarePanel {
public:
    using JerzyHardwarePanel::JerzyHardwarePanel;
    void draw(VSTGUI::CDrawContext*) override;
};

// Recessed, screw-free surface for densely populated algorithm modules.
class JerzyConsoleInsetPanel : public JerzyHardwarePanel {
public:
    using JerzyHardwarePanel::JerzyHardwarePanel;
    void draw(VSTGUI::CDrawContext*) override;
};

class JerzyChassis : public VSTGUI::CKnob {
public:
    std::shared_ptr<Jerzy3DAssets> guiAssets=acquireJerzy3DAssets();
    JerzyChassis(const VSTGUI::CRect&,VSTGUI::IControlListener*,int32_t);
    void draw(VSTGUI::CDrawContext*) override;
};

} // namespace JerzyAudio
