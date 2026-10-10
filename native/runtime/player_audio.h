#pragma once
#include "audio_events.h"
#include <filesystem>
struct UCAudioSourceEngine;
extern "C" {
#include "../animation_sound.h"
#include "../animation_events.h"
#include "../audio_group.h"
}

/* Fresh resident pistol/shotgun, Player.amp SwitchWeapon, PlayerFootsteps and
 * LevelGlobals HeartBeep audio. One session owns the banks, shared Game3D and distinct Panel
 * groups, the separate audio RNG and the device. profile_volume is an
 * explicit PC setting0..16 (263230 scales by1/16), never a loaded save default.
 * Group/instance values stay fixed for these one-shots; fades, arbitration,
 * spatial voices, other Player.amp events and other weapons remain unsupported.
 * All calls and destruction occur on the constructing thread. */
class NativePlayerAudio {
    friend class NativeCharacterDialogueAudio;
    NativeAudioBankAssets banks_[5];
    std::unique_ptr<NativeAudioBankAssets> character_steps_;
    std::unique_ptr<NativeAudioBankAssets> character_foley_;
    std::string material_footstep(NativeAudioBankAssets &,uint32_t,bool character);
    UCAudioGroup group_{},panel_{};
    UCAudioRandom random_{1};
    bool mute_;
    uint64_t requests_=0,started_=0;
    // Destroy voices before releasing their bank/PCM owners.
    std::unique_ptr<NativeAudioDevice> device_;
    struct MissionState;
    std::shared_ptr<MissionState> mission_state_;
    struct EmptyState;
    std::shared_ptr<EmptyState> empty_state_; // Destroy helpers before this device.
    struct LiveVoices;std::unique_ptr<LiveVoices> live_;
    NativeAudioInstanceMix live_mix(const UCAudioGroup &) const;
    void update_live();
    void filter_voice(NativeAudioDevice::VoiceHandle,const UCAudioGroup *,uint16_t cutoff);
public:
    struct VoiceSnapshot {std::string group;NativeAudioDevice::VoiceParameters mix;NativeAudioDevice::VoiceFilter filter;};
    struct EmptySource;
    // PC scheduled source on this same device/group/RNG. Authored commands and
    // loops and the qualified named policies are retained; original spatial
    // attenuation and the engine-wide scheduler remain separate.
    class MissionSource {
        struct Impl;std::unique_ptr<Impl> impl_;
    public:
        MissionSource(NativePlayerAudio &,const UCArchive *,size_t,const std::filesystem::path &,bool panel=false,uint32_t panel_groups=1);
        ~MissionSource();
        bool play(const char *);
        void stop(const char *);
        void volume(const char *,float);
        void parameter(const char *event,const char *name,float value);
        void step(float);
        size_t voices()const;
        bool busy()const; //2160A0/2167C0 pending or live nonpaused event instance
        std::vector<VoiceSnapshot> inspect()const; //read-only bounded verification
    };
    NativePlayerAudio(const std::filesystem::path &game,uint8_t profile_volume,bool mute);
    ~NativePlayerAudio();
    NativePlayerAudio(const NativePlayerAudio&)=delete;
    NativePlayerAudio &operator=(const NativePlayerAudio&)=delete;
    const NativeAudioBankAssets &bank(uint32_t item) const;
    NativeAudioInstanceMix mix() const;
    NativeAudioPlayResult play(uint32_t item,const char *name);
    /* BC8D0 sound request only; no ammo, animation, projectile or effect claim.
     * These real banks have no SecondaryFire, so nonzero alternate falls back
     * to Fire as the original does after a missing event. */
    NativeAudioPlayResult fire(uint32_t item,uint8_t alternate);
    /* CF8A0 requests ShieldEquip for item0. The actual Player bank has no such
     * event: preserve MissingEvent, with no substituted Activate/Deactivate. */
    NativeAudioPlayResult switch_weapon(bool shield=false);
    // D0C30's real Player-bank requests, separate from the absent ShieldEquip.
    NativeAudioPlayResult shield(bool active);
    /* Actual empty-AMPC helper domain used by the opening's Default.amp.
     * Same session/device, one retained immutable empty bank and real source
     * registry. The PC nonspatial engine has no listener interface; no spatial
     * scale/playback claim. Populated banks reject before helper allocation.
     * Source pointers are borrowed, session-local, and expire on close/session
     * destruction. No caller may mutate their engine/rings. */
    EmptySource *create_empty_source(const void *bank,size_t bytes,const float point[3],bool attached=false);
    bool play_empty_source(EmptySource *,const char *event);
    bool empty_source_busy(const EmptySource *) const;
    /*216040/216850 with empty notifier/buffer rings. Attached point storage
     * must outlive the helper. Same engine receives current position;245CD0
     * has no first buffer. Finite point/positive finite delta, no spatial voice. */
    void tick_empty_source(EmptySource *,float delta);
    void close_empty_source(EmptySource *);
    size_t empty_source_count() const;
    const UCAudioSourceEngine *empty_source_engine(const EmptySource *) const;
    /* HUD HeartBeep request through the distinct shared Panel group. */
    NativeAudioPlayResult heartbeat();
    /* Completed ground-query material only; audio216C60 routing, no ground or
     * footprint claim. Returns the actual requested/default name. Material30's
     * named arbitration and authored two-track program remain unsupported
     * and reject before effects. */
    std::string footstep(uint32_t material);
    // Separate original NPC bank on this same session/RNG/device. Explicit PC
    // nonspatial one-shot output, matching MissionSource's current output scope.
    void prepare_character_footsteps(const std::filesystem::path &game);
    std::string character_footstep(uint32_t material);
    NativeAudioPlayResult character_foley(const NativeAudioBank &);
    /* Apply global20BF50 to the remaining queue AFTER local player handlers.
     * The weapon state has actor=Mainplayer; the separately constructed arms
     * state has actor=null. Unknown marker types remain in authored order. */
    void dispatch(UCAnimationEventQueue &,const void *actor,uint32_t state,
                  int32_t active,int32_t previous);
    size_t collect();
    void stop_all();
    void pause(bool paused){if(device_)device_->pause(paused);}
    //20D840 and20F750/20E320, using the real shared case-insensitive group
    //registry and absolute published world clock. Missing groups are quiet.
    void group_pitch(const char *name,float ratio);
    void group_filter(const char *name,float cutoff,float attack,float hold,float release,float clock);
    void step_groups(float clock);
    std::vector<VoiceSnapshot> inspect_live()const;
    uint32_t seed() const {return random_.seed;}
    uint64_t requests() const {return requests_;}
    uint64_t started() const {return started_;}
};
