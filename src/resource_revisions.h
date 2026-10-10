#ifndef UC_RESOURCE_REVISIONS_H
#define UC_RESOURCE_REVISIONS_H
#include <stdint.h>
#include <stddef.h>
/* Renderer-owner registration/arming. IDs remain stable until shutdown; zero
 * means unsupported and requires exact byte comparison. IDs are opaque: opted-in
 * lighting borrowers use tagged owner IDs, not ordinary region-array indexes.
 * Native hooks may run
 * on other threads. Revisions cover every mapped physical alias. */
unsigned uc_resource_register(size_t offset,size_t bytes);
/* Read-only ownership lookup: never reserves slots or changes protections. */
unsigned uc_resource_find_existing(size_t offset,size_t bytes);
typedef enum UcResourceRegistrationStatus {
    UC_RESOURCE_UNATTEMPTED=0, UC_RESOURCE_REGISTERED, UC_RESOURCE_NATIVE_BUSY,
    UC_RESOURCE_DISABLED, UC_RESOURCE_FAILED, UC_RESOURCE_INVALID_RANGE,
    UC_RESOURCE_TOO_LARGE, UC_RESOURCE_PARTIAL_OVERLAP, UC_RESOURCE_CAPACITY,
    UC_RESOURCE_LIGHTING_OVERLAP, UC_RESOURCE_UNSUPPORTED_MAPPING,
    UC_RESOURCE_HANDLER_FAILED
} UcResourceRegistrationStatus;
typedef struct UcResourceRegistration {
    unsigned id;
    UcResourceRegistrationStatus status;
    uint64_t native_epoch;
} UcResourceRegistration;
UcResourceRegistration uc_resource_register_result(size_t offset,size_t bytes);
/* Nonzero only outside native-output intervals and while tracking is usable.
 * Changes at native begin/end. Equality checks may bracket a bounded byte scan
 * with this epoch; it does not serialize later writes or guest execution. */
uint64_t uc_resource_native_epoch(void);
uint64_t uc_resource_arm(unsigned id);
int uc_resource_clean(unsigned id,uint64_t revision);
int uc_resource_overlaps(size_t offset,size_t bytes);
int uc_resource_native_begin(void*buffer,size_t bytes);
void uc_resource_native_end(int entered);
int uc_resource_disable(void);
int uc_resource_shutdown(void);
#endif
