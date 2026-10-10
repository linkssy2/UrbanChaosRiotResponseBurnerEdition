#pragma once
#include "assets.h"
extern "C" {
#include "../mainplayer_state.h"
#include "../hud_health.h"
}
#include <array>
class NativeInputBindings;

/* Authored ammo/health slices and PC equipment controls. Heartbeat, original inventory carousel,
 * auxiliary ammo, low-ammo pulse, mission admission and the original outer UI
 * owner remain separate. Health history advances only through explicit ticks. */
struct NativeHudQuad {
    float x,y,width,height,u0,v0,u1,v1;
    uint32_t texture; /* 0..6 ammo/health; 7..9 item art; 10..12 keys; 13 unused; 14/15 Molotov/key. */
    uint32_t color=0xffffffffu;
};
struct NativeHudAmmo {std::array<NativeHudQuad,8> quads{};size_t count=0;};
/* Ordinary layout0, category0. Coordinates retain the original640x480 safe
 * area and half-pixel sprite rule. Unsupported equipment emits no quads;
 * invalid supported snapshots throw before any graphics operation. */
NativeHudAmmo native_hud_ammo(const UCMainplayerState &,
    const UCMainplayerItemSpec &);
/* Authored health frame + current/delayed fills; heartbeat is a separate owner. */
NativeHudAmmo native_hud_health(const UCHudHealth &);
/* PC quick slots: original equipment artwork, current inventory/selection,
 * graphical keys. Does not grant equipment or simulate a selection. */
NativeHudAmmo native_hud_inventory(const UCMainplayerState &);
struct NativeHud {
    std::array<ComPtr<ID3D11ShaderResourceView>,16> textures;
    NativeGpuMesh mesh;
    UCHudHealth health{};
    uint32_t health_mode=0,health_tier=0;
    bool health_initialized=false;
    /* Exact fixed-revision shader/texture identities are checked at load;
     * only the recovered typed material is submitted. Archive may then close. */
    void load(const UCArchive *,NativeRenderer &);
    void load_inventory(const std::filesystem::path &game,NativeRenderer &);
    /* Rebuild only equipment keycaps whose bindings changed. */
    void update_bindings(NativeRenderer &,const NativeInputBindings &);
    /* Explicit session/progression snapshot, not inferred from health. Primes
     * the first display at delta0 after inventory reset. Current runtime uses0/0. */
    void initialize_health(float current_health,uint32_t session_mode,uint32_t tier,float scale=2);
    /* Current runtime's defined fixed session/progression snapshot. Once per tick,
     * after actual health changes, never from render or while paused. */
    void update_health(float current_health,float delta);
    void draw(NativeRenderer &,const UCMainplayerState &,const UCMainplayerItemSpec &);
private:
    std::array<unsigned,4> binding_keys_{};
};
