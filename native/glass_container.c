#include "glass_container.h"
#include <math.h>
#include <stdlib.h>
#include <xmmintrin.h>
int uc_glass_container_create(const UCGlassContainerInput*i,UCMaterialResources*(*current)(void*),void*context,UCGlassContainer**out){
 if(!i||!current||!out||!i->registry||!i->air_pool||!i->ground_pool||!i->vectors||!i->random||!i->uv||!i->world||!i->quad||!i->pane_shader||!i->shard_shader||!isfinite(i->floor)||(_mm_getcsr()&0xE040u))return 1;
 for(unsigned n=0;n<16;n++)if(!isfinite(i->world[n]))return 1;for(unsigned n=0;n<8;n++)if(!isfinite(i->quad[n/2][n%2]))return 1;
 if(i->vectors->first>=28||i->vectors->second>=28||i->random->first>=250||i->random->second>=250)return 1;
 for(unsigned n=0;n<112;n++)if(!isfinite(i->vectors->values[n])||i->vectors->values[n]<0||i->vectors->values[n]>4000000)return 1;
 if(i->uv->initialized&1)for(unsigned n=0;n<8;n++)if(!isfinite(i->uv->uv[n/2][n%2]))return 1;
 float floor=i->floor+2.f;if(!isfinite(floor))return 1;
 *out=NULL;UCGlassContainer*c=calloc(1,sizeof*c);if(!c)return 2;c->registry=i->registry;c->random=i->vectors;
 c->ground=uc_glass_ground_create(i->ground_pool,i->vectors,floor);if(!c->ground){free(c);return 2;}
 c->air=malloc(sizeof*c->air);if(!c->air||uc_glass_store_init(c->air,i->air_pool,i->world,floor,&c->ground->service)){free(c->air);uc_glass_ground_destroy(c->ground);free(c);return 2;}
 c->pane=uc_glass_pane_create(i->uv,i->world,i->quad,c->air,i->vectors,i->random);if(!c->pane){uc_glass_store_destroy(c->air);free(c->air);free(c);return 2;}
 *out=c;const void*sources[]={c->ground,c->air,c->pane};for(unsigned n=0;n<3;n++)if(uc_glass_registry_add(c->registry,sources[n],current(context),n==2?i->pane_shader:i->shard_shader))return 2;return 0;
}
uint32_t uc_glass_container_state(const UCGlassContainer*c){if(!c||!c->pane)return 0;UCGlassBullet b=uc_glass_pane_bullet(c->pane);return *b.triggered?2:0;}
int uc_glass_container_update(UCGlassContainer*c,float delta){
 if(!c||!isfinite(delta)||delta<0||delta>1)return 1;if(delta==0)return 0;
 if(!uc_glass_store_valid(c->air)||!uc_glass_ground_valid(c->ground))return 1;
 if(uc_glass_store_update(c->air,c->random,delta)||uc_glass_ground_update(c->ground,delta))return 2;return 0;
}
int uc_glass_container_destroy(UCGlassContainer*c){
 if(!c||!c->registry||!c->pane||!uc_glass_store_valid(c->air)||!uc_glass_ground_valid(c->ground))return 1;
 if(uc_glass_registry_remove(c->registry,c->ground))return 1;
 if(uc_glass_registry_remove(c->registry,c->air)||uc_glass_registry_remove(c->registry,c->pane)||uc_glass_pane_destroy(c->pane))return 2;free(c);return 0;
}
