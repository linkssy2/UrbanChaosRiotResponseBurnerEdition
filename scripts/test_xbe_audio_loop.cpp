// Executes copied production mixer bodies before/after instrumentation, plus the
// actual recorder. The tiny deterministic PCM provider is not a codec/audio test.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "xbe_audio_loop.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cstdio>
struct Packet {uint32_t data,size,used=0;};
struct Voice {
    uint32_t obj=0; bool stream=false,playing=true,looping=true,paused=false,sync=false,active=false,ending=false,is3d=false;
    uint16_t tag=1,channels=2,align=4; uint32_t rate=48000,freq=0;
    uint32_t data=4,bytes=16,play_start=0,play_len=16,loop_start=0,loop_len=16;
    double pos=0; float volume=.5f,headroom=1,p[3]{};std::deque<Packet> packets;
    bool mixin=false;uint32_t output=0,mode=0;float min_dist=1,max_dist=100;
};
std::recursive_mutex g_lock;
std::map<uint32_t,Voice> g_voices;
float g_listener[3]{};
uint32_t completed=0;
uint32_t FramesOf(const Voice& v,uint32_t bytes){return bytes/v.align;}
double RoutedPlaybackStep(const Voice&){return 1;}
void Levels(Voice& v,float g[2][2]){g[0][0]=g[1][1]=v.volume;g[0][1]=g[1][0]=0;}
float Sample(Voice&,uint32_t,uint32_t f,uint32_t c){return (f&1)?.25f*(c?1:-1):.5f;}
void Complete(Voice&,Packet&,uint32_t){++completed;}
void Deactivate(Voice& v){v.sync=false;if(v.active)v.active=v.paused=v.ending=false;}
// GENERATED_ORIGINAL_RENDER
// GENERATED_TRACED_RENDER

static void Wait(){
    auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(TrialAudioTraceBusyTest()&&std::chrono::steady_clock::now()<deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    assert(!TrialAudioTraceBusyTest());
}
static std::string Read(const std::filesystem::path& p){std::ifstream in(p);assert(in);return {std::istreambuf_iterator<char>(in),{}};}
static std::vector<std::vector<double>> Rows(const std::string& csv){
    std::istringstream lines(csv);std::string line;std::vector<std::vector<double>> result;
    while(std::getline(lines,line)){
        if(line.empty()||line[0]=='#'||line.starts_with("us,"))continue;
        std::istringstream row(line);std::string field;std::vector<double> r;
        while(std::getline(row,field,','))r.push_back(std::stod(field));
        assert(r.size()==31);result.push_back(r);
    }return result;
}
int main(){
    assert(!uc_audio_trace::Active()&&!uc_audio_trace::SnapshotDue());
    assert(!TrialAudioTraceStart(nullptr)&&!TrialAudioTraceStart(""));
    assert(TrialAudioTraceHotkey(0x100,0x73,0,true,true));
    for(uint64_t bit:{uint64_t(1)<<29,uint64_t(1)<<30})assert(!TrialAudioTraceHotkey(0x100,0x73,bit,true,true));
    assert(!TrialAudioTraceHotkey(0x101,0x73,0,true,true));
    assert(!TrialAudioTraceHotkey(0x104,0x73,0,true,true));
    assert(!TrialAudioTraceHotkey(0x100,0x72,0,true,true));
    assert(!TrialAudioTraceHotkey(0x100,0x73,0,false,true));
    assert(!TrialAudioTraceHotkey(0x100,0x73,0,true,false));

    auto dir=std::filesystem::temp_directory_path()/std::to_string(uc_audio_trace::ClockMicros());
    std::filesystem::create_directories(dir);auto file=dir/"trace.csv";
    std::string mutable_path=file.string();
    assert(TrialAudioTraceStartTest(mutable_path.c_str(),120,128));
    mutable_path.assign("not-the-output-path");
    assert(!TrialAudioTraceStartTest("overlap.csv",1,1));
    assert(uc_audio_trace::SnapshotDue()&&!uc_audio_trace::SnapshotDue());
    std::map<uint32_t,Voice> initial;
    Voice loop;loop.obj=1;initial[1]=loop;
    Voice finite=loop;finite.obj=2;finite.looping=false;finite.pos=3;initial[2]=finite;
    Voice silent=loop;silent.obj=3;silent.volume=0;initial[3]=silent;
    Voice held=loop;held.obj=4;held.paused=true;initial[4]=held;
    Voice bus=loop;bus.obj=6;bus.mixin=true;bus.is3d=true;bus.p[2]=5;bus.output=7;initial[6]=bus;
    Voice stream=loop;stream.obj=5;stream.stream=true;stream.active=true;stream.ending=true;
    stream.packets.push_back({100,8});stream.packets.push_back({200,8});initial[5]=stream;
    float before[20]{},after[20]{};
    g_voices=initial;completed=0;RenderOriginal(before,10);auto expected=g_voices;auto expected_done=completed;
    g_voices=initial;completed=0;RenderTracked<true>(after,10);
    assert(std::memcmp(before,after,sizeof(before))==0&&completed==expected_done);
    for(auto& [id,v]:g_voices){auto& e=expected.at(id);assert(v.pos==e.pos&&v.playing==e.playing&&v.active==e.active&&v.packets.size()==e.packets.size());}
    g_voices=initial;completed=0;RenderTracked<false>(after,10);
    assert(std::memcmp(before,after,sizeof(before))==0&&completed==expected_done);
    uc_audio_trace::Event(uc_audio_trace::Stop,g_voices.at(1));
    uc_audio_trace::OutputState(0,35000,0x80004005);
    Wait();auto csv=Read(file);assert(csv.find("dropped=0 truncated=0")!=std::string::npos);
    auto rows=Rows(csv);assert(rows.size()==8);
    bool loop_seen=false,silent_seen=false,held_seen=false,output_seen=false,bus_seen=false;
    for(auto& r:rows){
        if(r[1]==uc_audio_trace::Snapshot&&r[2]==1){loop_seen=true;assert(r[14]==10&&r[16]>0&&std::abs(r[17]-r[16]*.5)<1e-7);}
        if(r[1]==uc_audio_trace::Snapshot&&r[2]==3){silent_seen=true;assert(r[16]>0&&r[17]==0&&r[18]==0&&r[19]==0);}
        if(r[1]==uc_audio_trace::Snapshot&&r[2]==4){held_seen=true;assert(r[14]==0&&(uint32_t(r[3])&8));}
        if(r[1]==uc_audio_trace::Output){output_seen=true;assert(r[13]==0&&r[15]==35000&&uint32_t(r[25])==0x80004005);}
        if(r[1]==uc_audio_trace::Snapshot&&r[2]==6){bus_seen=true;assert((uint32_t(r[3])&256)&&r[14]==0&&r[18]==.5&&r[26]==7&&r[28]==1&&r[29]==100&&r[30]==5);}
    }assert(loop_seen&&silent_seen&&held_seen&&output_seen&&bus_seen);
    auto capped=dir/"capped.csv";assert(TrialAudioTraceStartTest(capped.string().c_str(),40,2));
    for(int i=0;i<20;++i)uc_audio_trace::OutputState(2,1,0);
    Wait();auto cap=Read(capped);assert(cap.find("records=2 dropped=18 truncated=1")!=std::string::npos);
    assert(!uc_audio_trace::Active()&&!uc_audio_trace::SnapshotDue());
    std::filesystem::remove(file);std::filesystem::remove(capped);std::filesystem::remove(dir);
    std::puts("audio trace: original/traced mixer parity, voice/source/mixed evidence, inactive path, hotkey, overlap, copied filename and truncation passed");
}
