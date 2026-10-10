#ifndef UC_PARTICLE_REGISTRY_H
#define UC_PARTICLE_REGISTRY_H
#include "particle_lifecycle.h"
#include "gunflare_sprite.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleRegistry UCParticleRegistry;
/* Bounded concrete plain0 registration8A100, rotating3 registration89FF0,
 * motion1/rotating-quad2/stretched5 registration89F90, and shared89AB0 removal.
 * Owns stable wrapper/node storage; borrows the lifecycle view until removal.
 * Original successful pool/list allocation is replaced by preallocated native
 * storage. Exhausted pool returns success with token0 and makes no list entry.
 * A fresh wrapper is unique even when two registrations borrow the same view.
 * Insertion order is retained; removal unlinks only that unique wrapper.
 * The original null-after-nonfull allocation failure and foreign/duplicate list
 * nodes are outside this domain. No culling/sort/draw or provider singleton.
 * Tokens expire on removal, may be reused, and belong to this owner only.
 * Iteration is quiescent; never keep a borrowed view past lifecycle unregister.
 * All owners/outputs disjoint, same-thread/no callbacks. Return0 success,
 * 1 invalid input/inactive or foreign token/nonempty close,2 allocation failure.
 */
int uc_particle_registry_create(uint32_t capacity,UCParticleRegistry **);
int uc_particle_registry_add(UCParticleRegistry *,const UCParticleLifecycleView *,uintptr_t *token);
/* Same rotating3 wrapper/pool/list, borrowing the actual GunFlare sprite
 * virtual-source projection rather than a fabricated lifecycle. Duplicate
 * sources still get distinct wrappers. Both source accessors return NULL for
 * the other source kind; source kind is not the wrapper family (both are3). */
int uc_particle_registry_add_sprite(UCParticleRegistry *,const UCGunFlareSpriteSource *,uintptr_t *token);
int uc_particle_registry_remove(UCParticleRegistry *,uintptr_t token);
uint32_t uc_particle_registry_count(const UCParticleRegistry *);
uintptr_t uc_particle_registry_first(const UCParticleRegistry *);
uintptr_t uc_particle_registry_next(const UCParticleRegistry *,uintptr_t);
const UCParticleLifecycleView *uc_particle_registry_view(const UCParticleRegistry *,uintptr_t);
const UCGunFlareSpriteSource *uc_particle_registry_sprite(const UCParticleRegistry *,uintptr_t);
int uc_particle_registry_close(UCParticleRegistry *);
#ifdef __cplusplus
}
#endif
#endif
