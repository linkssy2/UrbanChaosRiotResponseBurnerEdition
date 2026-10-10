#ifndef UC_HYBRID_PARTICLE_SUBMISSION_GUEST_H
#define UC_HYBRID_PARTICLE_SUBMISSION_GUEST_H
int uc_hybrid_particle_submission(void);
/* The five exact copyout owners differ only in return and global result VA. */
int uc_hybrid_particle_bounds_copy(unsigned entry);
#endif
