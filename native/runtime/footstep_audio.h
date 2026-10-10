#pragma once
#include "audio_events.h"
/* Direct nonspatial PlayerFootsteps event, one immediate opcode1/00A4 or
 * opcode3/0084 mode4 command, or opcode1/0024 weighted mode0 command.
 * Requires absent bank observers, no named sound-arbitration key, a current shared Game3D
 * mix and nonconflicting fresh one-shot policy. Every possible selected row's
 * actual resident PCM/rate/range is admitted before RNG/table mutation. Mode4
 * uses the SAME shared audio RNG after pitch/volume, with persistent markers.
 * Weighted mode0 consumes one draw for multi-row tables and keeps them unchanged.
 * Spatial NPC Footsteps, WoodenHoles' arbitration/two-track program and subsounds remain
 * unsupported. Backend errors after commitment are not retryable. */
NativeAudioPlayResult native_audio_play_footstep(NativeAudioDevice &,
    NativeAudioBankAssets &,const char *event,const NativeAudioInstanceMix &,UCAudioRandom &);
// Same command/selector path, actual NPC Footsteps bank policy. Explicit PC
// nonspatial output; original distance curves and per-source arbitration are
// not supplied by this adapter. Authored gain/pitch and shared RNG are retained.
NativeAudioPlayResult native_audio_play_character_footstep(NativeAudioDevice &,
    NativeAudioBankAssets &,const char *event,const NativeAudioInstanceMix &,UCAudioRandom &);
