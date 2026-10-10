#pragma once
#include <cmath>
extern "C" {
#include "../game_random.h"
}
//27930: source fields44/48/4C/50/52; the difficulty callback is evaluated
//after both shared RNG draws, with its x87-width value retained to final store.
struct NativeCopCadence {float elapsed=0,delay=0,interval=0;bool enabled=false,held=false;};
inline int native_cop_cadence_init(NativeCopCadence &s,UCGameRandom &random,
    void *owner,int (*difficulty)(void *,double *)){
    if(!difficulty)return 1;
    s.elapsed=0;s.enabled=true;s.held=false;int32_t draw=0;
    if(uc_game_random_next(&random,&draw))return 1;
    s.delay=float((double(draw)*(double(.7f)-.25f))*0x1p-31+.25f);
    if(uc_game_random_next(&random,&draw))return 1;
    s.interval=float((double(draw)*(double(.5f)-.25f))*0x1p-31+.25f);
    double value=0;if(difficulty(owner,&value)||!std::isfinite(value))return 1;
    s.interval=float((2.0-value*1.25)*s.interval);return 0;
}
