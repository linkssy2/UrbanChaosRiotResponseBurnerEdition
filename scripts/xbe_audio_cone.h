#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

// XDK 5849 cone state. Angles are full degrees; volume is signed millibels.
struct UcCone {
    uint32_t inside = 360, outside = 360;
    float direction[3] = {0, 0, 1};
    int32_t volume = 0;
};

template<class Voice> void ConeCommit(Voice& v) {
    if (v.cone_dirty & 4) {
        v.cone.inside = v.pending_cone.inside;
        v.cone.outside = v.pending_cone.outside;
    }
    if (v.cone_dirty & 8)
        for (unsigned i = 0; i < 3; ++i) v.cone.direction[i] = v.pending_cone.direction[i];
    if (v.cone_dirty & 16) v.cone.volume = v.pending_cone.volume;
    v.cone_dirty = 0;
}

// 4279CE uses this rational angle approximation, not acos. Preserve its float
// publication of the difference length and its double-precision sum length.
// d is the normalized listener-to-source vector (opposite the cone direction).
inline float ConeAngle(const float direction[3], const float d[3]) {
    float sum[3], difference[3];
    for (unsigned i = 0; i < 3; ++i) {
        sum[i] = float(double(direction[i]) + d[i]);
        difference[i] = float(double(direction[i]) - d[i]);
    }
    auto length = [](const float v[3]) {
        return std::sqrt((double(v[0]) * v[0] + double(v[1]) * v[1]) + double(v[2]) * v[2]);
    };
    float minus = float(length(difference));
    double plus = length(sum);
    return plus < minus ? float((plus / minus * 45.0) * 4.0)
                        : float((90.0 - double(minus) / plus * 45.0) * 4.0);
}

// 427BC0: linear interpolation in integer millibels, including unsigned width
// subtraction and CVTTSS2SI truncation. Equal cone angles use a width of one.
inline int32_t ConeAttenuation(const UcCone& cone, float angle) {
    if (double(angle) <= cone.inside) return 0;
    if (cone.inside < cone.outside && double(angle) >= cone.outside) return cone.volume;
    uint32_t width = cone.outside - cone.inside;
    if (!width) width = 1;
    float value = float((double(angle) - cone.inside) * cone.volume / double(width));
    if (!std::isfinite(value) || double(value) >= 2147483648.0 || double(value) < -2147483648.0)
        return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}

inline float ConeGain(const UcCone& cone, const float d[3]) {
    if (!cone.volume) return 1;
    double length = std::sqrt((double(d[0]) * d[0] + double(d[1]) * d[1]) + double(d[2]) * d[2]);
    // A colocated source has no direction. Do not attenuate an undefined cone.
    if (length == 0) return 1;
    float direction[3];
    for (unsigned i = 0; i < 3; ++i) direction[i] = float(double(d[i]) / length);
    int32_t mb = ConeAttenuation(cone, ConeAngle(cone.direction, direction));
    return mb <= -10000 ? 0.0f : std::pow(10.0f, mb / 2000.0f);
}
