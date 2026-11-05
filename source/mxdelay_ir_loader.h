#pragma once
#include "mxdelay_ir.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
namespace JerzyAudio {
// SPSC ownership exchange. Audio may exchange/retire pointers but never delete them.
class MXIRLoader {
    static_assert(std::atomic<MXPartitionedConvolver*>::is_always_lock_free);
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    std::atomic<MXPartitionedConvolver*> pending{nullptr};
    std::array<MXPartitionedConvolver*,64> retired{};std::atomic<unsigned> write{0},read{0};
    std::atomic<uint64_t> options{0},pathRevision{0};std::atomic<bool> stopping{true};std::thread worker;
    mutable std::mutex stateMutex;std::string path,name="Factory Room",error;int state=0;double rate=48000;
    void drain(){auto r=read.load(std::memory_order_relaxed);const auto w=write.load(std::memory_order_acquire);while(r!=w){delete retired[r];r=(r+1)%retired.size();}read.store(r,std::memory_order_release);}
public:
    struct Status {std::string name,error;int state=0;};
    static uint64_t key(int profile,bool external,double length,bool normalize){const auto f=std::bit_cast<uint32_t>(float(std::clamp(std::isfinite(length)?length:1.,0.,1.)));return uint64_t(f)|(uint64_t(std::clamp(profile,0,2))<<32)|(uint64_t(external)<<34)|(uint64_t(normalize)<<35);}
    void stop(){stopping.store(true);if(worker.joinable())worker.join();}
    ~MXIRLoader(){stop();delete pending.exchange(nullptr);drain();}
    void start(double sampleRate,uint64_t initial=key(0,false,1.,true)){
        stop();delete pending.exchange(nullptr);drain();rate=sampleRate;stopping=false;options.store(initial);
        worker=std::thread([this,initial]{
            uint64_t seen=initial,seenPath=pathRevision.load();std::unique_ptr<MXIRAudio> external;uint64_t externalRevision=~uint64_t{};
            while(!stopping.load()){
                drain();const auto request=options.load(std::memory_order_acquire),revision=pathRevision.load(std::memory_order_acquire);
                if(request!=seen||revision!=seenPath){
                    seen=request;seenPath=revision;const int profile=int((request>>32)&3);const bool useExternal=(request>>34)&1,norm=(request>>35)&1;
                    const double length=std::bit_cast<float>(uint32_t(request));
                    {std::lock_guard<std::mutex> lock(stateMutex);state=1;error.clear();}
                    try {
                        MXIRAudio factory;const MXIRAudio* source=nullptr;
                        if(useExternal){if(!external||externalRevision!=revision){std::string file;{std::lock_guard<std::mutex> lock(stateMutex);file=path;}external=std::make_unique<MXIRAudio>(loadMXIRWav(file));externalRevision=revision;}source=external.get();}
                        else {factory=factoryMXIR(profile,rate);source=&factory;}
                        auto prepared=prepareMXIR(*source,rate,.02+7.98*length,norm);
                        auto bank=std::make_unique<MXPartitionedConvolver>(prepared);
                        if(stopping.load()||options.load()!=request||pathRevision.load()!=revision)continue;
                        delete pending.exchange(bank.release(),std::memory_order_acq_rel);
                        std::lock_guard<std::mutex> lock(stateMutex);name=source->name;state=2;error.clear();
                    }catch(const std::exception& e){std::lock_guard<std::mutex> lock(stateMutex);state=3;error=e.what();}
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            drain();
        });
    }
    void requestOptions(uint64_t value)noexcept{if(options.load(std::memory_order_relaxed)!=value)options.store(value,std::memory_order_release);}
    void requestPath(const std::string& value,bool selectExternal=true){std::lock_guard<std::mutex> lock(stateMutex);path=value;state=1;error.clear();if(selectExternal)options.fetch_or(uint64_t(1)<<34,std::memory_order_release);pathRevision.fetch_add(1,std::memory_order_release);}
    void initialStatus(const std::string& n,const std::string& e){std::lock_guard<std::mutex> lock(stateMutex);name=n;error=e;state=e.empty()?2:3;}
    Status status()const{std::lock_guard<std::mutex> lock(stateMutex);return {name,error,state};}
    MXPartitionedConvolver* take()noexcept{return pending.exchange(nullptr,std::memory_order_acq_rel);}
    bool retire(MXPartitionedConvolver* bank)noexcept{
        if(!bank)return true;const unsigned w=write.load(std::memory_order_relaxed),next=(w+1)%unsigned(retired.size());if(next==read.load(std::memory_order_acquire))return false;
        retired[w]=bank;write.store(unsigned(next),std::memory_order_release);return true;
    }
};
}
