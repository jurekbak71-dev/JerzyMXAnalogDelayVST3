#include "mxdelay_processor.h"
#include "mxdelay_ids.h"
#include "mxdelay_state.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cmath>
#include "pluginterfaces/vst/ivstmessage.h"
#include <cstring>
using namespace Steinberg;using namespace Steinberg::Vst;
namespace JerzyAudio {
MXDelayProcessor::MXDelayProcessor(){setControllerClass(kMXDelayControllerUID);processContextRequirements.needTempo().needTransportState();}
tresult PLUGIN_API MXDelayProcessor::initialize(FUnknown* c){auto r=AudioEffect::initialize(c);if(r!=kResultOk)return r;addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);addEventInput(STR16("MIDI In"),16);return kResultOk;}
tresult PLUGIN_API MXDelayProcessor::setBusArrangements(SpeakerArrangement* i,int32 ni,SpeakerArrangement* o,int32 no){if(ni!=1||no!=1)return kResultFalse;auto a=SpeakerArr::getChannelCount(i[0]),b=SpeakerArr::getChannelCount(o[0]);if(a!=b||(a!=1&&a!=2))return kResultFalse;return AudioEffect::setBusArrangements(i,ni,o,no);}
tresult PLUGIN_API MXDelayProcessor::setupProcessing(ProcessSetup& s){sampleRate=s.sampleRate>0?s.sampleRate:44100;processing64=s.symbolicSampleSize==kSample64;if(processing64)dsp64.prepare(sampleRate,2,&p.reverb);else dsp32.prepare(sampleRate,2,&p.reverb);return AudioEffect::setupProcessing(s);}
tresult PLUGIN_API MXDelayProcessor::setActive(TBool x){if(x){dsp32.reset();dsp64.reset();lastPeak=0;}return AudioEffect::setActive(x);}
tresult PLUGIN_API MXDelayProcessor::canProcessSampleSize(int32 s){return(s==kSample32||s==kSample64)?kResultTrue:kResultFalse;}
void MXDelayProcessor::setNormalized(ParamID id,double v){v=std::clamp(v,0.0,1.0);switch(id){case kMXMixId:p.mix=v;return;case kMXInputTrimId:p.inputTrim=v;return;case kMXOutputTrimId:p.outputTrim=v;return;case kMXRoutingId:p.routing=v;return;case kMXBypassId:p.bypass=v;return;case kMXSpillId:p.spill=v;return;case kMXMachineConditionId:p.machineCondition=v;return;case kMXReverbEnableId:p.reverb.enable=v;return;case kMXReverbWetId:p.reverb.wet=v;return;case kMXReverbDecayId:p.reverb.decay=v;return;case kMXReverbDampingId:p.reverb.damping=v;return;case kMXReverbWidthId:p.reverb.width=v;return;case kMXReverbPreDelayId:p.reverb.preDelay=v;return;case kMXReverbFreezeId:p.reverb.freeze=v;return;case kMXReverbModeId:p.reverb.mode=v;return;case kMXReverbIRId:p.reverb.ir=v;return;case kMXReverbExternalId:p.reverb.external=v;return;case kMXReverbIRLengthId:p.reverb.irLength=v;return;case kMXReverbNormalizeId:p.reverb.normalize=v;return;case kMXReverbLowCutId:p.reverb.lowCut=v;return;case kMXReverbDiffusionId:p.reverb.diffusion=v;return;case kMXReverbEarlyId:p.reverb.early=v;return;case kMXReverbHybridId:p.reverb.hybrid=v;return;case kMXReverbIRDampingId:p.reverb.irDamping=v;return;default:break;}if(id>=kMXWetFx1TypeId&&id<=kMXWetFx4RouteId){unsigned q=id-kMXWetFx1TypeId;int fx=int(q/6),field=int(q%6);if(fx>=0&&fx<4){auto& z=p.fx[fx];switch(field){case kWetFxType:z.type=v;break;case kWetFxAmount:z.amount=v;break;case kWetFxRate:z.rate=v;break;case kWetFxDepth:z.depth=v;break;case kWetFxDrive:z.drive=v;break;case kWetFxRoute:z.route=v;break;}return;}}for(int s=0;s<2;++s){unsigned base=slotBase(s);if(id<base||id>=base+kMXSlotStride)continue;unsigned off=id-base;auto& x=p.slot[s];if(off<=kSlotDuck){switch(off){case kSlotAlgorithm:x.algorithm=v;break;case kSlotEnable:x.enable=v;break;case kSlotSync:x.sync=v;break;case kSlotDivision:x.division=v;break;case kSlotTime:x.time=v;break;case kSlotFeedback:x.feedback=v;break;case kSlotLevel:x.level=v;break;case kSlotPan:x.pan=v;break;case kSlotDuck:x.duck=v;break;default:break;}return;}if(off>=kSlotHeadPan1&&off<=kSlotHeadPan4){x.headPan[off-kSlotHeadPan1]=v;return;}if(off>=kSlotControlBase){unsigned q=off-kSlotControlBase;int a=(int)(q/kAlgoControls),c=(int)(q%kAlgoControls);if(a<kAlgorithmCount){x.c[a][c]=v;return;}}}}
void MXDelayProcessor::readChanges(IParameterChanges* changes){if(!changes)return;for(int32 i=0;i<changes->getParameterCount();++i)if(auto*q=changes->getParameterData(i)){int32 n=q->getPointCount();if(n<=0)continue;int32 off=0;ParamValue v=0;if(q->getPoint(n-1,off,v)==kResultTrue&&std::isfinite(v))setNormalized(q->getParameterId(),v);}}
void MXDelayProcessor::sendTempo(ProcessData& d){if(!d.outputParameterChanges)return;int32 qi=0;if(auto*q=d.outputParameterChanges->addParameterData(kMXTempoMeterId,qi)){int32 pi=0;q->addPoint(std::max<int32>(0,d.numSamples-1),std::clamp(bpm/300.0,0.0,1.0),pi);}}
tresult PLUGIN_API MXDelayProcessor::process(ProcessData& d){
 if(d.processContext&&(d.processContext->state&ProcessContext::kTempoValid)&&std::isfinite(d.processContext->tempo)&&d.processContext->tempo>1.0)bpm=d.processContext->tempo;
 if(d.numInputs==0||d.numOutputs==0||d.numSamples<=0){readChanges(d.inputParameterChanges);sendTempo(d);return kResultOk;}
 int ch=std::min(d.inputs[0].numChannels,d.outputs[0].numChannels);if(ch<=0)return kResultOk;if(d.symbolicSampleSize!=kSample32&&d.symbolicSampleSize!=kSample64)return kResultFalse;lastPeak=0.0;MXDelayDSP<float>::MeterPeaks meter32{};MXDelayDSP<double>::MeterPeaks meter64{};
 const int32 queueCount=d.inputParameterChanges?d.inputParameterChanges->getParameterCount():0;
 if(queueCount<0||queueCount>int32(automation.size()))return kInvalidArgument;
 auto advance=[&](QueueCursor& cursor){
  cursor.valid=false;if(!cursor.queue)return;
  while(cursor.point<cursor.queue->getPointCount()){
   int32 offset=0;ParamValue value=0;
   if(cursor.queue->getPoint(cursor.point++,offset,value)==kResultTrue&&std::isfinite(value)){
    cursor.offset=std::clamp<int32>(offset,0,d.numSamples);cursor.value=std::clamp(value,0.0,1.0);cursor.valid=true;return;
   }
  }
 };
 for(int32 i=0;i<queueCount;++i){automation[i]={d.inputParameterChanges->getParameterData(i),0,0,0,false};advance(automation[i]);}
 auto run32=[&](int32 start,int32 count){
  if(count<=0)return;auto*baseIn=d.inputs[0].channelBuffers32;auto*baseOut=d.outputs[0].channelBuffers32;float*in[2]={baseIn&&baseIn[0]?baseIn[0]+start:nullptr,ch>1&&baseIn&&baseIn[1]?baseIn[1]+start:nullptr};float*out[2]={baseOut&&baseOut[0]?baseOut[0]+start:nullptr,ch>1&&baseOut&&baseOut[1]?baseOut[1]+start:nullptr};double peak=0;dsp32.process(in,out,ch,count,p,bpm,peak,&meter32);lastPeak=std::max(lastPeak,peak);
 };
 auto run64=[&](int32 start,int32 count){
  if(count<=0)return;auto*baseIn=d.inputs[0].channelBuffers64;auto*baseOut=d.outputs[0].channelBuffers64;double*in[2]={baseIn&&baseIn[0]?baseIn[0]+start:nullptr,ch>1&&baseIn&&baseIn[1]?baseIn[1]+start:nullptr};double*out[2]={baseOut&&baseOut[0]?baseOut[0]+start:nullptr,ch>1&&baseOut&&baseOut[1]?baseOut[1]+start:nullptr};double peak=0;dsp64.process(in,out,ch,count,p,bpm,peak,&meter64);lastPeak=std::max(lastPeak,peak);
 };
 int32 pos=0;
 while(true){
  int32 next=d.numSamples;bool found=false;
  for(int32 i=0;i<queueCount;++i)if(automation[i].valid){next=std::min(next,std::max(pos,automation[i].offset));found=true;}
  if(!found)break;
  if(next>pos){if(d.symbolicSampleSize==kSample32)run32(pos,next-pos);else run64(pos,next-pos);pos=next;}
  for(int32 i=0;i<queueCount;++i)while(automation[i].valid&&automation[i].offset<=pos){setNormalized(automation[i].queue->getParameterId(),automation[i].value);advance(automation[i]);}
 }

 if(pos<d.numSamples){if(d.symbolicSampleSize==kSample32)run32(pos,d.numSamples-pos);else run64(pos,d.numSamples-pos);}
 d.outputs[0].silenceFlags=0;sendTempo(d);
 if(d.outputParameterChanges){
  const bool activeA=p.slot[0].enable>=.5&&! (p.bypass>=.5&&p.spill<.5);
  const bool activeB=p.slot[1].enable>=.5&&! (p.bypass>=.5&&p.spill<.5);
  const double inPeak=d.symbolicSampleSize==kSample32?meter32.input:meter64.input;
  const double wetPeak=d.symbolicSampleSize==kSample32?meter32.wet:meter64.wet;
  const std::pair<ParamID,double> telemetry[]={
   {kMXInputPeakMeterId,inPeak},{kMXWetPeakMeterId,wetPeak},
   {kMXPeakMeterId,lastPeak},{kMXEngineAActiveId,activeA?1.0:0.0},
   {kMXEngineBActiveId,activeB?1.0:0.0},{kMXEngineBothActiveId,activeA&&activeB?1.0:0.0}
  };
  for(const auto& m:telemetry){int32 qi=0;if(auto*q=d.outputParameterChanges->addParameterData(m.first,qi)){int32 pi=0;q->addPoint(std::max<int32>(0,d.numSamples-1),std::clamp(m.second,0.0,1.0),pi);}}
 }
 return kResultOk;
}
tresult PLUGIN_API MXDelayProcessor::setState(IBStream*s){if(!s)return kResultFalse;IBStreamer b(s,kLittleEndian);if(!readMXState(b,p))return kResultFalse;dsp32.requestIRPath(p.reverb.irPath,false);dsp64.requestIRPath(p.reverb.irPath,false);return kResultOk;}
tresult PLUGIN_API MXDelayProcessor::notify(IMessage* message){
 if(!message)return kInvalidArgument;
 if(std::strcmp(message->getMessageID(),"MXIRLoad")==0){
  const void* data=nullptr;uint32 size=0;
  if(message->getAttributes()->getBinary("path",data,size)!=kResultOk||!data||size==0||size>32768)return kInvalidArgument;
  std::string path(static_cast<const char*>(data),size);if(path.find('\0')!=std::string::npos)return kInvalidArgument;
  p.reverb.irPath=path;dsp32.requestIRPath(path);dsp64.requestIRPath(path);return kResultOk;
 }
 if(std::strcmp(message->getMessageID(),"MXIRStatusRequest")==0){
  const auto status=processing64?dsp64.irStatus():dsp32.irStatus();
  if(auto reply=owned(allocateMessage())){reply->setMessageID("MXIRStatus");reply->getAttributes()->setBinary("name",status.name.data(),uint32(status.name.size()));reply->getAttributes()->setBinary("error",status.error.data(),uint32(status.error.size()));reply->getAttributes()->setInt("state",status.state);sendMessage(reply);}
  return kResultOk;
 }
 return AudioEffect::notify(message);
}
tresult PLUGIN_API MXDelayProcessor::getState(IBStream*s){if(!s)return kResultFalse;IBStreamer b(s,kLittleEndian);return writeMXState(b,p)?kResultOk:kResultFalse;}
}
