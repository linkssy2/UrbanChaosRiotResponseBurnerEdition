#include "particle_render_state.h"
static int valid(const UCParticleRenderState *s){
    return s&&s->blend_enabled<=1&&s->alpha_test<=1&&s->source_factor<=5&&
        s->destination_factor<=5&&s->operation<=1&&s->cull_winding<=2&&
        s->alpha_comparison<=7&&s->alpha_reference<=255&&
        s->texture_alpha_kill<=1&&s->fog_enabled<=1;
}
int uc_particle_render_begin(UCParticleRenderState *s,uint32_t mode){
    if(!valid(s))return 0;
    if(mode==0){s->blend_enabled=0;s->alpha_test=0;}
    else if(mode==3)s->blend_enabled=0;
    else if(mode<=6){
        s->blend_enabled=1;s->alpha_test=mode==2||mode==4||mode==6;
        s->source_factor=mode==1?UC_PARTICLE_SOURCE_COLOR:
            mode==5?UC_PARTICLE_INVERSE_SOURCE_COLOR:UC_PARTICLE_SOURCE_ALPHA;
        s->destination_factor=mode==2?UC_PARTICLE_INVERSE_SOURCE_ALPHA:UC_PARTICLE_ONE;
        s->operation=mode>=5?UC_PARTICLE_REVERSE_SUBTRACT:UC_PARTICLE_ADD;
    }
    s->cull_winding=UC_PARTICLE_CULL_NONE;s->alpha_comparison=6;
    s->alpha_reference=1;s->texture_alpha_kill=1;return 1;
}
int uc_particle_render_end(UCParticleRenderState *s){
    if(!valid(s))return 0;
    s->blend_enabled=s->alpha_test=s->texture_alpha_kill=s->fog_enabled=0;
    s->source_factor=UC_PARTICLE_ONE;s->destination_factor=UC_PARTICLE_ZERO;
    s->operation=UC_PARTICLE_ADD;s->cull_winding=UC_PARTICLE_CULL_CLOCKWISE;return 1;
}
