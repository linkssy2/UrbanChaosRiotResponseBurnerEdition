#ifndef UC_HYBRID_PARTICLE_LINE_GUEST_H
#define UC_HYBRID_PARTICLE_LINE_GUEST_H
/* Complete 9A4F0 line draw owner. Original source and device callbacks retain
 * their live objects, command queue and completing-call contract. A zero
 * result is possible only before guest effects. Staged, not yet qualified. */
int uc_hybrid_particle_line_draw(void);
#endif
