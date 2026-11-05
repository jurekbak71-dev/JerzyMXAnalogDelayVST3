#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cvstguitimer.h"
namespace JerzyAudio {
class MXDelayEditor : public VSTGUI::VST3Editor {
public:
    using VSTGUI::VST3Editor::VST3Editor;
    ~MXDelayEditor() override;
    void valueChanged(VSTGUI::CControl* control) override;
    void onMouseEvent(VSTGUI::MouseEvent& event, VSTGUI::CFrame* frame) override;
protected:
    VSTGUI::CView* verifyView(VSTGUI::CView*,const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*) override;
    void PLUGIN_API close() override;
    bool PLUGIN_API open(void* parent,const VSTGUI::PlatformType& type) override;
#ifdef VST3_CONTENT_SCALE_SUPPORT
    Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override;
#endif
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rect) override;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* rect) override;
    bool beforeSizeChange(const VSTGUI::CRect& newSize,const VSTGUI::CRect& oldSize) override;
private:
    struct NativeResizeWatcher;
    NativeResizeWatcher* nativeWatcher=nullptr;
    bool applyingSize=false;
    double userZoom=1.0;
    int consolePage=0; // Editor-local navigation; never an audio parameter or preset field.
    void selectConsolePage(int page);
    void fitHostSize(const Steinberg::ViewRect& size);
    void applyZoom(double factor);
    void refreshIRStatus();
    VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> irStatusTimer;
    VSTGUI::SharedPointer<VSTGUI::CNewFileSelector> irSelector;
};
}
