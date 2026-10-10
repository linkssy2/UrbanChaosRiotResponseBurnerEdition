#ifndef UC_DECAL_SUBMIT_H
#define UC_DECAL_SUBMIT_H
#include "decal_quad.h"
#include "footprint.h"
/* Actual EB150 request through2088D0/223BC0: NULL tangent, square size,
 * mask0x4000, flags0, fade0. Borrows existing publication services. Preflight
 * sees a request with unused zero tangent and must admit the full synchronous
 * query/cache/publication work and valid immutable inputs. The random stream
 * advances before the first ray, including requests rejected by geometry or an
 * absent manager. Returns0 completed/missed,1 pre-effect rejection,2 committed
 * cache-service failure; never retry2. submitted records publication, not pixels.
 * Uses the finite/RN domain of uc_decal_quad, stable nonreentrant services. */
int uc_bullet_decal_submit(const char *shader,const float point[3],const float normal[3],
    float size,UCVectorRandom *,const UCFootprintServices *,uint8_t *submitted);
#endif
