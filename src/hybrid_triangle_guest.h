#pragma once
#include <stdint.h>
#include "../native/collision_mesh.h"
/*0 original fallback,1 miss,2 hit; caller retains original return/collector.*/
int uc_hybrid_triangle_cast(uint32_t shape,uint32_t ray,uint32_t hit);
/*1 transformed hit,0 original fallback; original callback remains authoritative.*/
int uc_hybrid_triangle_world_hit(uint32_t body,uint32_t hit);
/*1 child constructed,0 untouched original fallback.*/
int uc_hybrid_mesh_child(uint32_t mesh,uint32_t key,uint32_t child);
/* Ungated pure child decoder; reject every read overlapping a future output. */
int uc_hybrid_mesh_child_read_checked(uint32_t mesh,uint32_t key,uint32_t reserved,unsigned bytes,UCCollisionMeshChild*result);
/*Read-only material lookup; returns1 on success,0 untouched fallback.*/
int uc_hybrid_mesh_material(uint32_t mesh,uint32_t key,uint32_t*result);
/* Same validated read, with original volatile results; rejects reads overlapping
 * scratch before a caller publishes the original guest stack writes. */
typedef struct {uint32_t material,index,offset;} UCHybridMeshMaterial;
/* Ungated pure decoder for independently admitted native boundaries. */
int uc_hybrid_mesh_material_read_checked(uint32_t mesh,uint32_t key,uint32_t scratch,unsigned bytes,UCHybridMeshMaterial*result);
int uc_hybrid_mesh_material_read(uint32_t mesh,uint32_t key,uint32_t scratch,unsigned bytes,UCHybridMeshMaterial*result);
/* Complete original3042E0 register/stack effects, or unchanged decline. */
int uc_hybrid_mesh_material_guest(uint32_t mesh,uint32_t key,uint32_t stack);
