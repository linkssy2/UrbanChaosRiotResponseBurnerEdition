#ifndef UC_NATIVE_MATERIAL_ASSET_H
#define UC_NATIVE_MATERIAL_ASSET_H
#include "archive_reader/archive_reader.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMaterialSpan { const uint8_t *data; uint32_t size; } UCMaterialSpan;
typedef struct UCMaterialDependency {
    uint8_t kind; /* 0 = parameter, nonzero = resource in original90AD0. */
    uint16_t name_bytes;
    const char *name; /* Exact source bytes, appended NUL; no case normalization. */
} UCMaterialDependency;
typedef struct UCMaterialPass {
    uint8_t header[16];
    uint8_t dependency_count, dependency_indices[4]; /* Bound in stage order. */
    UCMaterialSpan vertex_program, vertex_declaration, pixel_program;
} UCMaterialPass;
typedef struct UCMaterialAsset {
    uint8_t header[14];
    uint16_t source_flags, effective_flags;
    uint32_t dependency_count, pass_count;
    UCMaterialDependency *dependencies;
    UCMaterialPass *passes;
    UCMaterialSpan setup_program[2];
    uint8_t *storage; size_t storage_bytes;
} UCMaterialAsset;
typedef enum UCMaterialStatus {
    UC_MATERIAL_OK, UC_MATERIAL_ARGUMENT, UC_MATERIAL_FORMAT,
    UC_MATERIAL_UNSUPPORTED, UC_MATERIAL_LIMIT, UC_MATERIAL_MEMORY,
    UC_MATERIAL_ARCHIVE
} UCMaterialStatus;
/* Structural asset loading only: no execution of engine scripts, Xbox vertex
 * programs or combiner tokens. Native rendering requires an independently
 * recovered/converted native material descriptor. These raw spans are available
 * for offline conversion and evidence, not a shader VM in the game loop.
 * Output must start zero/freed; failure leaves zero, success owns all data. */
UCMaterialStatus uc_material_decode(const void *data,size_t size,UCMaterialAsset *out);
UCMaterialStatus uc_material_load(const UCArchive *archive,size_t index,
    UCArchiveSource source,UCMaterialAsset *out);
void uc_material_free(UCMaterialAsset *material);
const char *uc_material_status(UCMaterialStatus status);

/* Offline-converted catalog: final native renderer consumes named PC material
 * formulas and texture names, never original shader/script instructions. */
typedef enum UCNativeMaterialKind {
    UC_NATIVE_MATERIAL_UNSUPPORTED=0,
    UC_NATIVE_MATERIAL_TEXTURE_VERTEX_COLOR=1,
    UC_NATIVE_MATERIAL_TEXTURE_VERTEX_CONSTANT=2,
    UC_NATIVE_MATERIAL_VERTEX_CONSTANT=3, /* No texture; empty texture_name. */
    UC_NATIVE_MATERIAL_ALPHA_PREPASS=4 /* Separate8FF30 texture*C2; ordinary entry draws nothing. */
} UCNativeMaterialKind;
typedef enum UCNativeMaterialCull {
    UC_NATIVE_MATERIAL_CULL_INHERIT=0,
    UC_NATIVE_MATERIAL_CULL_NONE=1
} UCNativeMaterialCull;
typedef enum UCNativeMaterialAddressV {
    UC_NATIVE_MATERIAL_ADDRESS_V_INHERIT=0,
    UC_NATIVE_MATERIAL_ADDRESS_V_CLAMP=1
} UCNativeMaterialAddressV;
typedef enum UCNativeMaterialAddressU {
    UC_NATIVE_MATERIAL_ADDRESS_U_INHERIT=0,
    UC_NATIVE_MATERIAL_ADDRESS_U_CLAMP=1
} UCNativeMaterialAddressU;
typedef enum UCNativeMaterialBlend {
    UC_NATIVE_MATERIAL_BLEND_OPAQUE=0, /* Prototype choice; original parent pass. */
    UC_NATIVE_MATERIAL_BLEND_ALPHA=1,  /* Authored SRC_ALPHA / INV_SRC_ALPHA. */
    UC_NATIVE_MATERIAL_BLEND_ADDITIVE=2, /* Authored SRC_ALPHA / ONE. */
    UC_NATIVE_MATERIAL_BLEND_REVERSE_SUBTRACT=4 /* D - S*alpha; renderer3 is parent multiply. */
} UCNativeMaterialBlend;

