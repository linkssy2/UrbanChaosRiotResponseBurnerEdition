#ifndef UC_NATIVE_FIRSTPERSON_PROJECTION_H
#define UC_NATIVE_FIRSTPERSON_PROJECTION_H
#ifdef __cplusplus
extern "C" {
#endif
/* Original21E260 perspective and BAC10 draw-scoped depth slice. Supplied
 * resolved viewport aspect (original missing-owner fallback is float4/3).
 * Finite, 0<fov<180, aspect>0, 0<near<far, 0<=low<=high<=1. Native errors
 * (-1 argument, -2 unsupported arithmetic/mode) leave output unchanged.
 * Slice permits input==output; ordinary projection need never be mutated.
 * Strict nearest-even arithmetic/denormals; double/libm original comparison
 * does not certify physical Xbox FPTAN bit identity. */
int uc_firstperson_projection(float fov_degrees,float aspect,float near_z,float far_z,
                             float depth_low,float depth_high,float out[16]);
int uc_firstperson_depth_slice(const float projection[16],float low,float high,float out[16]);
#ifdef __cplusplus
}
#endif
#endif
