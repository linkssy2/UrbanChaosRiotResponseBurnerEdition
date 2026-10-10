#ifndef UC_GLASS_BULLET_H
#define UC_GLASS_BULLET_H
#include "bullet_scenery.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassBulletHit {float fraction,point[2],normal[3];} UCGlassBulletHit;
typedef struct UCGlassBulletSink {
    void *owner;
    int (*preflight)(void *,const float quad[4][2],const float impact[6]);
    /* Actual receiver F0 virtual+4,450B3C->53520 for the authored glass class.
     * Must complete real shard work, not report a fabricated success. It may
     * mutate live owner fields, but retains their slots and the view through
     * return; local quad/impact snapshots are immutable and never retained. */
    int (*submit)(void *,const float quad[4][2],const float impact[6]);
} UCGlassBulletSink;
typedef struct UCGlassBullet {
    const float *world;                 /* actual receiver+B0,16 floats */
    const float (*quad)[2];             /* actual receiver+84,4 local points */
    uint8_t *triggered;                 /* actual receiver+A4 */
    const UCGlassBulletSink *const *sink;/* actual receiver+F0 native binding */
} UCGlassBullet;
/* Full51D00 finite query using the already qualified2D4360 inverse. Native x86
 * RCPSS approximation, RN/gradual underflow; nonsingular matrix and bounded
 * finite local/world coordinates (abs<=1e6). Disjoint stable borrowed inputs.
 * Exact strict0<t<1 and local edge test, including source partial output on
 * miss: fraction may change before point, normal changes only on hit.
 * Return0 executed (accepted is0/1),1 invalid before output publication. */
int uc_glass_bullet_query(const float world[16],const float quad[4][2],
    const UCBulletSegment *,UCGlassBulletHit *,uint8_t *accepted);
/* Full520E0/520F0 ->52290 represented glass response. Already-triggered owner
 * returns without reading later inputs. A real miss completes without a sink.
 * A hit transforms the segment again, forms pointXY/velocityXYZ/mass, snapshots
 * the current quad, invokes its completing sink, THEN publishes triggered=1.
 * Finite nonzero delta and finite local velocity required on the hit branch;
 * pure preflight promises submit can finish. Return0 complete,1 pre-effect
 * rejection,2 partial sink failure (not retryable; no trigger publication).
 * No constructor, shard factory/render/audio or complete world claim. */
int uc_glass_bullet_receive(const UCGlassBullet *,const UCBulletSegment *);
#ifdef __cplusplus
}
#endif
#endif
