#ifndef UCRR_FEATURE_REMOVAL_H
#define UCRR_FEATURE_REMOVAL_H
#include <windows.h>
#include <stdint.h>
int uc_particle_registration_enabled(void);
int uc_particle_registration_admit(uint32_t r,uint32_t caller);
void uc_registration_00089EB0(void),uc_registration_00089F90(void),uc_registration_00089FF0(void),uc_registration_0008A100(void);
void uc_sky_model_setup(void);
void uc_sky_setup_observe(void);
int uc_glass_probe_enabled(void);
void uc_glass_render_probe(void);
void uc_glass_observe(unsigned stage,uint32_t object,uint32_t count);
unsigned uc_sky_visibility_value(uint32_t owner,uint32_t renderer,unsigned index);
void uc_feature_removal_init(void);
int uc_feature_removal_enabled(void);
int uc_feature_removal_key(UINT message, WPARAM key, LPARAM flags);
void uc_feature_particles(void);
void uc_feature_particles_1(void);
void uc_feature_particles_2(void);
void uc_feature_particles_3(void);
void uc_feature_particles_4(void);
void uc_feature_particles_5(void);
void uc_feature_blob_shadow(void);
#endif
