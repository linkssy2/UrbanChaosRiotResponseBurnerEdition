#include "xbe_audio_loop.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {
using Clock=std::chrono::steady_clock;
struct Capture {
    std::mutex lock;
    std::atomic<bool> active{false};
    bool busy=false;
    uint32_t capacity=65536;
    std::atomic<uint64_t> dropped{0}, next_sample{0};
    uint64_t start=0;
    std::vector<uc_audio_trace::Record> records;
};
// Process lifetime, matching the existing detached XAudio2 output thread.
Capture* const g_capture=new Capture;
Capture& State(){return *g_capture;}
bool Start(const char* path,uint32_t duration,uint32_t capacity) {
    if(!path||!*path||!capacity)return false;
    auto& s=State();
    std::lock_guard<std::mutex> lock(s.lock);
    if(s.busy)return false;
    try {
        std::string filename(path);
        s.records.clear();s.records.reserve(capacity);s.capacity=capacity;
        s.dropped=0;s.next_sample=0;s.start=uc_audio_trace::ClockMicros();s.busy=true;s.active=true;
        std::thread([filename=std::move(filename),duration]{
            std::this_thread::sleep_for(std::chrono::milliseconds(duration));
            auto& state=State();
            std::vector<uc_audio_trace::Record> rows;
            uint64_t dropped;
            {
                std::lock_guard<std::mutex> guard(state.lock);
                state.active=false;rows.swap(state.records);dropped=state.dropped.load();
            }
            // No mixer lock is held during filesystem access.
            try {
                std::ofstream out(filename,std::ios::out|std::ios::trunc);
                out<<"# audio trace duration_ms="<<duration<<" records="<<rows.size()<<" dropped="<<dropped
                   <<" truncated="<<(dropped!=0)<<"\n";
                out<<"# kinds 1=snapshot 2=play 3=stop 4=stopex 5=release 6=flush 7=process 8=pause 9=output 10=route\n";
                out<<"# flags stream=1 playing=2 loop=4 pause=8 sync=16 active=32 ending=64 spatial=128 mixin=256\n";
                out<<"# Play/Stop/StopEx/Pause/Flush events precede the operation; Process follows enqueue; Release precedes final deletion.\n";
                out<<"# Snapshots describe post-render state; stream front data can differ from the packets just measured. Source/mixed RMS sample one block every 100ms and divide by produced frames, not the full output window. frames=0 means skipped/stopped/starved, not measured silence.\n";
                out<<"# Output packets=queued buffers sampled BEFORE render/submit (not an underrun callback), position=render_us including mixer-lock wait, argument=submit HRESULT.\n";
                out<<"# Route events precede the request; argument=requested target, output=previous target. MIXIN snapshots have gain but no source frames; their playing flag is not bus audibility. distance=-1 means no host 3D range measurement (software-authored distance may instead change mix-bin gains).\n";
                out<<"us,kind,object,flags,tag,channels,rate,data,bytes,play_start,play_len,loop_start,loop_len,packets,frames,position,source_rms,mixed_rms,gain_l,gain_r,volume,headroom,x,y,z,argument,output,mode,min_distance,max_distance,distance\n";
                out<<std::setprecision(9);
                for(const auto& r:rows)out<<r.us<<','<<r.kind<<','<<r.object<<','<<r.flags<<','<<r.tag<<','
                    <<r.channels<<','<<r.rate<<','<<r.data<<','<<r.bytes<<','<<r.play_start<<','<<r.play_len<<','
                    <<r.loop_start<<','<<r.loop_len<<','<<r.packets<<','<<r.frames<<','<<r.position<<','
                    <<r.source_rms<<','<<r.mixed_rms<<','<<r.gain_l<<','<<r.gain_r<<','<<r.volume<<','
                    <<r.headroom<<','<<r.x<<','<<r.y<<','<<r.z<<','<<r.argument<<','<<r.output<<','<<r.mode<<','
                    <<r.min_distance<<','<<r.max_distance<<','<<r.distance<<'\n';
                out.flush();
                if(!out)std::fprintf(stderr,"Audio trace could not write %s\n",filename.c_str());
            } catch(...) {std::fprintf(stderr,"Audio trace output failed\n");}
            std::lock_guard<std::mutex> guard(state.lock);state.busy=false;
        }).detach();
        return true;
    }catch(...){s.active=false;s.busy=false;return false;}
}
}
namespace uc_audio_trace {
uint64_t ClockMicros(){return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());}
bool Active(){return State().active.load(std::memory_order_relaxed);}
bool SnapshotDue(){
    if(!Active())return false;
    auto& s=State();uint64_t now=ClockMicros();
    if(now<s.next_sample.load(std::memory_order_relaxed))return false;
    s.next_sample.store(now+100000,std::memory_order_relaxed);return true;
}
void Append(Record record){
    auto& s=State();if(!s.active.load(std::memory_order_relaxed))return;
    std::unique_lock<std::mutex> lock(s.lock,std::try_to_lock);
    if(!lock.owns_lock()){++s.dropped;return;}
    if(!s.active.load(std::memory_order_relaxed))return;
    if(s.records.size()>=s.capacity){++s.dropped;return;}
    record.us=ClockMicros()-s.start;s.records.push_back(record);
}
void OutputState(uint32_t queued,uint64_t render_us,uint32_t result){
    Record r;r.kind=Output;r.packets=queued;r.position=double(render_us);r.argument=result;Append(r);
}
}
bool TrialAudioTraceStart(const char* path){return Start(path,20000,65536);}
#ifdef XBE_AUDIO_TRACE_TEST
bool TrialAudioTraceStartTest(const char* path,uint32_t duration,uint32_t capacity){return Start(path,duration,capacity);}
bool TrialAudioTraceBusyTest(){auto& s=State();std::lock_guard<std::mutex> lock(s.lock);return s.busy;}
#endif
