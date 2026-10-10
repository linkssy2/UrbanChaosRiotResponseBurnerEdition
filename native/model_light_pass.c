#include "model_light_pass.h"
#include <math.h>
#include <string.h>
static int eligible(const UCModelBatch *b,uint8_t indexed,uint16_t light){return !indexed||(b->light_index==light&&b->flags[1]!=0);}
int uc_model_light_pass(const UCModel *m,uint8_t indexed,uint16_t light,uint8_t partition,
    const float (*rows)[12],const uint8_t *valid,UCModelRenderPalette *palette,const UCModelLightPassHooks *h){
    if(!m||!palette||!h||!h->upload_palette||!h->material_flags||!h->draw||indexed>1||partition>1||
       m->batch_count>100000||m->bone_count>4096||m->palette_update_count>100000||
       (m->batch_count&&!m->batches)||(m->palette_update_count&&!m->palette_updates))return 0;
    for(uint32_t i=0;i<m->batch_count;i++){
        const UCModelBatch *b=m->batches+i;if(!eligible(b,indexed,light))continue;
        if(b->material_index>=m->material_count||b->first_palette_update>m->palette_update_count||
           b->palette_update_count>m->palette_update_count-b->first_palette_update)return 0;
        for(uint32_t j=0;j<b->palette_update_count;j++){
            const UCModelPaletteUpdate *p=m->palette_updates+b->first_palette_update+j;
            if(!rows||!valid||p->bone>=m->bone_count||!valid[p->bone]||p->row_offset< -96||p->row_offset>93)return 0;
            for(unsigned k=0;k<12;k++)if(!isfinite(rows[p->bone][k]))return 0;
        }
    }
    for(uint32_t i=0;i<m->batch_count;i++){
        const UCModelBatch *b=m->batches+i;if(!eligible(b,indexed,light))continue;
        for(uint32_t j=0;j<b->palette_update_count;j++){
            const UCModelPaletteUpdate *p=m->palette_updates+b->first_palette_update+j;unsigned row=(unsigned)(96+p->row_offset);
            memcpy(palette->rows+row,rows[p->bone],48);memset(palette->valid+row,1,3);
            h->upload_palette(h->context,row,rows[p->bone]);
        }
        if((h->material_flags(h->context,b->material_index)&1u)==partition)h->draw(h->context,i,b,palette);
    }
    return 1;
}
