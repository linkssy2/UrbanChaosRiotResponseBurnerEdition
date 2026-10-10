#include "scene_view_publish.h"
int uc_scene_view_publish(const UCSceneView*v,UCSceneMatrixProvider*(*get)(void*),void*owner,const UCSceneMatrixServices*h){
 static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 if(!v||!get||!h||!h->context||!h->upload)return 1;
 UCSceneMatrixProvider*p=get(owner);if(!p)return 2;
 return uc_scene_matrix_set(p,2,identity)||uc_scene_matrix_set(p,0,v->world)||
 uc_scene_matrix_set(p,1,v->inverse)||uc_scene_matrix_set(p,3,v->projection)||
 uc_scene_matrix_set(p,4,v->alternate)||uc_scene_matrix_flush(p,h)?2:0;
}
