#ifndef UC_NATIVE_MAINPLAYER_STATE_H
#define UC_NATIVE_MAINPLAYER_STATE_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCMainplayerStatus {
    UC_MAINPLAYER_OK, UC_MAINPLAYER_ARGUMENT, UC_MAINPLAYER_FORMAT,
    UC_MAINPLAYER_UNSUPPORTED, UC_MAINPLAYER_MISSING_DEFINITION
} UCMainplayerStatus;
typedef struct UCMainplayerProperties {
    UCLevelBytes flags[3]; /* Borrowed little-endian u32 arrays: disable orders,
                            enable weapons, enable shield. Level must outlive. */
    const char *shield_animation, *sound_banks[6]; /* NULL means source -1. */
    int32_t start_without_pistol_ammo;
    uint8_t start_with_weapons;
} UCMainplayerProperties;
/* Resolved original44210/44160 outputs, not guessed static defaults. */
typedef struct UCMainplayerItemSpec {
    uint32_t category, ammo_category, clip_size, reserve_factor;
    uint8_t available;
} UCMainplayerItemSpec;
/* Fixed-revision original definitions plus 44210/44160/64400 upgrade rules.
 * mode is the low-byte result of original4A840 virtual+20, not the reset's
 * alternate_mode flag. upgrades are original progression+50 bytes0..35;
 * caller owns their save/session provenance. Output is unchanged on failure. */
UCMainplayerStatus uc_mainplayer_item_spec(uint32_t item, uint8_t mode,
    const uint8_t upgrades[36], UCMainplayerItemSpec *out);
typedef struct UCMainplayerResolveInput {
    uint32_t session_bypass;
    int32_t parsed_level_number;
    uint32_t progression_class, selected_record_first;
    uint8_t special_level, has_selected_record;
    uint8_t pistol_override, pistol_extended, rifle_override;
    uint8_t upgrades[48];
} UCMainplayerResolveInput;
/* Pure54CE0/54BF0/54C70 item selection, with stable caller-owned inputs.
 * session_bypass is session+74; special_level means an exact six-byte match to
 * "99-01\0". parsed_level_number is the original decimal-prefix parser result
 * BEFORE54BF0 aliases; locale/string parsing remains outside this function.
 * has_selected_record=0 represents session+78==-1. Otherwise the caller must
 * resolve a valid record and supply its first word; no save array is indexed.
 * progression_class is progression+84; override bytes are +17C/+184/+187;
 * upgrades are progression+50 bytes0..47. Nonzero bytes mean true.
 * Raw u32 output preserves passthrough IDs and UINT32_MAX means omitted.
 * Output is unchanged on failure. This does not load saves or initialize the
 * player's equipment resources, actions or presentation. */
UCMainplayerStatus uc_mainplayer_resolve_item(uint32_t requested,
    const UCMainplayerResolveInput *input, uint32_t *out);
typedef struct UCMainplayerResetInput {
    int32_t resolved_items[6]; /* Original54CE0(2,17,20,1,13,14), -1 omitted. */
    uint8_t health_upgrades[3]; /* Original progression bytes12,13,14. */
    uint8_t alternate_mode, no_saved_override;
    const UCMainplayerItemSpec *items; uint32_t item_count;
} UCMainplayerResetInput;
typedef struct UCMainplayerState {
    uint8_t owned[27], seen_items[27]; /* Explicit original42C00 side effect. */
    uint32_t clip[27], ammo[9], ammo_capacity[9];
    int32_t active_item, requested_item, previous_item, category_item[3];
    uint32_t category_order[3][27], category_count[3];
    float health; /* Original reset health; existing maximum uses max(old,this). */
} UCMainplayerState;
/* 42C00 scalar seen/selection/order publication, shared by reset and grants.
 * Does not grant ownership/ammo or perform421B0 presentation refresh. Caller
 * must refresh when this changes requested_item from -1. Invalid state leaves
 * output unchanged; definitions must cover every existing category member. */
UCMainplayerStatus uc_mainplayer_record_item(UCMainplayerState *, uint32_t item,
    const UCMainplayerItemSpec *, uint32_t item_count);
UCMainplayerStatus uc_mainplayer_properties(const UCLevel *, uint32_t entity,
    UCMainplayerProperties *out);
/* Scalar no-save CC2F0 inventory/health subset. Success is NOT entity factory
 * completion: weapon timing/recoil, animation/model/audio, progression owner,
 * input and world components remain the host's required bindings. Nonzero
 * no-pistol-ammo requests need43CE0 and are explicitly unsupported. Output
 * remains unchanged on failure. No implicit stage/progression/save defaults. */
UCMainplayerStatus uc_mainplayer_reset(const UCMainplayerProperties *,
    const UCMainplayerResetInput *, UCMainplayerState *out);
typedef struct UCMainplayerEquipControl {
    uint8_t select_empty, select_previous;
} UCMainplayerEquipControl;
/* Original42270 scalar commit, called by the equipment owner after reset or a
 * selection request. select_empty has priority and selects item0, preserving
 * requested as previous; select_previous restores previous; otherwise requested
 * becomes active and the old active becomes previous. Consumed flag alone is
 * cleared. Original421B0 flashlight/presentation refresh remains the caller's
 * next operation; this does not load a model or start an animation. State and
 * controls are disjoint. Invalid item IDs leave both unchanged. */
UCMainplayerStatus uc_mainplayer_equipment_commit(UCMainplayerState *,
    UCMainplayerEquipControl *);
#ifdef __cplusplus
}
#endif
#endif
