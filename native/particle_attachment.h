#ifndef UC_PARTICLE_ATTACHMENT_H
#define UC_PARTICLE_ATTACHMENT_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticleAttachment {
    int32_t entity,bone;
    uint8_t dead_when_hidden;
} UCParticleAttachment;
typedef struct UCParticleAttachmentServices {
    void *owner;
    const UCEntityEventRegistry *registry;
    int32_t (*find_bone)(void *,UCEntityEventTarget *,const char *);
    uint8_t (*bone_matrix)(void *,UCEntityEventTarget *,int32_t,float out[16]);
    const float *(*world)(void *,UCEntityEventTarget *);
    const uint32_t *(*flags)(void *,UCEntityEventTarget *);
} UCParticleAttachmentServices;
/* 219740 field construction after successful caller-owned source allocation;
 * no original pool allocation/failure claim. Current registry and completing
 * entity bone-name/matrix/world endpoints are required. The source fields and
 * input name remain stable; callbacks may mutate target pose/world/flags or
 * registry but retain the captured target until return. flags is a pure alias
 * adapter, not an invented source virtual. Registry is the current manager's
 * actual generation registry at entry; source lookup runs once per operation.
 * bind requires a resolvable target; -1 bone is a valid missing-bone result.
 * dead is whole218DC0; matrix is whole219560, including identity fallback,
 * direct Y/Z row exchange and exact pairwise SSE float multiplication order.
 * Finite inputs/results, nearest/gradual FP, disjoint outputs.0 success,
 *1 pre-effect invalid,2 completing-service result invalid (effects retained).
 * Output stays unchanged on failure; never retry status2 as an untouched call.
 * Source storage and entity destruction/scheduling remain caller-owned. */
int uc_particle_attachment_bind(const UCParticleAttachmentServices *,int32_t,
    const char *,uint8_t,UCParticleAttachment *);
int uc_particle_attachment_dead(const UCParticleAttachmentServices *,const UCParticleAttachment *,uint8_t *);
int uc_particle_attachment_matrix(const UCParticleAttachmentServices *,const UCParticleAttachment *,float out[16]);
#ifdef __cplusplus
}
#endif
#endif
