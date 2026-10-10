#ifndef UC_HYBRID_MODELS_TEXTURE_H
#define UC_HYBRID_MODELS_TEXTURE_H
#include <stdint.h>
/* Original 3E3840 texture bind. A zero Size leaves pitch/rect unchanged. */
static void uc_model_texture_registers(const uint32_t header[5],uint32_t *offset,
    uint32_t *format,uint32_t *control1,uint32_t *rect){
    *offset=header[1];*format=header[3];
    const uint32_t size=header[4];
    if(size){*control1=((size>>24)+1u)<<22;
        *rect=(((size&4095u)+1u)<<16)|(((size>>12)&4095u)+1u);}
}
#endif
