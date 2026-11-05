// Load the compiled module, exercise VST3 automation and serialized state.
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "mxdelay_params.h"
#include <array>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace Steinberg;using namespace Steinberg::Vst;using namespace JerzyAudio;
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char**argv){try{
 check(argc==2,"Pass compiled module path");std::string error;auto module=VST3::Hosting::Module::create(std::filesystem::canonical(argv[1]).string(),error);check(bool(module),error.c_str());HostApplication host;module->getFactory().setHostContext(&host);
 IPtr<IComponent> component;IPtr<IEditController> controller;for(const auto& c:module->getFactory().classInfos()){if(c.category()==kVstAudioEffectClass)component=module->getFactory().createInstance<IComponent>(c.ID());if(c.category()==kVstComponentControllerClass)controller=module->getFactory().createInstance<IEditController>(c.ID());}
 check(component&&controller,"component/controller absent");check(component->initialize(&host)==kResultOk&&controller->initialize(&host)==kResultOk,"initialize failed");FUnknownPtr<IAudioProcessor> processor(component);check(processor!=nullptr,"processor absent");
 for(int precision:{kSample32,kSample64}){
  ProcessSetup setup{};setup.processMode=kOffline;setup.symbolicSampleSize=precision;setup.maxSamplesPerBlock=257;setup.sampleRate=48000;check(processor->setupProcessing(setup)==kResultOk,"setup failed");component->activateBus(kAudio,kInput,0,true);component->activateBus(kAudio,kOutput,0,true);component->setActive(true);processor->setProcessing(true);
  for(int mode=0;mode<3;++mode){
   ParameterChanges changes(16);auto add=[&](ParamID id,double v,int off=0){int32 q=0,point=0;changes.addParameterData(id,q)->addPoint(off,v,point);};
   add(slotParam(0,kSlotEnable),0);add(slotParam(1,kSlotEnable),0);add(kMXMixId,1);add(kMXReverbEnableId,1);add(kMXReverbWetId,1);add(kMXReverbPreDelayId,0);add(kMXReverbModeId,mode*.5);add(kMXReverbHybridId,.37);add(kMXReverbDecayId,.7);add(kMXReverbDiffusionId,.8);add(kMXReverbEarlyId,.2);
   std::array<float,257> fl{},fr{},ol{},orr{};std::array<double,257> dl{},dr{},dol{},dor{};float* fi[]={fl.data(),fr.data()};float* fo[]={ol.data(),orr.data()};double* di[]={dl.data(),dr.data()};double* dout[]={dol.data(),dor.data()};
   AudioBusBuffers in{},out{};in.numChannels=out.numChannels=2;if(precision==kSample32){in.channelBuffers32=fi;out.channelBuffers32=fo;}else{in.channelBuffers64=di;out.channelBuffers64=dout;}
   ProcessData data{};data.processMode=kOffline;data.symbolicSampleSize=precision;data.numSamples=257;data.numInputs=data.numOutputs=1;data.inputs=&in;data.outputs=&out;data.inputParameterChanges=&changes;
   double energy=0;for(int b=0;b<220;++b){fl.fill(0);fr.fill(0);dl.fill(0);dr.fill(0);if(b==20)fl[0]=fr[0]=float(dl[0]=dr[0]=1);check(processor->process(data)==kResultOk,"compiled processor failed");data.inputParameterChanges=nullptr;for(int i=0;i<257;++i){double l=precision==kSample32?ol[i]:dol[i],r=precision==kSample32?orr[i]:dor[i];check(std::isfinite(l)&&std::isfinite(r),"nonfinite VST output");energy+=l*l+r*r;}}
   check(energy>1e-7,"compiled reverb silent with delays off");std::cout<<"VST3 precision "<<precision<<" mode "<<mode<<" energy "<<energy<<'\n';
   auto state=owned(new MemoryStream);check(component->getState(state)==kResultOk,"getState failed");state->seek(0,IBStream::kIBSeekSet,nullptr);check(controller->setComponentState(state)==kResultOk,"controller state failed");check(std::abs(controller->getParamNormalized(kMXReverbModeId)-mode*.5)<1e-6&&std::abs(controller->getParamNormalized(kMXReverbHybridId)-.37)<1e-6,"new parameters lost in host state");state->seek(0,IBStream::kIBSeekSet,nullptr);check(component->setState(state)==kResultOk,"setState failed");
  }
  processor->setProcessing(false);component->setActive(false);
 }
 controller->terminate();component->terminate();processor=nullptr;component=nullptr;controller=nullptr;module.reset();std::cout<<"Compiled VST3 reverb automation, standalone audio and state passed\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
