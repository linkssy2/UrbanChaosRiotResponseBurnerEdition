#ifndef UC_NATIVE_PLAYER_FIRE_H
#define UC_NATIVE_PLAYER_FIRE_H
#include "mainplayer_actions.h"
#include "weapon_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerFire {
    uint32_t item;
    UCMainplayerActionState *action;
    UCWeaponState *weapon;
    const uint32_t *reserve;
    float clock,delta;
} UCPlayerFire;
typedef struct UCPlayerFireServices {
    int (*preflight)(void *,const UCPlayerFire *);
    void (*action)(void *,uint32_t); /* Real C1E60; publishes animation itself. */
    void (*reset_fire_input)(void *); /*2084E0(11)->210840, before cadence writes.*/
    int (*shot)(void *); /* Real CBA20;0 means completed. Failure retains effects. */
    uint8_t (*controller_done)(void *); /* Current first controller1FB4E0. */
    void (*utility)(void *); /* C5510 */
    void (*switch_weapon)(void *); /* C4CB0 */
    void (*fire_input)(void *); /* D1A20 */
    void (*reload_input)(void *); /* CF770 */
    void (*aim)(void *); /* BE870 */
    void (*stop_fire)(void *); /* BC6A0, before BFE00 state0/stage0 publication. */
    void (*reevaluate)(void *); /* D1AE0, after state0/stage0. */
} UCPlayerFireServices;
typedef enum UCPlayerFireStatus {UC_PLAYER_FIRE_OK,UC_PLAYER_FIRE_ARGUMENT,
    UC_PLAYER_FIRE_UNSUPPORTED,UC_PLAYER_FIRE_EFFECT_FAILED} UCPlayerFireStatus;
/* Full CFBC0 ordinary primary2/17 mode4, stage0 or stage2/Fire3. Stage0 performs
 * the original immediate recursive stage2 evaluation, including its additional
 * outer aim tail. Existing action/weapon/reserve are borrowed distinct owners.
 * No input, shot, sound, cleanup or state-transition service is fabricated.
 * Pure preflight admits all reachable completing effects, including the live
 * controller and any states produced by input services. Callbacks may mutate
 * live action/weapon as their original owners do, but retain binding/item and
 * primary mode, remain nonreentrant and preserve clock/delta/reserve pointer.
 * action, shot and controller_done preserve action.state4. Only the explicit
 * utility/switch/fire/reload input services and reevaluate may change it.
 * reset_fire_input may change input ownership only. A return with applied1 can
 * follow committed effects and must not be retried as a new trigger. */
UCPlayerFireStatus uc_player_fire_update(UCPlayerFire *,const UCPlayerFireServices *,void *,uint8_t *applied);
#ifdef __cplusplus
}
#endif
#endif
