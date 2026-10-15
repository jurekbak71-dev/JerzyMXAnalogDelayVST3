#include "mxdelay_editor.h"
#include "mxdelay_editor_geometry.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/controls/ccontrol.h"
#include <cmath>

namespace JerzyAudio {
#ifdef VST3_CONTENT_SCALE_SUPPORT
Steinberg::tresult PLUGIN_API MXDelayEditor::setContentScaleFactor(ScaleFactor factor) {
    if (!std::isfinite(factor) || factor <= 0)
        return Steinberg::kInvalidArgument;
    const auto result = VSTGUI::VST3Editor::setContentScaleFactor(factor);
    if (result == Steinberg::kResultOk && !getFrame())
        setRect(Steinberg::ViewRect(0,0,
            MXDelayEditorGeometry::pixelWidth(getZoomFactor(),factor),
            MXDelayEditorGeometry::pixelHeight(getZoomFactor(),factor)));
    return result;
}
#endif

void MXDelayEditor::onMouseEvent(VSTGUI::MouseEvent& event,VSTGUI::CFrame* frame){
    if(event.type==VSTGUI::EventType::MouseDown&&event.buttonState.isRight()&&frame){
        VSTGUI::CViewContainer::ViewList views;
        auto point=event.mousePosition;
        frame->getTransform().transform(point);
        if(frame->getViewsAt(point,views,VSTGUI::GetViewOptions().deep().includeViewContainer())){
            for(const auto& view:views){
                auto* control=view.cast<VSTGUI::CControl>();
                if(!control||!control->getMouseEnabled()||control->getTag()<0||control->getTag()>=9000)continue;
                control->beginEdit();
                control->setValue(control->getDefaultValue());
                control->valueChanged();
                control->invalid();
                control->endEdit();
                event.consumed=true;
                return;
            }
        }
    }
    VSTGUI::VST3Editor::onMouseEvent(event,frame);
}
void MXDelayEditor::valueChanged(VSTGUI::CControl* control){
    if(control&&control->getTag()>=9101&&control->getTag()<=9104){
        if(control->getValueNormalized()>0.5f){
            static const double zooms[]={0.75,1.0,1.25,1.5};
            setZoomFactor(zooms[control->getTag()-9101]);
        }
        return;
    }
    VSTGUI::VST3Editor::valueChanged(control);
}
}
