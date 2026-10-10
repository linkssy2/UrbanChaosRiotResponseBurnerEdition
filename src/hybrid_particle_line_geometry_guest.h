#ifndef UC_HYBRID_PARTICLE_LINE_GEOMETRY_GUEST_H
#define UC_HYBRID_PARTICLE_LINE_GEOMETRY_GUEST_H
/* Complete Line preparation, endpoint/color generation, bounds and shared
 * render-type getter. Operations 0..3 retain the original guest ABI. */
int uc_hybrid_particle_line_geometry(unsigned operation);
#endif
