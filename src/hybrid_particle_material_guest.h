#ifndef UC_HYBRID_PARTICLE_MATERIAL_GUEST_H
#define UC_HYBRID_PARTICLE_MATERIAL_GUEST_H
/* Exact complete material setup/cleanup. Original command services retain
 * queue ownership and may flush. Zero declines before any guest effect. */
int uc_hybrid_particle_material_setup(void);
int uc_hybrid_particle_material_cleanup(void);
#endif
