#ifndef UC_SHELL_EMITTER_H
#define UC_SHELL_EMITTER_H
#include "shell_lifecycle.h"
#include "muzzle_placement.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full BCCB0 tag4/5 names, default ASCII locale, first 16-byte match. The
 * second name is longer than16 bytes; preserve the original prefix match.
 * Output unchanged on invalid input. Use unchanged uc_muzzle_world for each
 * returned index: C01C0 tags2/4/5 share the same actual C0222 matrix branch. */
int uc_shell_bind(const UCModel *,int32_t indices[2]);
typedef struct UCShellEmitterView {
    const uint32_t *suppressed; /*Mainplayer+35C*/
    const int32_t *equipped; /*actual inventory's active item,41F80*/
    const uint32_t *entity_id; /*Mainplayer+54; equipped is also source+4FC*/
    const float *ground_y; /*Mainplayer+44*/
} UCShellEmitterView;
typedef struct UCShellRequest {
    UCShellProperties properties;
    float position[3],rotation[3];
    uint32_t parent_id,bone_tag,mode;
} UCShellRequest;
typedef struct UCShellEmitterServices {
    void *context;
    int (*preflight)(void *,const UCShellEmitterView *,const UCGameRandom *);
    void (*tag_world)(void *,unsigned tag,float out[16]);
    /* Complete20A260/207D20 Shell creation before returning. Request is
     * ephemeral; copy into the real owner. No retained address/second graph.
     * This service may consume this SAME shared RNG and update live fields. */
    void (*create)(void *,const UCShellRequest *,UCGameRandom *);
} UCShellEmitterServices;
/* Complete represented C6670/C4790 for actual Mainplayer46FBE8. Its+38 slot is
 * 2C2B60(NULL); full28A150 permits creation with either null/present51D900.
 * No configurable fake visibility bypass. Other actor subclasses unsupported.
 * Admitted fixed definitions2/5/17 have firstword==item; active item reloads at each
 * loop frontier. Item5 emits up to2, other items1; callbacks can change this.
 * All borrowed fields/current pose remain valid throughout; no aliases with
 * RNG, scratch or request. Preflight is pure and certifies complete services,
 * future valid active items, finite<=1e6 tag matrices/GroundY and finite results.
 * Only create may consume RNG/change live emitter fields; neither callback
 * changes FP environment/reenters. Shared RNG is never cloned or reseeded.
 * Normal nearest FP, FTZ/DAZ off; double-backed original x87 random helpers.
 * Return0 success (including source suppression),1 rejected before effects.
 * Typed request replaces successful temporary property/string storage only;
 * property allocator failure/exception and actual factory lifetime are not
 * implemented. Do not install a success-only create service into gameplay. */
int uc_shell_emit(const UCShellEmitterView *,UCGameRandom *,const UCShellEmitterServices *);
#ifdef __cplusplus
}
#endif
#endif
