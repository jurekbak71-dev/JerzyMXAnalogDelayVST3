// Native Linux/X11 smoke test. Windows host coverage remains in mxdelay_gui_test.cpp.
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "mxdelay_params.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <poll.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace Steinberg;using namespace Steinberg::Vst;using namespace JerzyAudio;
using Clock=std::chrono::steady_clock;
static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
class Frame final:public U::Implements<U::Directly<IPlugFrame,Linux::IRunLoop,IComponentHandler>> {
    struct FD {IPtr<Linux::IEventHandler> callback;int descriptor;};
    struct Timer {IPtr<Linux::ITimerHandler> callback;Linux::TimerInterval interval;Clock::time_point next;};
    std::vector<FD> descriptors;std::vector<Timer> timers;
public:
    Display* display=nullptr;Window window=0;int gestures=0,edits=0;
    tresult PLUGIN_API resizeView(IPlugView* view,ViewRect* rect)override{XResizeWindow(display,window,rect->getWidth(),rect->getHeight());XFlush(display);return view->onSize(rect);}
    tresult PLUGIN_API registerEventHandler(Linux::IEventHandler* h,int fd)override{descriptors.push_back({h,fd});return kResultOk;}
    tresult PLUGIN_API unregisterEventHandler(Linux::IEventHandler* h)override{descriptors.erase(std::remove_if(descriptors.begin(),descriptors.end(),[h](auto& e){return e.callback==h;}),descriptors.end());return kResultOk;}
    tresult PLUGIN_API registerTimer(Linux::ITimerHandler* h,Linux::TimerInterval ms)override{timers.push_back({h,ms,Clock::now()});return kResultOk;}
    tresult PLUGIN_API unregisterTimer(Linux::ITimerHandler* h)override{timers.erase(std::remove_if(timers.begin(),timers.end(),[h](auto& t){return t.callback==h;}),timers.end());return kResultOk;}
    tresult PLUGIN_API beginEdit(ParamID id)override{require(id<9000,"UI navigation began audio gesture");++gestures;return kResultOk;}
    tresult PLUGIN_API performEdit(ParamID id,ParamValue)override{require(id<9000,"UI navigation sent automation");++edits;return kResultOk;}
    tresult PLUGIN_API endEdit(ParamID)override{--gestures;return kResultOk;}
    tresult PLUGIN_API restartComponent(int32)override{return kResultOk;}
    void pump(){
        const auto until=Clock::now()+std::chrono::milliseconds(120);
        do {
            auto active=descriptors;std::vector<pollfd> polling;
            for(auto& fd:active)polling.push_back({fd.descriptor,POLLIN,0});
            poll(polling.data(),polling.size(),5);
            for(size_t i=0;i<polling.size();++i)if(polling[i].revents&POLLIN)active[i].callback->onFDIsSet(active[i].descriptor);
            auto time=Clock::now();std::vector<IPtr<Linux::ITimerHandler>> callbacks;
            for(auto& timer:timers)if(time>=timer.next){timer.next=time+std::chrono::milliseconds(timer.interval);callbacks.push_back(timer.callback);}
            for(auto& callback:callbacks)callback->onTimer();
            while(XPending(display)){XEvent e;XNextEvent(display,&e);}
        }while(Clock::now()<until);
    }
    void click(IPlugView* view,double x,double y,unsigned button=1){
        ViewRect rect;view->getSize(&rect);Window root,parent,*children=nullptr;unsigned count=0;
        require(XQueryTree(display,window,&root,&parent,&children,&count)&&count,"Editor has no X11 child");
        Window child=children[count-1];XFree(children);
        XEvent event{};event.xbutton.display=display;event.xbutton.window=child;event.xbutton.root=root;
        event.xbutton.x=int(std::lround(x*rect.getWidth()/1280.));event.xbutton.y=int(std::lround(y*rect.getHeight()/790.));event.xbutton.button=button;event.xbutton.same_screen=True;
        event.type=ButtonPress;XSendEvent(display,child,False,ButtonPressMask,&event);
        event.type=ButtonRelease;event.xbutton.state=button==1?Button1Mask:Button3Mask;XSendEvent(display,child,False,ButtonReleaseMask,&event);XFlush(display);pump();
    }
    void capture(IPlugView* view,const char* file){
        ViewRect rect;view->getSize(&rect);XSync(display,False);
        auto* pixels=XGetImage(display,window,0,0,rect.getWidth(),rect.getHeight(),AllPlanes,ZPixmap);require(pixels,"X11 capture failed");
        std::ofstream output(file,std::ios::binary);output<<"P6\n"<<rect.getWidth()<<' '<<rect.getHeight()<<"\n255\n";
        size_t lit=0;
        for(int y=0;y<rect.getHeight();++y)for(int x=0;x<rect.getWidth();++x){unsigned long p=XGetPixel(pixels,x,y);unsigned char rgb[]={static_cast<unsigned char>(p>>16),static_cast<unsigned char>(p>>8),static_cast<unsigned char>(p)};output.write(reinterpret_cast<char*>(rgb),3);if(rgb[0]>100||rgb[1]>100||rgb[2]>100)++lit;}
        XDestroyImage(pixels);require(output.good()&&lit>size_t(rect.getWidth()*rect.getHeight()/20),"GUI was blank");
    }
};
int main(int argc,char** argv){try{
    require(argc==2,"Pass VST3 bundle");std::string error;auto module=VST3::Hosting::Module::create(std::filesystem::canonical(argv[1]).string(),error);if(!module)throw std::runtime_error(error);
    HostApplication host;auto frame=owned(new Frame);frame->display=XOpenDisplay(nullptr);require(frame->display,"X11 unavailable");
    module->getFactory().setHostContext(&host);IPtr<IEditController> controller;
    for(auto& info:module->getFactory().classInfos())if(info.category()==kVstComponentControllerClass)controller=module->getFactory().createInstance<IEditController>(info.ID());
    require(controller&&controller->initialize(&host)==kResultOk,"Controller failed");controller->setComponentHandler(frame);
    require(controller->getParameterCount()==202,"Host parameter inventory changed");
    for(int lifecycle=0;lifecycle<2;++lifecycle){
        auto view=owned(controller->createView(ViewType::kEditor));require(view,"No editor");
        frame->window=XCreateSimpleWindow(frame->display,DefaultRootWindow(frame->display),0,0,1280,790,0,0,0);XMapWindow(frame->display,frame->window);XSync(frame->display,False);
        view->setFrame(frame);require(view->attached(reinterpret_cast<void*>(frame->window),kPlatformTypeX11EmbedWindowID)==kResultOk,"Attach failed");frame->pump();
        FUnknownPtr<IParameterFinder> finder(view);require(finder,"Parameter finder unavailable");
        auto has=[&](double x,double y,ParamID id){ViewRect r;view->getSize(&r);ParamID actual=0;return finder->findParameter(int32(std::lround(x*r.getWidth()/1280.)),int32(std::lround(y*r.getHeight()/790.)),actual)==kResultOk&&actual==id;};
        require(has(221,255,slotParam(0,kSlotTime)),"Initial A/B page missing");
        for(double zoom:{.75,1.,1.5}){
            ViewRect size(0,0,int(1280*zoom),int(790*zoom));require(frame->resizeView(view,&size)==kResultOk,"Resize failed");frame->pump();
            for(int tab=0;tab<4;++tab){
                const int before=frame->edits;frame->click(view,366+140*tab,40);require(frame->edits==before,"Tab changed audio");
                require(has(325,738,kMXMixId),"Persistent mix missing");
                if(tab==0)require(has(221,255,slotParam(0,kSlotTime)),"A/B binding missing");
                if(tab==1)require(has(155,247,wetFxParam(0,kWetFxType)),"FX binding missing");
                if(tab==2)require(!has(221,255,slotParam(0,kSlotTime)),"Hidden controls hittable");
                if(tab==3)require(has(580,281,kMXRoutingId),"Routing binding missing");
                if(lifecycle==0&&zoom==1.)frame->capture(view,("console-tab-"+std::to_string(tab)+".ppm").c_str());
            }
            controller->setParamNormalized(kMXMixId,.82);frame->pump();frame->click(view,325,738,3);
            require(std::abs(controller->getParamNormalized(kMXMixId)-.35)<.002,"Factory reset regression");
            require(frame->gestures==0,"Unbalanced gestures");
        }
        view->removed();view->setFrame(nullptr);view=nullptr;XDestroyWindow(frame->display,frame->window);
    }
    controller->setComponentHandler(nullptr);controller->terminate();controller=nullptr;module.reset();XCloseDisplay(frame->display);
    std::cout<<"Native GUI: 2 lifecycles x 3 scales x 4 tabs, parameter bindings, hidden hit areas, UI-only navigation, right-click passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
