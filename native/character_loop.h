#ifndef UC_CHARACTER_LOOP_H
#define UC_CHARACTER_LOOP_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterLoopController { void *actor; } UCCharacterLoopController;
/* One behaviour's actual mutable fields. world borrows its entity matrix;
 * actor/controller/animation/condition are the live source identities. */
typedef struct UCCharacterLoop {
    UCCharacterLoopController *controller;
    void *actor,*animation,*condition,*bank,*clip;
    float *world,duration,elapsed;
    int32_t next,exit_condition,interrupt_count;
    const int32_t *interrupts;
    uint8_t active,disable_collision;
} UCCharacterLoop;
typedef struct UCCharacterLoopServices {
    void *owner;
    void (*sound)(void *,void *actor,void *bank,int start);
    void (*attach_condition)(void *,UCCharacterLoop *,int32_t descriptor);
    void (*collision)(void *,void *actor,int enabled);
    void *(*animation_owner)(void *,void *actor);
    void (*animation_begin)(void *,void *animation_owner,void *clip,void **output);
    int (*animation_cycle)(void *,void *animation);
    int32_t (*condition)(void *,void *condition);
    int (*interrupt)(void *,UCCharacterLoopController *,int32_t,const int32_t *);
    int (*transition)(void *,UCCharacterLoop *,int32_t descriptor);
    void (*base_exit)(void *,UCCharacterLoop *);
    float (*delta)(void *);
    const float *(*actor_world)(void *,void *actor);
    /* Optional pure admission at the transition boundary. Zero stops BEFORE
     * calling325A0; return2 then exposes incomplete work, never success. Earlier
     * cycle/sound/condition effects are committed. NULL admits the real service. */
    int (*transition_ready)(void *,const UCCharacterLoop *,int32_t descriptor);
} UCCharacterLoopServices;
/* Complete13E7A0/13E8A0/13E850 control flow plus32660/32680 scalar publication.
 * Services are REQUIRED completing original effects, not success observers:
 * attach=2084E0/210CF0/325F0, animation_begin=body+A8 then animation+34
 * (clip,0,0,0,0,0,&output), transition=325A0, base_exit=328F0. The clip,
 * bank, actor, controller and condition owners are supplied, never fabricated.
 * Callback bindings remain stable. Callbacks may alter live state but must
 * preserve valid borrowed pointers, finite durations/elapsed, finite nonnegative
 * delta and finite elapsed+delta, and all later completing services. No reentry
 * or destruction of this behaviour. actor_world is a pure current-field view.
 * <=64 interrupt flags is a native bound; negative source count is preserved.
 * RN/normal denormals required. Return0 completed,1 invalid BEFORE effects,
 * 2 stopped at an explicitly unavailable transition (do not replay that tick).
 * Caller supplies real behaviour scheduling/activation and retains all owners.
 * No AI locomotion, health/death, factory or global behaviour registry implied.
 */
int uc_character_loop_enter(UCCharacterLoop *,UCCharacterLoopController *,const UCCharacterLoopServices *);
int uc_character_loop_update(UCCharacterLoop *,const UCCharacterLoopServices *);
int uc_character_loop_exit(UCCharacterLoop *,const UCCharacterLoopServices *);
/* Complete2107C0 predicate, used by32E60: ALL bytes must be nonzero. Nonpositive
 * signed count returns1. Invalid indexes return-1 before reading flag bytes. */
int uc_character_loop_flags(const uint8_t *flags,size_t size,int32_t count,const int32_t *indices);
#ifdef __cplusplus
}
#endif
#endif
