#pragma once
#include <array>

// Private co-op layout derived from both campaign helicopter interiors. The
// PLAYERBONE's local X follows Chopper-Body Y. These two positions place feet
// at cabin Y=-26 and +24, retaining the authored height and outward facing.
// The gun-only mesh (including its ammo box) is 77.0527 units wide unscaled;
// the compact pair has an 11.474-unit gap inside the conservative doorway.
namespace coop_turret_mount {
inline constexpr std::array<std::array<float,3>,2> kPlayerOffset{{
    {{-10.6554f,0.f,0.f}},{{39.3446f,0.f,0.f}}
}};
inline constexpr float kGunScale=0.5f;
// Gun98 pivots in the original helicopter's Chopper-Body coordinates. The
// actual posed native weapon must agree with these positions before packaging.
inline constexpr std::array<std::array<float,3>,2> kGunPivot{{
    {{-145.f,-32.8254555f,-60.f}},{{-145.f,17.1745445f,-60.f}}
}};
}
