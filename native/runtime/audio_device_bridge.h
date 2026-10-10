#pragma once
#include "audio_resident.h"
/* Actual244260/244120 quantized conversions, then explicit PC sample-rate
 * normalization. Only source receiver ranges gain[-10000,0], pitch[-32767,8191]
 * and backend ratio[1/1024,1024] are admitted; unsupported input throws
 * before device changes. Default FP mode, host pow numerical boundary. */
float native_audio_device_gain(int32_t);
float native_audio_device_ratio(int32_t,uint32_t sample_rate);
/* Source mode1, stage0, equal coefficient pairs, Q=0x8000 only. Signed
 * frequency[-32768,0] maps to2^(value/4096), without clamping. Positive codes,
 * upper word bits, asymmetric pairs and other modes reject before effects.
 * The six original words remain caller-owned; default FP/host exp2 boundary. */
NativeAudioDevice::VoiceFilter native_audio_device_filter(const uint32_t coefficients[6]);
/* One ordinary resident backend attached to an existing source device.buffer.
 * Uses the SAME NativeAudioDevice engine/voice pool. It owns only the PC token
 * and retained immutable PCM; instance/track/voice/device rings stay external.
 * Format/data setters stage validated source metadata; region commits the PC
 * format and decoded buffer together. This is a PC adaptation, not SDK call
 * timing equivalence. Initial gain/ratio are explicit caller mix. Live setters
 * consume actual quantized receiver output; they do not recompute group mix.
 * Each PC voice reserves max(4, source maximum pitch for this clip rate), so
 * all admitted source pitches fit without forcing every voice to reserve1024.
 * PC resident clip rate>=1000; pure source conversion also covers188..999.
 * One-shot nonlooped complete ADPCM blocks only.
 * Status exposes source PLAYING bit1 from actual started+queued PC state.
 * Stop is terminal Stop+Flush (no pause/resume/cursor-preservation claim).
 * Explicit headroom/output forward to the same-device nonspatial bus owner;
 * filter forwards the admitted real PC filter; spatial/reverb mix and source
 * factory completion remain outside this adapter.
 * The device must outlive this address-stable backend; all calls same thread. */
class NativeAudioDeviceBridge {
    NativeAudioDevice &device_;
    NativeAudioResidentView resident_;
    NativeAudioDevice::VoiceHandle handle_;
    float gain_,ratio_;
    bool format_=false,data_=false;
public:
    NativeAudioDeviceBridge(NativeAudioDevice &,std::shared_ptr<const NativeAudioClip>,float gain,float ratio);
    ~NativeAudioDeviceBridge();
    NativeAudioDeviceBridge(const NativeAudioDeviceBridge&)=delete;
    NativeAudioDeviceBridge&operator=(const NativeAudioDeviceBridge&)=delete;
    NativeAudioDeviceBridge(NativeAudioDeviceBridge&&)=delete;
    NativeAudioDeviceBridge&operator=(NativeAudioDeviceBridge&&)=delete;
    const UCAudioResidentWave &wave()const{return resident_.wave();}
    void format(const UCAudioDeviceFormat *);
    void data(const void *,uint32_t bytes);
    void region(uint32_t start,uint32_t bytes);
    void gain(int32_t quantized_gain);
    void pitch(int32_t quantized_pitch);
    void filter(const uint32_t coefficients[6]){device_.set_owned_filter(handle_,native_audio_device_filter(coefficients));}
    NativeAudioDevice::VoiceFilter filter()const{return device_.owned_voice_filter(handle_);}
    void headroom(uint32_t millibels){device_.set_owned_headroom(handle_,millibels);}
    void output(NativeAudioDevice::BusHandle destination={}){device_.route_owned_voice(handle_,destination);}
    NativeAudioDevice::VoiceRouting routing()const{return device_.owned_voice_routing(handle_);}
    NativeAudioDevice::VoiceParameters parameters()const{return device_.owned_voice_parameters(handle_);}
    void start(uint32_t repeat);
    void stop();
    uint32_t status()const;
};
