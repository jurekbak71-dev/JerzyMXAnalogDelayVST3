#include "mxdelay_gui_views.h"
#include "vstgui/uidescription/uiviewfactory.h"
#include "vstgui/uidescription/uiviewcreator.h"
#include "vstgui/uidescription/iviewcreator.h"

using namespace VSTGUI;
namespace JerzyAudio {
namespace {
template<class T> class SimpleCreator : public ViewCreatorAdapter {
public:
    explicit SimpleCreator(const char* n):name(n){UIViewFactory::registerViewCreator(*this);}
    IdStringPtr getViewName() const override{return name;}
    IdStringPtr getBaseViewName() const override{return "CKnob";}
    CView* create(const UIAttributes&,const IUIDescription*) const override{
        return new T(CRect(0,0,100,100),nullptr,-1);
    }
private:
    const char* name;
};

class ConsoleButtonCreator : public ViewCreatorAdapter {
public:
    ConsoleButtonCreator(){UIViewFactory::registerViewCreator(*this);}
    IdStringPtr getViewName()const override{return "ConsoleButton";}
    IdStringPtr getBaseViewName()const override{return "CTextButton";}
    CView* create(const UIAttributes&,const IUIDescription*)const override{return new JerzyConsoleButton(CRect(0,0,100,32));}
} consoleButtonCreator;

SimpleCreator<ChickenKnob> knob("ChickenKnob");
SimpleCreator<AnalogMeter> meter("AnalogMeter");
SimpleCreator<JerzyLedBarMeter> ledBar("LedBarMeter");
SimpleCreator<ToggleSwitch> toggle("ToggleSwitch");
SimpleCreator<ThreeWaySwitch> three("ThreeWaySwitch");
SimpleCreator<LedToggleSwitch> ledToggle("LedToggleSwitch");
SimpleCreator<LedThreeWaySwitch> ledThree("LedThreeWaySwitch");
SimpleCreator<BypassButton> bypass("BypassButton");
SimpleCreator<JerzyGlassPanel> glassPanel("GlassPanel");
SimpleCreator<HardwarePanel> panel("HardwarePanel");
SimpleCreator<ConsoleInsetPanel> insetPanel("ConsoleInsetPanel");
SimpleCreator<OxidizedPanel> oxidePanel("OxidizedPanel");
}

void registerMXDelayViews(){
    setJerzyGuiTheme(jerzyThemeConsole());
}
} // namespace JerzyAudio
