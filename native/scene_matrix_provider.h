#ifndef UC_SCENE_MATRIX_PROVIDER_H
#define UC_SCENE_MATRIX_PROVIDER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneMatrixProvider {
    float camera[16],view[16],material[16],projection[16],alternate[16],composite[16];
    uint8_t dirty[4];
} UCSceneMatrixProvider;
typedef struct UCSceneMatrixServices {
    void *owner;
    void (*context)(void *);
    void (*upload)(void *,uint32_t first,const float *,uint32_t floats);
} UCSceneMatrixServices;
/* A25E0 clears only four dirty bytes; matrix contents are not initialized.
 * Slot0 camera(ADEF0),1 view(ADF20),2 material(ADEB0),3 projection(ADF50),
 *4 alternate(A2610). The first four setters mark their corresponding byte1;
 * alternate does not dirty the composite. Setter input must be disjoint. */
int uc_scene_matrix_initialize(UCSceneMatrixProvider *);
int uc_scene_matrix_set(UCSceneMatrixProvider *,uint32_t slot,const float [16]);
/* Whole A2670: always call the current graphics+A4/current+1C service first;
 * when any dirty byte is nonzero, publish (material*view)*projection, ordered
 * uploads96/118/100/117/104, then clear all four bytes. Exact float pair order,
 * source transpose and existing2D4360 RCPSS inverse are retained. Finite RN
 * normal-denormal inputs/results, nonsingular material when inverse is used.
 * Native preflights arithmetic before effects. Required context/upload services
 * complete, may affect their own real owners, but cannot mutate/reenter/free
 * this provider or change FP mode; FP sticky flags are outside the contract.
 * Upload borrows temporary storage only through the call. This is not a device
 * constant owner or frame presenter.0 complete,1 invalid before any effects. */
int uc_scene_matrix_flush(UCSceneMatrixProvider *,const UCSceneMatrixServices *);
#ifdef __cplusplus
}
#endif
#endif
