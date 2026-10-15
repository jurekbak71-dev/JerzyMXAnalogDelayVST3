#pragma once
#include "vstgui/plugin-bindings/vst3editor.h"

namespace JerzyAudio {
// Use VSTGUI's official VST3 HWND lifecycle, avoiding host-owned HWND
// subclassing, polling, forced redraw/resize and manual frame transforms.
class MXDelayEditor : public VSTGUI::VST3Editor {
public:
    using VSTGUI::VST3Editor::VST3Editor;
    void valueChanged(VSTGUI::CControl* control) override;
    void onMouseEvent(VSTGUI::MouseEvent& event,VSTGUI::CFrame* frame) override;
#ifdef VST3_CONTENT_SCALE_SUPPORT
    Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override;
#endif
};
}
