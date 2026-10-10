#include "character_damage.h"
#include <math.h>
#include <string.h>
int uc_character_injury_read(const UCCharacterInjury*s,uint32_t*flags,float*damage){
 if(!s||!flags||!damage)return 1;
 static const uint32_t priority[]={0x2000,0x1000,0x80,2,0x40,4,0x10,8,0x20,0x4000};
 *damage=s->damage;*flags|=s->reactions;
 for(unsigned i=0;i<10;i++)if((s->reactions&priority[i])==priority[i]){*flags=(*flags&0xFFFE0F01u)|priority[i];break;}return 0;
}
int uc_character_hit_direction(const float*w,const float*v,uint8_t force,uint32_t*out){
 if(!w||!v||!out||force>1)return 1;
 for(unsigned i=0;i<3;i++)if(!isfinite(v[i]))return 1;
 if(!isfinite(w[8])||!isfinite(w[10]))return 1;
 float square=(v[2]*v[2]+v[1]*v[1])+v[0]*v[0];if(!(square>0)){*out=0x100;return 0;}
 double fz=w[10],fx=w[8];double length=sqrt(fz*fz+fx*fx);if(!(length>0)||!isfinite(length)||!isfinite(square))return 1;
 float inv=(float)(1.0/sqrt((double)square));float x=v[0]*inv,y=v[1]*inv,z=v[2]*inv;
 inv=(float)(1.0/length);float zx=inv*w[8],zz=inv*w[10],zy=inv*0.f;
 float forward=(z*zz+y*zy)+zx*x;
 float negative_z=0.f-w[10];double side_z=negative_z;
 inv=(float)(1.0/sqrt(fx*fx+side_z*side_z));float side=(z*(inv*w[8])+y*(inv*0.f))+(inv*negative_z)*x;
 *out=(fabs((double)forward)>fabs((double)side)||force)?(0.f>forward?0x100:0x200):(0.f>side?0x400:0x800);return 0;
}
int uc_character_injure(const UCCharacterInjuryServices*h,uint32_t flags,float damage,
 const float point[3],int32_t kind,int32_t owner,uint8_t check,uint8_t*accepted){
 if(!h||!h->root_id||!h->enemy||!h->shield||!h->resolve||!point||!accepted||!isfinite(damage))return 1;
 int32_t root;uint8_t value;
 if(h->root_id(h->owner,&root))return 2;
 if(root==-1){*accepted=0;return 0;}
 if(check&&!(flags&0x2000)){
  if(h->enemy(h->owner,root,&value))return 2;
  if(!value){if(h->shield(h->owner,point,&value))return 2;if(value){*accepted=0;return 0;}}
 }
 if(h->root_id(h->owner,&root))return 2;
 UCCharacterInjury*r=h->resolve(h->owner,root);if(!r)return 2;
 r->reactions|=flags;r->damage=damage+r->damage;r->kind=kind;r->owner=owner;
 *accepted=1;return 0;
}
int uc_character_restrain(uint32_t*status,int32_t*cause,uint8_t floor,uint8_t player,const UCCharacterRestraintServices*h){
 if(!status||!cause||!h||!h->create||!h->activate||floor>1||player>1)return 1;
 if(*status&5)return 0;int32_t id;
 if(h->create(h->owner,floor,&id)||h->activate(h->owner,id))return 2;
 if(player)*cause=2;*status|=4;return 0;
}
void uc_character_blast_initialize(UCCharacterBlastResponse*s){
 if(!s)return;
 const float defaults[9]={1200,900,500,350,.6f,.4f,1,1.5f,20};
 memcpy(s->impulse,defaults,sizeof defaults);
 memset(s->impulse+12,0,16);memset(s->impulse+16,0,16);s->impulse[16]=1;
 memset(s->impulse+20,0,16);s->impulse[24]=0;
 /*E3252 initializes actor19C separately from DF110.*/
 s->direction[0]=s->direction[1]=0;s->direction[2]=1;
}
int uc_character_blast(UCCharacterBlastResponse*s,const UCExplosionBlast*b,int32_t sender,
 const UCCharacterBlastServices*h){
 if(!s||!b||!h||!h->position||!h->eligible||!h->player||!h->root||!h->order||!h->property||!h->notify||!h->injure||!h->root_blast)return 1;
 const float*p=h->position;int32_t kind=b->type==2?7:5,player,root;uint32_t flags;uint8_t yes,found;
 if(b->blast){
  if(h->eligible(h->owner,b,&yes))return 2;
  if(yes){float d[3];for(unsigned j=0;j<3;j++)d[j]=b->point[j]-p[j];float square=(d[0]*d[0]+d[1]*d[1])+d[2]*d[2];
   if(500.0>sqrt((double)square)){
    float direction[3]={p[0]-b->point[0],0,p[2]-b->point[2]};square=(direction[0]*direction[0]+direction[1]*direction[1])+direction[2]*direction[2];
    if(!isfinite(square))return 2;double reciprocal=square>0?1.0/sqrt((double)square):0;
    for(unsigned j=0;j<3;j++)s->direction[j]=(float)(reciprocal*direction[j]);
    float v[4]={p[0]*.01f,p[2]*.01f,p[1]*.01f,0};v[2]+=1.2f;memcpy(s->impulse+12,v,16);
    const float center[4]={b->point[0]*.01f,b->point[2]*.01f,b->point[1]*.01f,0};
    for(unsigned j=0;j<4;j++)s->impulse[16+j]=v[j]-center[j];
    s->impulse[24]=0;s->impulse[8]=1000;s->impulse[2]=s->impulse[3]=s->impulse[5]=s->impulse[0]=s->impulse[1]=s->impulse[4]=0;s->impulse[7]=1.5f;s->impulse[6]=1.9f;
    float amount=b->damage;if(h->player(h->owner,&player))return 2;
    if(player==b->owner){
     if(h->injure(h->owner,0x40000,0,b->point,kind,b->owner,1))return 2;
     if(h->controller&&b->type==3){int32_t order;if(h->order(h->owner,&order))return 2;if(order!=-1)amount*=4.f;}
    }
    if(h->injure(h->owner,0x1000,amount,b->point,kind,b->owner,1))return 2;
   }
  }
 }
 if(b->type==2){
  if(h->eligible(h->owner,b,&yes))return 2;
  if(yes){
   if(!h->controller||h->root(h->owner,&root,&flags,&found))return 2;
   if(!found||!(flags&4)){
    float amount=0;if(h->player(h->owner,&player))return 2;
    if(player==b->owner){
     if(h->property(h->owner,"enemy of player",&yes))return 2;
     if(yes){if(h->property(h->owner,"hostage taker",&yes))return 2;
      if(!yes){if(h->notify(h->owner,h->id,0,1))return 2;}else amount=b->damage;
     }else amount=b->damage;
    }
    if(h->injure(h->owner,0x10000,amount,b->point,kind,b->owner,1))return 2;
   }
  }
 }
 if(h->controller){if(h->root(h->owner,&root,&flags,&found))return 2;if(found&&h->root_blast(h->owner,b,sender))return 2;}
 return 0;
}
static int blast_position(const float*p){return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static double blast_notice_ratio(const float*a,const float*b,float radius){
 double x=(double)a[0]-b[0],y=(double)a[1]-b[1],z=(double)a[2]-b[2];
 return sqrt((z*z+y*y)+x*x)/((double)radius+radius);
}
int uc_character_blast_notice(UCCharacterBlastNotice*s,const float*a,const float*p,float radius,int32_t sender,float duration){
 if(!s||!blast_position(a)||!blast_position(p)||!isfinite(radius)||radius<=0||!isfinite(duration)||duration<0)return 1;
 double ratio=blast_notice_ratio(a,p,radius);if(!isfinite(ratio))return 1;
 if(1.5>ratio){memcpy(s->point,p,12);s->radius=radius;s->sender=sender;s->remaining=duration;s->ratio=(float)ratio;}return 0;
}
int uc_character_blast_notice_step(UCCharacterBlastNotice*s,const float*a,const float*p,double delta){
 if(!s||!blast_position(a)||!blast_position(s->point)||(p&&!blast_position(p))||!isfinite(s->remaining)||!isfinite(delta)||delta<0)return 1;
 if(s->remaining<=0)return 0;if(!isfinite(s->radius)||s->radius<=0)return 1;
 if(p)memcpy(s->point,p,12);double ratio=blast_notice_ratio(a,s->point,s->radius);if(!isfinite(ratio))return 1;
 s->remaining=ratio>1.5?0:(float)((double)s->remaining-delta);return 0;
}
int uc_character_bullet(UCCharacterBlastResponse*s,const UCCharacterBullet*b,const UCCharacterBulletImpact*p,const UCCharacterBulletServices*h){
 if(!s||!b||!p||!h||!h->world)return 1;
 if(p->kind!=2)return 0;
 uint32_t flags=0;int32_t owner=b->owner,index=-1;uint8_t yes,found;
 if(b->kind==11){
  if(!h->effect||!h->health||h->effect(h->owner,h->id,"Bip01 Pelvis","AM-body-burn.simpleparticlesystem")||h->health(h->owner,-1,4))return 2;
 }else{
  switch(p->material){case 9:flags=0x80;break;case 14:flags=4;break;case 15:flags=0x10;break;case 16:flags=8;break;case 17:flags=0x20;break;default:flags=0x40;break;}
  int large=b->item==4||b->item==17||b->item==18||b->item==19;
  if(large)flags|=0x200000;
  yes=(uint8_t)(large||b->item==6||b->item==23);
  if(!yes){if(!h->heavy_weapon||h->heavy_weapon(h->owner,b->item,&yes))return 2;
   if(!yes){if(!h->boolean||h->boolean(h->owner,owner,"is sniping",0,&yes))return 2;}}
  if(yes)flags|=0x2000000;
 }
 flags|=0x20000;if(b->kind==6)flags|=0x2000;else if(b->kind==10)flags|=0x1000000;
 if(!h->resolve||h->resolve(h->owner,p->source,&found))return 2;
 if(found){
  if(!h->player_index||h->player_index(h->owner,p->source,&index))return 2;
  if(index!=-1)flags|=0x40000;else flags&=~0x40000u;
  int32_t shooter;
  if(!h->source_owner||h->source_owner(h->owner,p->source,&shooter,&found))return 2;
  if(found&&shooter!=-1){
   yes=0;if(h->faction){if(!h->friendly||h->friendly(h->owner,shooter,&yes))return 2;}
   if(yes){if(index==-1)flags|=0x80000;}
   else if(h->controller){if(!h->target||h->target(h->owner,shooter))return 2;}
  }
 }else flags&=~0x40000u;
 memcpy(s->direction,p->momentum,12);s->direction[1]=0;
 float square=(s->direction[0]*s->direction[0]+s->direction[1]*s->direction[1])+s->direction[2]*s->direction[2];
 if(!isfinite(square))return 2;
 if(square>0.0001f){double inverse=1.0/sqrt((double)square);for(unsigned j=0;j<3;j++)s->direction[j]=(float)(inverse*s->direction[j]);}
 s->impulse[12]=p->point[0]*.01f;s->impulse[13]=p->point[2]*.01f;s->impulse[14]=p->point[1]*.01f;s->impulse[15]=0;
 s->impulse[16]=p->momentum[0]*.01f;s->impulse[17]=p->momentum[2]*.01f;s->impulse[18]=p->momentum[1]*.01f;s->impulse[19]=0;
 s->impulse[24]=p->distance*.01f;memcpy(s->impulse,p->parameters,36);
 uint32_t direction;if(uc_character_hit_direction(h->world,s->direction,0,&direction))return 2;flags|=direction;
 if(!h->profile||h->profile(h->owner,0x185,&yes))return 2;
 if(yes&&(flags&0x40000)){
  if(!h->boolean||h->boolean(h->owner,h->root_id,"enemy of player",1,&yes))return 2;
  if(yes){if(h->boolean(h->owner,h->id,"burnable",1,&yes))return 2;
   if(yes){if(!h->burn||h->burn(h->owner,&owner))return 2;
    if(!h->injure||h->injure(h->owner,flags,h->damage,b->position,b->kind,owner,1,&yes))return 2;return 0;}}
 }
 if(!h->injure||h->injure(h->owner,flags,h->damage,b->position,b->kind,owner,1,&yes))return 2;
 if(!yes)return 0;
 if(h->profile(h->owner,0x32,&yes))return 2;if(!yes||b->kind==6)return 0;
 square=(p->momentum[0]*p->momentum[0]+p->momentum[2]*p->momentum[2])+p->momentum[1]*p->momentum[1];
 if(!(square>0)||!isfinite(square))return 2;
 float inverse=(float)(1.0/sqrt((double)square)),end[3];for(unsigned j=0;j<3;j++)end[j]=p->point[j]+(p->momentum[j]*inverse)*600.f;
 UCCharacterBloodHit hit;if(!h->ray||h->ray(h->owner,p->point,end,9,h->group,0x80,0,&hit,&found))return 2;
 if(!found||hit.actor)return 0;
 static const uint8_t rejected[]={0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,1,0,0};
 if(hit.material<1||hit.material>33||rejected[hit.material-1])return 0;
 int32_t choice;if(!h->random||h->random(h->owner,0,5,&choice))return 2;
 const char*shaders[]={"FX\\FX_BloodSplats\\FX_BloodSplat_Wall_1.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_2.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_3.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_4.shader"};
 if(!h->decal||h->decal(h->owner,shaders[choice==0?0:choice==1?1:choice==2?2:3],&hit,50,0x80,2))return 2;
 return 0;
}
