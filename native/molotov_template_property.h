#ifndef UC_MOLOTOV_TEMPLATE_PROPERTY_H
#define UC_MOLOTOV_TEMPLATE_PROPERTY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMolotovTemplateProperty {
    uint32_t type;float number;const char *string;
} UCMolotovTemplateProperty;
/* Whole F8120: case-sensitive source virtual8; returns1 found or0 missing.
 * Only the source-written type/payload change. Missing leaves output intact. */
int uc_molotov_template_property(const char *,UCMolotovTemplateProperty *);
#ifdef __cplusplus
}
#endif
#endif
