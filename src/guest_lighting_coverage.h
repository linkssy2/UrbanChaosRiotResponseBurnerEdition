#ifndef UCRR_GUEST_LIGHTING_COVERAGE_H
#define UCRR_GUEST_LIGHTING_COVERAGE_H
#include <stdint.h>
/* Bounded observation only. No cache admission or rendering decisions. */
void uc_guest_light_coverage_boundary(int start,uint64_t qpc);
void uc_guest_light_traversal_enter(void);
void uc_guest_light_traversal_leave(void);
int uc_guest_light_in_traversal(void);
void uc_guest_light_palette_enter(uint32_t stack);
void uc_guest_light_palette_upload(uint32_t stack,uint32_t source);
void uc_guest_light_material_enter(void);
void uc_guest_light_projected(uint32_t matrix,uint32_t projection);
void uc_guest_light_selection_node(void);
void uc_guest_light_selection(uint32_t receiver,uint32_t stack_after,uint32_t result);
#endif
