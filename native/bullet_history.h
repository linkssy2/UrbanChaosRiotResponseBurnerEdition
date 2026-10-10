#ifndef UC_NATIVE_BULLET_HISTORY_H
#define UC_NATIVE_BULLET_HISTORY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletHistoryEntry {
    uint32_t owner_id,bullet_id;
    float from[3],to[3];
} UCBulletHistoryEntry;
typedef struct UCBulletHistory {
    float player_shot_age;
    uint32_t count;
    UCBulletHistoryEntry entries[200];
} UCBulletHistory;
/* Actual2C3C0/2C170 field initialization into fresh caller-owned storage.
 * Leaves each entry's bullet_id untouched, as the original constructor does.
 * No second queue, allocation or game-manager registration is introduced. */
int uc_bullet_history_init(UCBulletHistory *);
/* Actual2C1C0: append up to200 entries; excess shots are dropped, yet a matching
 * local player's shot still resets age to positive zero even when full.
 * local_player_id borrows actual session+834, or NULL if session is absent.
 * Source getters have no callbacks and stable session storage is required.
 * Raw coordinate bits are copied, with no normalization or finite-value gate.
 * Entry/ID pointers and owner are valid, disjoint, immutable during this call. */
int uc_bullet_history_append(UCBulletHistory *,const UCBulletHistoryEntry *,
                            const uint32_t *local_player_id);
/* Actual2C1A0 frame boundary: clear count without touching entries, advance age
 * by the actual1F9010 delta. Finite inputs/output, RN/gradual underflow, lifted
 * double-backed x87 domain. Zero and negative delta preserve source behavior.
 * Both functions return0 completed,1 rejected before any effects. No NPC
 * reaction, suppression test2C290 or outer frame scheduling is implied. */
int uc_bullet_history_advance(UCBulletHistory *,float delta);
#ifdef __cplusplus
}
#endif
#endif
