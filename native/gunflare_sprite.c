#include "gunflare_sprite.h"
#include "particle_registry.h"
static int valid(const UCGunFlareSpriteSource *s){
    return s&&s->visible&&s->enabled&&s->texture&&s->position&&s->radius;
}
int uc_gunflare_sprite_construct(UCParticleRegistry *r,const UCGunFlareSpriteSource *s,uintptr_t *token){
    if(!r||!valid(s)||!token)return 1;
    *s->visible=0;*s->enabled=0;
    return uc_particle_registry_add_sprite(r,s,token);
}
int uc_gunflare_sprite_read(const UCGunFlareSpriteSource *s,UCGunFlareSpriteRead *out){
    if(!valid(s)||!out)return 1;
    out->count=*s->visible&&*s->enabled?1u:0u;out->type=4;
    out->texture=*s->texture;*s->visible=0;return 0;
}
int uc_gunflare_sprite_draw_quiet(const UCGunFlareSpriteSource *s,const UCGunFlareSpriteHooks *h,void *ctx){
    if(!valid(s)||*s->enabled||!h||!h->current_environment||!h->environment_context||!h->current_camera)return 1;
    void *owner=h->current_environment(ctx);if(!owner)return 2;
    const UCGunFlareSpriteContext *environment=h->environment_context(ctx,owner);if(!environment)return 2;
    const void *captured=environment->environment;(void)captured;
    if(!h->current_camera(ctx))return 2;
    UCGunFlareSpriteRead read;
    return uc_gunflare_sprite_read(s,&read)||read.count?2:0;
}
