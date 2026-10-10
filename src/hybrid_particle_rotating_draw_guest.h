#ifndef UC_HYBRID_PARTICLE_ROTATING_DRAW_GUEST_H
#define UC_HYBRID_PARTICLE_ROTATING_DRAW_GUEST_H
/* Complete99DD0 rotating sprite owner, retaining source callbacks, optional
 * legacy GPU hook, diagnostic probe and packet ABI. */
/* legacy is selected by the pinned transform: 0 raw, 1 prior GPU splice. */
int uc_hybrid_particle_rotating_draw(unsigned legacy);
#endif