typedef enum UCNativeMaterialGeometry {
    UC_NATIVE_MATERIAL_GEOMETRY_STATIC=0,
    UC_NATIVE_MATERIAL_GEOMETRY_SKIN3=1 /* uc_model_skin_position contract. */
} UCNativeMaterialGeometry;
typedef enum UCNativeMaterialAlpha {
    UC_NATIVE_MATERIAL_ALPHA_TEXTURE_VERTEX=0,
    UC_NATIVE_MATERIAL_ALPHA_VERTEX=1 /* Exact constant alpha1; ignores texture alpha. */
} UCNativeMaterialAlpha;
typedef enum UCNativeMaterialColorWrite {
    UC_NATIVE_MATERIAL_WRITE_INHERIT=0,
    UC_NATIVE_MATERIAL_WRITE_RGBA=1
} UCNativeMaterialColorWrite;
typedef enum UCNativeMaterialTextureSelection {
    UC_NATIVE_MATERIAL_TEXTURE_DIRECT=0,
    UC_NATIVE_MATERIAL_TEXTURE_SEQUENCE_PERIOD1=1 /* uc_texture_sequence_select. */
} UCNativeMaterialTextureSelection;
typedef struct UCNativeMaterial {
    UCNativeMaterialKind kind;
    uint32_t unsupported_reason;
    UCNativeMaterialCull cull_mode;
    UCNativeMaterialAddressV address_v;
    UCNativeMaterialAddressU address_u;
    UCNativeMaterialBlend blend_mode;
    uint8_t vertex_exposure, depth_bias;
    uint32_t constant_argb; /* Kind2: quantized pixel constant; kind1: FFFFFFFF. */
    UCNativeMaterialGeometry geometry;
    UCNativeMaterialAlpha alpha_mode;
    UCNativeMaterialColorWrite color_write;
    UCNativeMaterialTextureSelection texture_selection;
    const char *name, *texture_name;
    uint8_t source_sha256[32];
    float uv_period[2]; /* UCMAT008: zero=literal0; otherwise signed fmod(phase/period,1). */
    uint8_t rotor_uv; /* UCMAT009 byte11 bit4: centered clockwise UV rotation, period .3f. */
} UCNativeMaterial;
typedef struct UCNativeMaterialCatalog {
    uint32_t count;
    UCNativeMaterial *materials;
    uint8_t source_archive_sha256[32];
    uint8_t *storage; size_t storage_bytes;
} UCNativeMaterialCatalog;
/* UCMAT009 retains the56-byte record; byte11 bit4 selects the exact skinned
 * rotor formula: angle=fmod(float(phase/.3f+0),1)*6.2831854820251465f,
 * UV=(u-.5,v-.5)*(cos,-sin;sin,cos)+(.5,.5). No scroll/IFL combination.
 * Formats006/007/008 remain accepted. The converter emits009 only if needed.
 * UCMAT008 adds two float UV periods after the retained48-byte record.
 * Zero is a literal-zero axis; nonzero gives signed fmod(float(phase/period+0),1).
 * Phase is the caller's current renderer clock. Reverse subtraction is D-S*alpha.
 * Kind4 is explicitly a separate8FF30 alpha prepass: sample*C2, alpha>=127,
 * with static flags21 or the exact flags25 skinned shield recipe. CPU skinning
 * supplies the latter's posed coordinates; its ordinary material has no draw.
 * RGB writes, LEQUAL depth writes. It never implies an ordinary8FD80 draw.
 * The caller supplies ambient C2/exposure and schedules that pass; the PC preview
 * chooses white ambient, not a claim about original scene ambient ancestry.
 * Formats006/007 remain byte-layout compatible and get literal-zero periods.
 */
/* Texture/color family: BGRA packed source color -> RGBA/255; vertex output
 * floor(saturate(color*tint)*255+0.5)/255; UV=float2 unmodified; pixel output
 * saturate(sample(texture0,UV)*interpolatedColor). Parent pass owns blend,
 * alpha/depth tests, sampler and tint (original initialization white).
 * Kind2 first multiplies sample by unpacked constant_argb, saturates, then
 * multiplies by interpolatedColor and saturates again (including alpha).
 * UCMAT006 carries per-draw cull/address-U/V overrides from exact known setup
 * templates. INHERIT means the parent pass's setting, not the previous draw's
 * override. Other sampler fields remain inherited. Original scripts restore
 * clockwise culling or U/V-wrap after these draws; native per-draw state must not
 * leak overrides. Classifier admits flags0/2 and exact static flags8/script02
 * for ordinary base entry only. Flags8 temporarily writes RGBA then restores
 * RGB through the same color_write field used by the skinned family;
 * separate lighting/emission paths and auxiliary resource consumers are absent.
 * Constant conversion is qualified for default nearest-even MXCSR. */
/* Exact alpha foliage, fire escapes, glass and fences additionally
 * quantize((color*raw119)*tint), preserving
 * both multiplication steps. Caller supplies raw119: RGB reciprocal exposure,
 * alpha1; original uses white when exposure manager is absent. depth_bias1 maps
 * to slope=-0.25,units=-1 for the separately qualified D24 filled-triangle path.
 * ALPHA does not imply alpha-test/discard or a complete original draw ordering;
 * no depth-write or parent light framebuffer behavior is encoded here. */
/* Static additive Window_glow1 uses the same exposure formula, ordinary kind1
 * pixel formula and depth_bias14 (slope=-3.5,units=-14). Dynamic IFL excluded. */
/* Exact skinned27/26 programs use SKIN3 geometry, the same tint/raw125 color
 * quantization and UV0 contract. Caller owns palette, object placement, tint,
 * projection, and parent state. ALPHA_VERTEX replaces only output alpha with
 * interpolated vertex alpha. WRITE_RGBA overrides the parent write mask;
 * INHERIT requires the explicit parent mask, never the previous draw's override.
 * Original flag8 temporarily writes RGBA then restores RGB. No appearance
 * policy, auxiliary lighting pass or complete character presentation is implied.
 * Exact muzzle27 additionally uses raw119 exposure before tint with two-sided
 * SRC_ALPHA/ONE or SRC_ALPHA/INV_SRC_ALPHA, inherited alpha/depth writes and
 * direct texture alpha. It does not imply a muzzle controller or depth range.
 * UCMAT007 retains 48-byte records: byte11 bits0/1/2 encode geometry/alpha/write,
 * bit3 enables exact skinned27 period1 IFL selection. texture_name then names
 * the IFL resource; all frames must resolve/validate before rendering. Caller
 * must supply renderer phase and optional per-instance integer/scalar state;
 * no timer or frame0 fallback. Frozen UCMAT006 catalogs remain accepted. */
UCMaterialStatus uc_material_catalog_decode(const void *data,size_t size,UCNativeMaterialCatalog *out);
void uc_material_catalog_free(UCNativeMaterialCatalog *catalog);
const char *uc_material_unsupported_reason(uint32_t reason);
#ifdef __cplusplus
}
#endif
#endif
