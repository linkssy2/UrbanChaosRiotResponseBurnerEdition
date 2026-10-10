#ifndef UC_NATIVE_POINT_LIGHT_H
#define UC_NATIVE_POINT_LIGHT_H

#include <stdint.h>

/* Native engine values. No Xbox addresses, register context or packet storage.
 * Callbacks are synchronous and may change these objects; do not snapshot their
 * later inputs across a callback. The objects and backend must remain alive. */
typedef struct UcNativePointLight {
    float color[4], intensity;
    uint8_t specular_enabled;
    uint8_t normal_cube_active;
} UcNativePointLight;

typedef struct UcNativeLightProjector {
    const void *texture;
    float transform[16];
} UcNativeLightProjector;

typedef struct UcNativeLightMaterial {
    uint8_t material_mode, alternate_color, vertex_variant;
    float specular_strength, scalar;
    const UcNativeLightProjector *projector;
    int texture_stage;
} UcNativeLightMaterial;

typedef struct UcNativeLightShaders {
    /* [material mode][specular][vertex variant][projector present] */
    const void *vertex[2][2][2][2];
    const void *basic_diffuse, *basic_specular;
    const void *material_diffuse[2], *material_specular[2];
} UcNativeLightShaders;

typedef struct UcNativeLightSampling {
    uint8_t normal_cube_coordinates, changed;
} UcNativeLightSampling;

typedef enum UcNativeLightTexture {
    UC_LIGHT_ATTENUATION_VOLUME,
    UC_LIGHT_NORMALIZATION_CUBE
} UcNativeLightTexture;

typedef enum UcNativeLightConstant {
    UC_LIGHT_COLOR = 0,
    UC_LIGHT_ALTERNATE_COLOR = 1,
    UC_LIGHT_SPECULAR_SCALE = 4,
    UC_LIGHT_MATERIAL_SCALAR = 5
} UcNativeLightConstant;

typedef struct UcNativeLightConstants {
    uint8_t valid[8];
    float value[8][4];
} UcNativeLightConstants;

typedef struct UcNativeLightBackend {
    void *user;
    void *(*resources)(void *user);
    const void *(*texture)(void *user, void *resources, UcNativeLightTexture kind);
    void (*bind_texture)(void *user, unsigned stage, const void *texture);
    void (*texture_coordinates)(void *user, int projected);
    void *(*camera)(void *user);
    const float *(*camera_matrix)(void *user, void *camera);
    void (*projector_matrix)(void *user, const float transposed[16]);
    void (*vertex_program)(void *user, const void *program);
    void (*pixel_program)(void *user, const void *program);
    void *(*exposure_source)(void *user);
    double (*exposure)(void *user, void *source);
    void *(*constant_context)(void *user);
    void (*prepare_constants)(void *user, void *context);
    void (*upload_constant)(void *user, UcNativeLightConstant kind, const float value[4]);
} UcNativeLightBackend;

/* Reconstructed full point-light material transaction. Executes each operation
 * once; callers must not replay it after a backend failure.
 * Distinct typed objects and valid callbacks are the native ownership contract.
 * The selected projector and returned camera matrix remain alive through the
 * matrix callback, even when callbacks change the material's projector pointer.
 * Projected arithmetic preserves operation grouping and floating-point flags;
 * NaN payload selection in malformed/nonfinite transforms is compiler-dependent.
 * This is not yet wired into the playable runtime. */
#ifdef __cplusplus
extern "C" {
#endif
void uc_native_point_light_apply(UcNativePointLight *light,
    UcNativeLightMaterial *material, UcNativeLightShaders *shaders,
    UcNativeLightSampling *sampling, UcNativeLightConstants *constants,
    const UcNativeLightBackend *backend);
#ifdef __cplusplus
}
#endif

#endif
