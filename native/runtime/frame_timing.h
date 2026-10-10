#pragma once
#include "renderer.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

// Explicit bounded benchmark only. No queries, polling or files in normal play.
// GPU timestamps measure elapsed command-stream time, including starvation;
// CPU frame time includes the preview's existing exposure readback waits.
class NativeFrameTiming {
    using Clock=std::chrono::steady_clock;
    struct Frame {
        ComPtr<ID3D11Query> clock,first,last;
        Clock::time_point start;
        double cpu_ms=0;
    };
    std::vector<Frame> frames_;
    unsigned completed_=0;
    static void check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Frame timing query failed");}
public:
    void open(NativeRenderer &r,unsigned count){
        if(!count||count>1024||!frames_.empty())throw std::invalid_argument("Invalid timing frame count");
        frames_.resize(count);
        for(auto &f:frames_){
            D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};check(r.device->CreateQuery(&q,&f.clock));
            q.Query=D3D11_QUERY_TIMESTAMP;check(r.device->CreateQuery(&q,&f.first));check(r.device->CreateQuery(&q,&f.last));
        }
    }
    void begin(NativeRenderer &r,unsigned i){
        if(i!=completed_||i>=frames_.size())throw std::logic_error("Timing frame sequence");
        auto &f=frames_[i];f.start=Clock::now();r.context->Begin(f.clock.Get());r.context->End(f.first.Get());
    }
    void end(NativeRenderer &r,unsigned i){
        if(i!=completed_||i>=frames_.size())throw std::logic_error("Timing frame completion");
        auto &f=frames_[i];r.context->End(f.last.Get());r.context->End(f.clock.Get());
        f.cpu_ms=std::chrono::duration<double,std::milli>(Clock::now()-f.start).count();++completed_;
    }
    void write(NativeRenderer &r,const std::filesystem::path &path,unsigned warmup){
        if(completed_!=frames_.size()||warmup>=completed_)throw std::logic_error("Incomplete frame timing");
        r.context->Flush();auto deadline=Clock::now()+std::chrono::seconds(10);
        auto read=[&](ID3D11Query *q,void *data,UINT size){
            for(;;){auto hr=r.context->GetData(q,data,size,D3D11_ASYNC_GETDATA_DONOTFLUSH);
                check(hr);if(hr==S_OK)return;
                if(Clock::now()>deadline)throw std::runtime_error("GPU timing completion timeout");
                SwitchToThread();
            }
        };
        std::ofstream out(path);out.exceptions(std::ios::badbit|std::ios::failbit);
        out<<"frame,cpu_frame_ms,gpu_stream_ms,warmup\n";
        for(unsigned i=0;i<completed_;++i){
            auto &f=frames_[i];D3D11_QUERY_DATA_TIMESTAMP_DISJOINT clock{};UINT64 first=0,last=0;
            read(f.clock.Get(),&clock,sizeof(clock));read(f.first.Get(),&first,sizeof(first));read(f.last.Get(),&last,sizeof(last));
            if(clock.Disjoint||!clock.Frequency||last<first)throw std::runtime_error("Invalid/disjoint GPU timing sample");
            out<<i<<','<<f.cpu_ms<<','<<double(last-first)*1000.0/double(clock.Frequency)<<','<<(i<warmup?1:0)<<'\n';
        }
    }
};
