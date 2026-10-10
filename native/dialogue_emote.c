#include "dialogue_emote.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "dialogue_emote_cooldowns.h"
int32_t uc_dialogue_mission_ordinal(const char*p){int32_t value=p?atoi(p):0;switch(value){case 12:return 0;case 22:return 1;case 21:return 2;case 23:return 3;case 24:return 6;case 26:return 11;case 25:return 12;default:return value;}}
double uc_dialogue_emote_distance(const float*m,const float*p){
 if(!p)return 100000.;float x=m[8]+m[12],y=m[9]+m[13],z=m[10]+m[14];
 double dx=(double)p[0]-x,dy=(double)p[1]-y,dz=(double)p[2]-z;return sqrt((dz*dz+dy*dy)+dx*dx);
}
int uc_dialogue_emote_pair(UCGameRandom*r,int32_t ordinal,int32_t first,int32_t second,float chance,int32_t*out){
 int32_t word;uint8_t choose=ordinal>=6;
 if(!r||!out||!isfinite(chance))return 1;
 if(ordinal>1&&ordinal<6){if(uc_game_random_next(r,&word))return 1;choose=(double)(ordinal-1)*(double).2f>(double)word*0x1p-31;}
 if(choose){if(uc_game_random_next(r,&word))return 1;float value=(float)word;value*=0x1p-31f;choose=chance>value;}
 *out=choose?second:first;return 0;
}
int uc_dialogue_emote_release(UCDialogueEmoteSession*g,UCDialogueEmoteState*s,uintptr_t identity){
 if(!g||!s||s->mode>3||!identity)return 1;
 for(unsigned i=0;i<3;i++)if(g->speakers[i]==identity){g->speakers[i]=0;g->counts[s->mode!=0]--;g->total--;s->active=0;break;}return 0;
}
int uc_dialogue_emote_advance(UCDialogueEmoteSession*g,float delta){
 if(!g||!isfinite(delta)||delta<0)return 1;
 for(unsigned i=0;i<230;i++)if(g->cooldowns[i]>0){g->cooldowns[i]-=delta;if(g->cooldowns[i]<0)g->cooldowns[i]=0;}return 0;
}
int uc_dialogue_emote_select(UCDialogueEmoteSession*g,UCDialogueEmoteState*s,uintptr_t identity,const char*prefix,int32_t code,int32_t priority,const UCDialogueEmoteServices*h){
 unsigned category,line,index;uint32_t flags=0;int found;
 if(!g||!s||!h||!identity||s->mode>3||!prefix||!h->distance||!h->actor_flags||!h->busy||!h->stop||!h->play||!h->text)return -1;
 if((s->locked&&priority<100)||!code||!s->has_bank)return 0;
 category=s->mode!=0;line=code<0?(category?(uint32_t)code&255:((uint32_t)code>>8)&255):(uint32_t)code;
 if(!line)return 0;if(line>=115)return -1;
 if(priority<=25&&(g->counts[category]>=1||s->cooldown>0))return 0;
 if(g->counts[category]>=2)return 0;index=category*115+line;if(g->cooldowns[index]>0)return 0;
 if(h->distance(h->owner)>2500.)return 0;
 found=h->actor_flags(h->owner,&flags);if(found<0)return -1;if(priority<51&&found&&(flags&0x32257))return 0;
 if(h->busy(h->owner)){if(priority<=s->priority)return 0;h->stop(h->owner,s->active);}
 for(unsigned i=0;i<3;i++)if(g->speakers[i]==identity){g->speakers[i]=0;g->counts[category]--;g->total--;break;}
 if(g->total>=3)return 0;
 g->counts[category]++;g->total++;s->active=1;g->cooldowns[index]=uc_dialogue_emote_cooldowns[index];
 for(unsigned i=0;i<3;i++)if(!g->speakers[i]){g->speakers[i]=identity;g->lines[i]=(int32_t)line;break;}
 s->priority=priority;h->play(h->owner,(int32_t)line);
 if(!s->mode)return 1;if(strlen(prefix)<=1)return 0;return h->text(h->owner,(int32_t)line,(uint8_t)(found!=0));
}
