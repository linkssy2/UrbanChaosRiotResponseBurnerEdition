#pragma once
#include "entity_stream.h"
/* Full1E2790's eight immutable serialized properties in their read order.
 * Boolean reads preserve the original low-byte truth rule. This is the field
 * projection only; base/coroutine/body/model construction is not supplied. */
typedef struct UCBasicPropParameters {
    uint32_t body;
    float model_value;
    uint32_t visible_thermal,visible_normal;
    float rotation_z,rotation_y,rotation_x;
    uint32_t difficulty_variant;
} UCBasicPropParameters;
/* Returns1 without cursor/output mutation if the actual stream is incomplete.
 * Descriptor and property storage must remain immutable for this read. */
int uc_basic_prop_parameters(const UCLevelEntity *,const uint8_t **cursor,UCBasicPropParameters *);
