#ifndef UC_MODEL_MATERIAL_H
#define UC_MODEL_MATERIAL_H
#include "material_prepass.h"
#include <string.h>
static int uc_model_material_literal(const uint8_t*p,uint8_t op,uint32_t value){
 if(p[0]!=op)return 0;for(unsigned i=0;i<4;i++){uint32_t x;memcpy(&x,p+1+i*4,4);if(x!=value)return 0;}return 1;
}
/* The native texture owner already selects the exact IFL frame before any
 * draw. Original8FF30 executes the same53/87-byte selection as8FD80, but its
 * untextured ambient shader consumes no selected image. No mutable IFL index
 * exists in this native owner; base draw borrows the same immutable selection. */
static int uc_model_material_ifl(UCMaterialSpan s){
 if(!s.data)return 0;
 if(s.size==87&&s.data[0]==0xC1&&uc_model_material_literal(s.data+17,0x86,0)){s.data+=34;s.size-=34;}
 return s.size==53&&uc_model_material_literal(s.data,0xC1,0x3F800000)&&uc_model_material_literal(s.data+17,0xC1,0)&&s.data[34]==0x4A&&uc_model_material_literal(s.data+35,0x87,0)&&s.data[52]==2;
}
static int uc_model_material_prepass(const UCMaterialAsset*asset,int resolved_sequence,UCMaterialPrepass*out){
 UCMaterialAsset m=*asset;m.effective_flags&=~(uint16_t)12;
 if(uc_material_prepass_prepare(&m,out))return 1;
 if(!resolved_sequence||!uc_model_material_ifl(m.setup_program[0]))return 0;
 const uint8_t draw=2;m.setup_program[0].data=&draw;m.setup_program[0].size=1;return uc_material_prepass_prepare(&m,out);
}
/* Original8FF30/905B0 nonspecular masked skinned material, no ordinary pass.
 * Exactly the authored hair recipe; the existing native sequence owns header8. */
static int uc_model_material_masked(const UCMaterialAsset*m){
 static const uint8_t header[14]={0,0,69,0,0x25,0,0,1,0,255,0,0,0,0};
 return m&&m->source_flags==0x25&&m->effective_flags==0x25&&m->dependency_count==1&&m->dependencies&&m->dependencies[0].kind&&
  !m->pass_count&&!m->setup_program[0].size&&m->setup_program[1].size==69&&!memcmp(m->header,header,14);
}
#endif
