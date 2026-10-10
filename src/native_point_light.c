#include "native_point_light.h"
#include <xmmintrin.h>

static void constant(UcNativeLightConstants *state,
    const UcNativeLightBackend *backend, UcNativeLightConstant kind,
    const float value[4])
{
    __m128 incoming = _mm_loadu_ps(value);
    if (state->valid[kind] && !_mm_movemask_ps(_mm_cmpneq_ps(
            _mm_loadu_ps(state->value[kind]), incoming))) return;
    _mm_storeu_ps(state->value[kind], incoming);
    state->valid[kind] = 1;
    void *context = backend->constant_context(backend->user);
    backend->prepare_constants(backend->user, context);
    backend->upload_constant(backend->user, kind, value);
}

static void pixel_program(UcNativePointLight *light,
    UcNativeLightConstants *constants, const UcNativeLightBackend *backend,
    const void *program)
{
    backend->pixel_program(backend->user, program);
    void *source = backend->exposure_source(backend->user);
    __m128 color = _mm_mul_ps(_mm_loadu_ps(light->color),
        _mm_set1_ps(light->intensity));
    /* Color is captured before the exposure callback; its result is rounded
     * once to float, then all four channels use the original packed divide. */
    float exposure = (float)backend->exposure(backend->user, source);
    float value[4];
    _mm_storeu_ps(value, _mm_div_ps(color, _mm_set1_ps(exposure)));
    constant(constants, backend, UC_LIGHT_COLOR, value);
}

static void projected_matrix(float transposed[16], const float camera[16],
    const float projector[16])
{
    __m128 p0 = _mm_loadu_ps(projector), p1 = _mm_loadu_ps(projector + 4);
    __m128 p2 = _mm_loadu_ps(projector + 8), p3 = _mm_loadu_ps(projector + 12);
    __m128 rows[4];
    for (unsigned i = 0; i < 4; ++i) {
        __m128 row = _mm_loadu_ps(camera + i * 4);
        __m128 w = _mm_mul_ps(p3, _mm_shuffle_ps(row, row, 0xFF));
        __m128 zw = _mm_add_ps(_mm_mul_ps(p2, _mm_shuffle_ps(row, row, 0xAA)), w);
        __m128 y = _mm_mul_ps(p1, _mm_shuffle_ps(row, row, 0x55));
        __m128 xy = _mm_add_ps(_mm_mul_ps(p0, _mm_shuffle_ps(row, row, 0)), y);
        rows[i] = _mm_add_ps(xy, zw);
    }
    /* Bitwise transpose after the complete product. */
    _MM_TRANSPOSE4_PS(rows[0], rows[1], rows[2], rows[3]);
    for (unsigned i = 0; i < 4; ++i) _mm_storeu_ps(transposed + i * 4, rows[i]);
}

void uc_native_point_light_apply(UcNativePointLight *light,
    UcNativeLightMaterial *material, UcNativeLightShaders *shaders,
    UcNativeLightSampling *sampling, UcNativeLightConstants *constants,
    const UcNativeLightBackend *backend)
{
    /* The original comparison precedes resource callbacks. Its result remains
     * fixed even if a callback subsequently changes strength or enablement. */
    double strength = material->specular_strength;
    double threshold = 0.05f;
    int specular = !(strength != strength || strength < threshold) &&
        light->specular_enabled != 0;
    int activated_cube = 0;
    if (material->projector) {
        backend->bind_texture(backend->user, 1, material->projector->texture);
        backend->texture_coordinates(backend->user, 1);
        void *camera = backend->camera(backend->user);
        const UcNativeLightProjector *projector = material->projector;
        const float *matrix = backend->camera_matrix(backend->user, camera);
        float transposed[16];
        projected_matrix(transposed, matrix, projector->transform);
        backend->projector_matrix(backend->user, transposed);
    } else {
        void *resources = backend->resources(backend->user);
        const void *texture = backend->texture(backend->user, resources,
            UC_LIGHT_ATTENUATION_VOLUME);
        backend->bind_texture(backend->user, 1, texture);
        backend->texture_coordinates(backend->user, 0);
    }

    int material_mode = material->material_mode || material->alternate_color;
    if (!material_mode) material->texture_stage = -1;
    backend->vertex_program(backend->user,
        shaders->vertex[material_mode][specular]
            [material->vertex_variant != 0][material->projector != 0]);

    /* Shader selection and all following inputs are deliberately read after
     * the vertex-program callback, matching the original operation order. */
    if (!material_mode) {
        pixel_program(light, constants, backend,
            specular ? shaders->basic_specular : shaders->basic_diffuse);
        if (specular) {
            void *resources = backend->resources(backend->user);
            const void *texture = backend->texture(backend->user, resources,
                UC_LIGHT_NORMALIZATION_CUBE);
            backend->bind_texture(backend->user, 0, texture);
            sampling->normal_cube_coordinates = 1;
            sampling->changed = 1;
            light->normal_cube_active = 1;
            activated_cube = 1;
        }
    } else {
        pixel_program(light, constants, backend, specular ?
            shaders->material_specular[material->alternate_color != 0] :
            shaders->material_diffuse[material->alternate_color != 0]);
        material->texture_stage = 0;
    }

    if (material->alternate_color) {
        const float value[4] = {0, 1, 0, 0};
        constant(constants, backend, UC_LIGHT_ALTERNATE_COLOR, value);
    }
    if (light->normal_cube_active && !activated_cube) {
        sampling->normal_cube_coordinates = 0;
        sampling->changed = 1;
        light->normal_cube_active = 0;
    }
    if (specular) {
        /* Scalar SSE preserves the authored multiply and MXCSR behavior. No
         * contraction, color normalization or finite-value substitution. */
        float scale = _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(light->intensity),
            _mm_set_ss(material->specular_strength)));
        const float value[4] = {scale, scale, scale, scale};
        constant(constants, backend, UC_LIGHT_SPECULAR_SCALE, value);
    }
    float scalar = material->scalar;
    const float value[4] = {scalar, scalar, scalar, scalar};
    constant(constants, backend, UC_LIGHT_MATERIAL_SCALAR, value);
}
