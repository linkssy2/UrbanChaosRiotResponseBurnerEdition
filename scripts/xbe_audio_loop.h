#pragma once
#include <cstdint>
#include <cmath>

// Diagnostic metadata only. No asset bytes or recorded audio are written.
namespace uc_audio_trace {
enum Kind : uint32_t { Snapshot=1, Play, Stop, StopEx, Release, Flush, Process, Pause, Output, Route };
struct Record {
    uint64_t us=0;
    uint32_t kind=0, object=0, flags=0, tag=0, channels=0, rate=0, data=0, bytes=0;
    uint32_t play_start=0, play_len=0, loop_start=0, loop_len=0, packets=0, frames=0;
    double position=0, source_rms=0, mixed_rms=0;
    float gain_l=0, gain_r=0, volume=0, headroom=0, x=0, y=0, z=0;
    uint32_t argument=0;
    uint32_t output=0, mode=0;
    float min_distance=0, max_distance=0, distance=-1;
};
bool Active();
bool SnapshotDue(); // Called only by the single output/mixer thread.
uint64_t ClockMicros();
void Append(Record record);
void OutputState(uint32_t queued, uint64_t render_us, uint32_t result);

template<class V>
Record VoiceRecord(Kind kind, const V& v, uint32_t argument=0, const float* listener=nullptr) {
    Record r;
    r.kind=kind; r.object=v.obj;
    r.flags=(v.stream?1u:0u)|(v.playing?2u:0u)|(v.looping?4u:0u)|
        (v.paused?8u:0u)|(v.sync?16u:0u)|(v.active?32u:0u)|(v.ending?64u:0u)|(v.is3d?128u:0u)|(v.mixin?256u:0u);
    r.tag=v.tag; r.channels=v.channels; r.rate=v.freq?v.freq:v.rate;
    r.data=v.stream&&!v.packets.empty()?v.packets.front().data:v.data;
    r.bytes=v.stream&&!v.packets.empty()?v.packets.front().size:v.bytes;
    r.play_start=v.play_start; r.play_len=v.play_len;
    r.loop_start=v.loop_start; r.loop_len=v.loop_len; r.packets=uint32_t(v.packets.size());
    r.position=v.pos; r.volume=v.volume; r.headroom=v.headroom;
    r.x=v.p[0]; r.y=v.p[1]; r.z=v.p[2]; r.argument=argument;
    r.output=v.output;r.mode=v.mode;r.min_distance=v.min_dist;r.max_distance=v.max_dist;
    if(listener&&v.is3d&&v.mode!=2){
        double length2=0;
        for(unsigned i=0;i<3;++i){double d=double(v.p[i])-(v.mode==1?0:double(listener[i]));length2+=d*d;}
        r.distance=float(std::sqrt(length2));
    }
    return r;
}
template<class V>
void Event(Kind kind, const V& v, uint32_t argument=0) {
    if(Active()) Append(VoiceRecord(kind,v,argument));
}
template<class V>
void VoiceSample(const V& v, const float gains[2][2], double source, double mixed, uint32_t frames, const float* listener=nullptr) {
    Record r=VoiceRecord(Snapshot,v,0,listener);
    r.gain_l=gains[0][0]+gains[1][0]; r.gain_r=gains[0][1]+gains[1][1]; r.frames=frames;
    r.source_rms=frames?std::sqrt(source/(2.0*frames)):0;
    r.mixed_rms=frames?std::sqrt(mixed/(2.0*frames)):0;
    Append(r);
}
}

// Copies the filename, rejects overlapping captures, records for twenty seconds.
// Allocation and final CSV writing never run on the audio thread.
bool TrialAudioTraceStart(const char* output_csv);
inline bool TrialAudioTraceHotkey(uint32_t message,uint32_t key,uint64_t lparam,
                                 bool focused,bool gameplay) {
    return message==0x100 && key==0x73 && focused && gameplay &&
        !(lparam&((uint64_t(1)<<29)|(uint64_t(1)<<30)));
}

#ifdef XBE_AUDIO_TRACE_TEST
bool TrialAudioTraceStartTest(const char* path,uint32_t duration_ms,uint32_t capacity);
bool TrialAudioTraceBusyTest();
#endif
