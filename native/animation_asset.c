#include "animation_asset.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct Reader {const uint8_t *p;size_t size,at;UCAnimationStatus status;} Reader;
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static const uint8_t *take(Reader*r,size_t n){const uint8_t*p;if(n>r->size-r->at){r->status=UC_ANIMATION_FORMAT;return NULL;}p=r->p+r->at;r->at+=n;return p;}
static void *copy(Reader*r,const void*p,size_t n){void*q=malloc(n?n:1);if(!q){r->status=UC_ANIMATION_MEMORY;return NULL;}if(n)memcpy(q,p,n);return q;}
static void *array(Reader*r,size_t n,size_t stride){void*p=calloc(n?n:1,stride);if(!p)r->status=UC_ANIMATION_MEMORY;return p;}
static char *string(Reader*r){const uint8_t*p=r->p+r->at,*end=(const uint8_t*)memchr(p,0,r->size-r->at);size_t n;if(!end){r->status=UC_ANIMATION_FORMAT;return NULL;}n=(size_t)(end-p)+1;if(n>4096){r->status=UC_ANIMATION_LIMIT;return NULL;}r->at+=n;return (char*)copy(r,p,n);}
#define NEED(x) do{if(!(x))return 0;}while(0)
#define REQUIRE(x,s) do{if(!(x)){r->status=(s);return 0;}}while(0)
static int name_block(Reader*r,char***names,uint32_t count){
 const uint8_t*p=take(r,4);Reader block;NEED(p);uint32_t n=u32(p);p=take(r,n);NEED(p);
 *names=(char**)array(r,count,sizeof(char*));NEED(*names);block=(Reader){p,n,0,UC_ANIMATION_OK};
 for(uint32_t i=0;i<count;++i){(*names)[i]=string(&block);if(!(*names)[i]){r->status=block.status;return 0;}}
 REQUIRE(block.at==block.size,UC_ANIMATION_FORMAT);return 1;
}
static int template_parse(Reader*r,UCAnimationTemplate*t){
 const uint8_t*p=take(r,4);NEED(p);REQUIRE(!memcmp(p,"SKTP",4),UC_ANIMATION_FORMAT);
 t->source_label=string(r);NEED(t->source_label);p=take(r,4);NEED(p);t->bone_count=u32(p);
 REQUIRE(t->bone_count<=4096,UC_ANIMATION_LIMIT);
 t->bone_names=(char**)array(r,t->bone_count,sizeof(char*));NEED(t->bone_names);
 for(uint32_t i=0;i<t->bone_count;++i){t->bone_names[i]=string(r);NEED(t->bone_names[i]);}
 p=take(r,t->bone_count);NEED(p);t->bone_flags=(uint8_t*)copy(r,p,t->bone_count);NEED(t->bone_flags);
 p=take(r,4);NEED(p);t->clip_count=u32(p);REQUIRE(t->clip_count<=65536,UC_ANIMATION_LIMIT);
 NEED(name_block(r,&t->clip_names,t->clip_count));NEED(name_block(r,&t->clip_aliases,t->clip_count));
 p=take(r,4);NEED(p);memcpy(&t->source_default,p,4);REQUIRE(r->at==r->size,UC_ANIMATION_UNSUPPORTED);return 1;
}
static int quaternion(const uint8_t*p,unsigned bytes,float q[4]){
 uint64_t bits=0;unsigned omitted,width=bytes==6?15:10;double sum;
 const float bias=0.7071067690849304f,scale=bytes==6?4.31583721365314e-05f:0.0013810679083690047f;
 for(unsigned i=0;i<bytes;++i)bits|=(uint64_t)p[i]<<(i*8);
 omitted=(unsigned)(bits&3);bits>>=2;
 for(unsigned i=0;i<4;++i){q[i]=0;if(i!=omitted){q[i]=(float)((double)(bits&((1u<<width)-1))*scale-bias);bits>>=width;}}
 sum=(((double)q[3]*q[3]+(double)q[2]*q[2])+(double)q[1]*q[1])+(double)q[0]*q[0];
 if(!(sum<=1.0))return 0;q[omitted]=(float)sqrt(1.0-sum);return 1;
}
UCAnimationStatus uc_animation_quaternion_decode(const void *packed,size_t bytes,float out[4]){
 if(!packed||!out||(bytes!=4&&bytes!=6))return UC_ANIMATION_ARGUMENT;
 float q[4];if(!quaternion((const uint8_t*)packed,(unsigned)bytes,q))return UC_ANIMATION_FORMAT;
 memcpy(out,q,sizeof q);return UC_ANIMATION_OK;
}
static int annotations(Reader*r,UCAnimationClip*c){
 const uint8_t*p;size_t start=r->at;uint32_t total=0,strings;
 p=take(r,4);NEED(p);c->event_count=u32(p);REQUIRE(c->event_count<=65536,UC_ANIMATION_LIMIT);
 if(c->event_count){
  p=take(r,4);NEED(p);c->parameter_count=u32(p);REQUIRE(c->parameter_count<=65536,UC_ANIMATION_LIMIT);
  for(uint32_t i=0;i<c->event_count;++i){p=take(r,12);NEED(p);uint32_t n=u32(p+8);REQUIRE(n<=c->parameter_count-total,UC_ANIMATION_FORMAT);total+=n;NEED(take(r,(size_t)n*8));}
  REQUIRE(total==c->parameter_count,UC_ANIMATION_FORMAT);strings=(c->event_count+c->parameter_count)*2;
  p=take(r,4);NEED(p);uint32_t n=u32(p);p=take(r,n);NEED(p);size_t at=0;
  for(uint32_t i=0;i<strings;++i){const uint8_t*end=(const uint8_t*)memchr(p+at,0,n-at);REQUIRE(end,UC_ANIMATION_FORMAT);at=(size_t)(end-p)+1;}
  REQUIRE(at==n,UC_ANIMATION_FORMAT);
 }
 p=take(r,4);NEED(p);c->tag_count=u32(p);REQUIRE(c->tag_count<=65536,UC_ANIMATION_LIMIT);NEED(take(r,(size_t)c->tag_count*8));
 REQUIRE(r->at==r->size,UC_ANIMATION_UNSUPPORTED);c->annotation_bytes=r->at-start;c->annotations=(uint8_t*)copy(r,r->p+start,c->annotation_bytes);NEED(c->annotations);return 1;
}
static int clip_parse(Reader*r,UCAnimationClip*c){
 const uint8_t*h=take(r,42),*p,*mask=NULL,*blob;size_t mask_stride,blob_size,pairs;unsigned rotation_bytes;
 NEED(h);REQUIRE(!memcmp(h,"KFNM",4),UC_ANIMATION_FORMAT);memcpy(c->source_header,h,42);
 c->source_control=u32(h+4);REQUIRE(c->source_control==0||c->source_control==2,UC_ANIMATION_UNSUPPORTED);memcpy(c->source_timing,h+8,12);c->source_format=u32(h+20);c->source_flags=u16(h+24);
 /*1F3854 and sampler1F3BA0 select quaternion width with bit1; authored Brute Point
  * uses format1, with the same four-byte stream and sampling as format0. */
 REQUIRE(c->source_format<=2&&(c->source_flags==1||c->source_flags==9),UC_ANIMATION_UNSUPPORTED);
 for(unsigned i=0;i<3;++i)REQUIRE(isfinite(c->source_timing[i]),UC_ANIMATION_FORMAT);
 c->track_count=u32(h+26);c->key_count=u32(h+30);memcpy(&c->root_track,h+34,4);blob_size=u32(h+38);
 REQUIRE(c->track_count&&c->track_count<=4096&&c->key_count>=2&&c->key_count<=65536,UC_ANIMATION_LIMIT);
 REQUIRE(c->root_track==-1||(c->root_track>=0&&(uint32_t)c->root_track<c->track_count),UC_ANIMATION_FORMAT);
 pairs=(size_t)c->track_count*c->key_count;REQUIRE(pairs<=4000000,UC_ANIMATION_LIMIT);
 p=take(r,(size_t)c->track_count*4);NEED(p);c->source_tracks=(int32_t*)copy(r,p,(size_t)c->track_count*4);NEED(c->source_tracks);
 for(uint32_t i=0;i<c->track_count;++i){int32_t bone=c->source_tracks[i]<0?~c->source_tracks[i]:c->source_tracks[i];REQUIRE(bone<4096,UC_ANIMATION_LIMIT);}
 mask_stride=(c->track_count+7u)/8;
 if(c->source_flags&8){mask=take(r,mask_stride*c->key_count);NEED(mask);
  if(c->track_count&7)for(uint32_t f=0;f<c->key_count;++f)REQUIRE((mask[f*mask_stride+mask_stride-1]>>(c->track_count&7))==0,UC_ANIMATION_UNSUPPORTED);
 }
 blob=take(r,blob_size);NEED(blob);p=take(r,(size_t)c->key_count*4);NEED(p);
 c->key_times=(float*)copy(r,p,(size_t)c->key_count*4);NEED(c->key_times);
 REQUIRE(c->key_times[0]==0&&c->key_times[c->key_count-1]==1,UC_ANIMATION_UNSUPPORTED);
 for(uint32_t i=0;i<c->key_count;++i)REQUIRE(isfinite(c->key_times[i])&&(!i||c->key_times[i]>c->key_times[i-1]),UC_ANIMATION_FORMAT);
 NEED(annotations(r,c));c->keys=(UCAnimationKey*)array(r,pairs,sizeof(*c->keys));NEED(c->keys);
 Reader b={blob,blob_size,0,UC_ANIMATION_OK};
 for(uint32_t f=0;f<c->key_count;++f)for(uint32_t t=0;t<c->track_count;++t){
  float*dst=c->keys[(size_t)f*c->track_count+t].translation;
  if(f&&c->source_tracks[t]<0)memcpy(dst,c->keys[t].translation,12);
  else {p=take(&b,12);if(!p){r->status=b.status;return 0;}memcpy(dst,p,12);}
  for(unsigned k=0;k<3;++k)REQUIRE(isfinite(dst[k]),UC_ANIMATION_FORMAT);
 }
 rotation_bytes=c->source_format&2?6:4;
 for(uint32_t f=0;f<c->key_count;++f)for(uint32_t t=0;t<c->track_count;++t){
  float*dst=c->keys[(size_t)f*c->track_count+t].rotation;
  if(mask&&(mask[f*mask_stride+t/8]&(1u<<(t%8)))){REQUIRE(f,UC_ANIMATION_FORMAT);memcpy(dst,c->keys[(size_t)(f-1)*c->track_count+t].rotation,16);}
  else {p=take(&b,rotation_bytes);if(!p){r->status=b.status;return 0;}REQUIRE(quaternion(p,rotation_bytes,dst),UC_ANIMATION_FORMAT);}
 }
 REQUIRE(b.at==b.size,UC_ANIMATION_FORMAT);return 1;
}
void uc_animation_template_free(UCAnimationTemplate*t){if(!t)return;if(t->bone_names)for(uint32_t i=0;i<t->bone_count;++i)free(t->bone_names[i]);if(t->clip_names)for(uint32_t i=0;i<t->clip_count;++i)free(t->clip_names[i]);if(t->clip_aliases)for(uint32_t i=0;i<t->clip_count;++i)free(t->clip_aliases[i]);free(t->source_label);free(t->bone_names);free(t->bone_flags);free(t->clip_names);free(t->clip_aliases);memset(t,0,sizeof(*t));}
void uc_animation_clip_free(UCAnimationClip*c){if(!c)return;free(c->source_tracks);free(c->key_times);free(c->keys);free(c->annotations);memset(c,0,sizeof(*c));}
static unsigned ascii_lower(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static UCAnimationStatus ascii_name(const char*s){
 if(!s)return UC_ANIMATION_ARGUMENT;
 for(unsigned i=0;i<4096;++i){unsigned c=(unsigned char)s[i];if(!c)return UC_ANIMATION_OK;if(c>127)return UC_ANIMATION_UNSUPPORTED;}
 return UC_ANIMATION_LIMIT;
}
static int alias_equal(const char*a,const char*b){
 while(*a&&ascii_lower((unsigned char)*a)==ascii_lower((unsigned char)*b)){++a;++b;}
 return *a==*b;
}
UCAnimationStatus uc_animation_template_find_clip(const UCAnimationTemplate*t,const char*alias,uint32_t*index){
 uint32_t found=UINT32_MAX;UCAnimationStatus s;
 if(!t||!index||(t->clip_count&&(!t->clip_names||!t->clip_aliases)))return UC_ANIMATION_ARGUMENT;
 if(t->clip_count>65536)return UC_ANIMATION_LIMIT;
 if(!alias)alias="";s=ascii_name(alias);if(s)return s;
 for(uint32_t i=0;i<t->clip_count;++i){
  s=ascii_name(t->clip_aliases[i]);if(s)return s;
  s=ascii_name(t->clip_names[i]);if(s)return s;
  if(alias_equal(t->clip_aliases[i],alias)){if(found!=UINT32_MAX)return UC_ANIMATION_UNSUPPORTED;found=i;}
 }
 if(found!=UINT32_MAX)for(uint32_t i=0;i<t->clip_count;++i)
  if(i!=found&&alias_equal(t->clip_names[i],t->clip_names[found]))return UC_ANIMATION_UNSUPPORTED;
 *index=found;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_template_find_action(const UCAnimationTemplate*t,const char*prefix,const char*alias,uint32_t*index){
 if(!index)return UC_ANIMATION_ARGUMENT;
 if(prefix){
  UCAnimationStatus s=ascii_name(prefix);if(s)return s;s=ascii_name(alias);if(s)return s;
  size_t a=strlen(prefix),b=strlen(alias);if(a+b>=1024)return UC_ANIMATION_LIMIT;
  char name[1024];memcpy(name,prefix,a);memcpy(name+a,alias,b+1);uint32_t found;
  s=uc_animation_template_find_clip(t,name,&found);if(s)return s;
  if(found!=UINT32_MAX){*index=found;return UC_ANIMATION_OK;}
 }
 return uc_animation_template_find_clip(t,alias,index);
}
UCAnimationStatus uc_animation_template_load(const void*p,size_t n,UCAnimationTemplate*t){if(!t)return UC_ANIMATION_ARGUMENT;memset(t,0,sizeof(*t));if(!p)return UC_ANIMATION_ARGUMENT;if(n>64u*1024u*1024u)return UC_ANIMATION_LIMIT;Reader r={(const uint8_t*)p,n,0,UC_ANIMATION_OK};if(!template_parse(&r,t)){uc_animation_template_free(t);return r.status;}return UC_ANIMATION_OK;}
UCAnimationStatus uc_animation_clip_load(const void*p,size_t n,UCAnimationClip*c){if(!c)return UC_ANIMATION_ARGUMENT;memset(c,0,sizeof(*c));if(!p)return UC_ANIMATION_ARGUMENT;if(n>64u*1024u*1024u)return UC_ANIMATION_LIMIT;Reader r={(const uint8_t*)p,n,0,UC_ANIMATION_OK};if(!clip_parse(&r,c)){uc_animation_clip_free(c);return r.status;}return UC_ANIMATION_OK;}
const char*uc_animation_status(UCAnimationStatus s){static const char*const n[]={"ok","argument","format","unsupported layout","limit","memory"};return (unsigned)s<sizeof(n)/sizeof(*n)?n[s]:"unknown";}
