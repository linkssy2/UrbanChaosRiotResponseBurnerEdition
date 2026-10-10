#include "explosion_children.h"
#include <math.h>
#include <string.h>
int uc_world_warp_asset(const UCExplosionVisualChild*r,UCParticleAsset*out){
 UCParticleAsset a;unsigned i;
 if(!r||!out||r->kind!=UC_EXPLOSION_WARP_RING||!isfinite(r->start_size)||!isfinite(r->end_size)||!isfinite(r->lifetime)||!isfinite(r->strength))return 1;
 uc_particle_asset_reset(&a);a.emitter_type=1;a.burst=1;a.rate=a.emitter_lifetime=0;
 a.simulation.shape=1;a.simulation.angle_max=a.simulation.force_min=a.simulation.force_max=0;
 for(i=0;i<4;i++){a.color[0][i]=a.color[2][i]=0;a.color[1][i]=r->strength;}
 a.hold[0]=0;a.hold[1]=.75f;a.simulation.start_size=r->start_size;a.simulation.end_size=r->end_size;a.simulation.lifetime=r->lifetime;
 a.render_type=1;memcpy(a.texture,"Particles\\Shapes\\Ring.tga",sizeof "Particles\\Shapes\\Ring.tga");*out=a;return 0;
}
int uc_explosion_camera_input(const UCExplosionVisualChild*r,const float*w,UCCameraPushInput*out){
 UCCameraPushInput p;
 if(!r||!w||!out||(r->kind!=UC_EXPLOSION_CAMERA_PUSH&&r->kind!=UC_EXPLOSION_CAMERA_KICK))return 1;
 memcpy(p.position,w+12,12);p.force=r->force;p.lifetime=r->lifetime;p.fade_in=r->fade_in;p.fade_out=r->fade_out;p.minimum_range=r->minimum_range;p.maximum_range=r->maximum_range;
 for(unsigned i=0;i<sizeof p/sizeof(float);i++)if(!isfinite(((float*)&p)[i]))return 1;
 *out=p;return 0;
}
int uc_world_warp_step(float*remaining,float delta,void*owner,int(*remove)(void*)){
 if(!remaining||!isfinite(*remaining)||!isfinite(delta)||!remove)return 1;
 if(*remaining<0&&remove(owner))return 2;
 *remaining-=delta;return 0;
}
