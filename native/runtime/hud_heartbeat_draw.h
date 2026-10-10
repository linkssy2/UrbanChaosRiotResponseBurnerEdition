#pragma once
#include "hud.h"
extern "C" {
#include "../hud_heartbeat.h"
}
struct NativeHeartbeatQuads {std::array<NativeHudQuad,56> quads{};size_t count=0;};
/* Original71D70 present-player waveform branch with no actor-name label. The
 * constructor's damage/background widget graph remains a separate slice. */
NativeHeartbeatQuads native_heartbeat_quads(const UCHudHeartbeat &,const float color[4]);
/* Requested PC lifeline extension, synchronized to the original pulse clock.
 * The original waveform and health amount remain unchanged. */
NativeHudAmmo native_lifeline_pulse(const UCHudHeartbeat &,const UCHudHealth &);
struct NativeHeartbeat {
    UCHudHeartbeat history{};
    NativeGpuMesh mesh;
    void load(NativeRenderer &);
    void update(const UCHudHeartbeatActor &,float,UCGameRandom &,void (*sound)(void *),void *);
    /* Borrow the already admitted original Font_Untextured_Alpha texture and
     * live health color. The current-player ID is present; ordinary safe area. */
    void draw(NativeRenderer &,ID3D11ShaderResourceView *,const UCHudHealth &);
};
