#pragma once
#include "hud.h"
extern "C" {
#include "../mainplayer_actions.h"
}
struct NativeReticleFade {float alpha=0,target=0;uint32_t mode=0;};
struct NativeReticleHistory {NativeReticleFade items[2];};
/* Original item2/17 rest reticles only. No scoped view, target identification,
 * hit confirmation, original outer HUD admission.
 * Item1 (Taser) remains hidden without an identified target, as73A50 requires.
 * Item0 shield has no loaded crosshair sprite; primary fades still advance.
 * Item11 Molotov currently fades primaries; its throw-aim HUD is not decoded.
 * Ordinary player+1CC=0 and white/no-target/no-recent-hit tint are explicit
 * admitted policies. Opacity is the source signed profile byte (0..8 here),
 * not a save setting loaded by this owner. Animation/state/latch are borrowed. */
void native_reticle_step(NativeReticleHistory &,const UCMainplayerState &,
    const UCMainplayerActionState &,float delta,uint8_t opacity);
NativeHudAmmo native_reticle_quads(const NativeReticleHistory &);
struct NativeReticle {
    NativeReticleHistory history{};
    std::array<ComPtr<ID3D11ShaderResourceView>,2> textures;
    NativeGpuMesh mesh;
    void load(const UCArchive *,NativeRenderer &);
    void update(const UCMainplayerState &inventory,const UCMainplayerActionState &action,
                float delta,uint8_t opacity){native_reticle_step(history,inventory,action,delta,opacity);}
    void draw(NativeRenderer &);
};
