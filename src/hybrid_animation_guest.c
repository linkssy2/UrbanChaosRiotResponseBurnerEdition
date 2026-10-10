#include "hybrid_animation_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include "../native/animation_skeleton.h"
#include "../native/model_pose.h"
#include <immintrin.h>
#include <stdlib.h>
#include <string.h>

enum { MIN_BONES=6,MAX_BONES=256 };
typedef struct Guard { uint32_t output,bytes,stack,stack_bytes; } Guard;
static unsigned char*view(uint32_t p,unsigned n){
 /* Primary ordinary RAM only; every high mirror/alias declines. */
 return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram?(unsigned char*)XBOX_PTR(p):NULL;
}
static int separate(uint32_t a,unsigned n,uint32_t b,unsigned m){
 return (uint64_t)a+n<=b||(uint64_t)b+m<=a;
}
static int readable(const Guard*g,uint32_t p,unsigned n){
 return view(p,n)&&separate(p,n,g->output,g->bytes)&&separate(p,n,g->stack,g->stack_bytes);
}
static int guard(Guard*g,uint32_t out,unsigned bytes,uint32_t stack,unsigned depth,unsigned args){
 if((stack&3)||stack<0x10000u+depth)return 0;
 *g=(Guard){out,bytes,stack-depth,depth+args};
 return view(out,bytes)&&view(g->stack,g->stack_bytes)&&separate(out,bytes,g->stack,g->stack_bytes);
}
static int finite(const void*p,unsigned bytes){
 const unsigned char*s=(const unsigned char*)p;
 for(unsigned i=0;i<bytes;i+=4){uint32_t b;memcpy(&b,s+i,4);if((b&0x7fffffffu)>=0x7f800000u)return 0;}return 1;
}
static int constant(const Guard*g,uint32_t at,uint32_t bits){return readable(g,at,4)&&MEM32(at)==bits;}
static int active(void){
 static int enabled=-1;if(enabled<0){const char*v=getenv("UCRR_NATIVE_ANIMATION_POSE");enabled=v&&!strcmp(v,"1")&&!getenv("UCRR_CONSTANT_SOURCE_TRACE");}
 /* The original 2D3F00 entry owns a diagnostic in non-play builds. Preserve
  * every original trace call when that diagnostic is requested. */
 /* The pinned title normally disables guest workers. This is a configuration
  * qualification, not a lock or proof that a previously started worker exited. */
 return enabled&&view(0x51c178,1)&&!MEM8(0x51c178)&&(_mm_getcsr()&~63u)==0x1f80u;
}
static int locals(uint32_t state,uint32_t stack,uint32_t*last_enabled){
 if(!view(state,40)||!view(stack,8))return 0;
 uint32_t n=MEM32(state+36),out=MEM32(stack+4);if(n<MIN_BONES||n>MAX_BONES)return 0;
 uint32_t mask=MEM32(state+12);if(!view(mask,n))return 0;
 unsigned active_bones=0;for(uint32_t i=1;i<n&&active_bones<3;i++)active_bones+=MEM8(mask+i)!=0;
 /* The exact tail repeats the last quaternion call. Sparse populations have
  * no measured gain; decline before copying poses or visiting their arrays. */
 if(active_bones<3)return 0;
 Guard g;if(!guard(&g,out,n*64,stack,128,8)||!readable(&g,state,40)||
   !constant(&g,0x44bf94,0x3f800000)||!constant(&g,0x44bf80,0xbf800000)||!constant(&g,0x44c530,0x40000000)||
   !readable(&g,0x51c178,1))return 0;
 uint32_t templ=MEM32(state+4),scales=MEM32(state+16),offsets=MEM32(state+20),records=MEM32(state+32);
 if(!readable(&g,templ,28)||!readable(&g,mask,n)||!readable(&g,scales,n*4)||!readable(&g,offsets,n*12)||!readable(&g,records,n*48))return 0;
 UCAnimationSkeleton s={n,(int32_t)MEM32(templ+24),MEM8(state+28),view(mask,n),(const float*)view(scales,n*4),(const float(*)[3])view(offsets,n*12)};
 UCAnimationBlend poses[MAX_BONES];float local[MAX_BONES][16],scratch[MAX_BONES][16];uint32_t last=0;
 for(uint32_t i=1;i<n;i++)if(s.enabled[i]){memcpy(&poses[i].pose,view(records+i*48,28),28);last=i;}
 if(uc_animation_skeleton_locals(&s,poses,local,scratch)!=UC_ANIMATION_OK)return 0;
 /* The kernel reads no prior local and writes only root/enabled slots. */
 for(uint32_t i=0;i<n;i++)if(!i||s.enabled[i])memcpy(view(out+i*64,64),local[i],64);
 *last_enabled=last;return 1;
}
static int world(uint32_t model,uint32_t stack){
 if(!view(model,0x70)||!view(stack,12))return 0;
 uint32_t asset=MEM32(model+0x68);if(!view(asset,0x44))return 0;
 uint32_t n=MEM32(asset+12),out=MEM32(stack+4),root=MEM32(stack+8);if(n<MIN_BONES||n>MAX_BONES)return 0;
 Guard g;if(!guard(&g,out,n*64,stack,112,12)||!readable(&g,model,0x70)||!readable(&g,asset,0x44)||!readable(&g,root,64)||!readable(&g,0x51c178,1))return 0;
 uint32_t metadata=MEM32(asset+0x40),input=MEM32(model+0x6c);
 if(!readable(&g,metadata,n*160)||!readable(&g,input,n*64))return 0;
 UCModel m={0};UCModelBone bones[MAX_BONES];float output[MAX_BONES][16];m.bone_count=n;m.bones=bones;m.source_header[73]=1;
 for(uint32_t i=0;i<n;i++)bones[i].parent=MEM16(metadata+i*160+22);
 if(uc_model_pose_world(&m,(const float(*)[16])view(input,n*64),(const float*)view(root,64),output)!=UC_POSE_OK||!finite(output,n*64))return 0;
 memcpy(view(out,n*64),output,n*64);return 1;
}
static int palette(uint32_t model,uint32_t stack){
 if(!view(model,0x78)||!view(stack,4))return 0;
 uint32_t asset=MEM32(model+0x68);if(!view(asset,0x48))return 0;
 uint32_t n=MEM32(asset+12),out=MEM32(model+0x74);if(n<MIN_BONES||n>MAX_BONES)return 0;
 Guard g;if(!guard(&g,out,n*48,stack,208,4)||!readable(&g,model,0x78)||!readable(&g,asset,0x48)||
  !constant(&g,0x44bf94,0x3f800000)||!readable(&g,0x51c178,1))return 0;
 uint32_t metadata=MEM32(asset+0x40),input=MEM32(model+0x70);
 if(!readable(&g,metadata,n*160)||!readable(&g,input,n*64))return 0;
 UCModel m={0};UCModelBone bones[MAX_BONES];float output[MAX_BONES][12];m.bone_count=n;m.bones=bones;memcpy(&m.position_scale,view(asset+0x44,4),4);
 for(uint32_t i=0;i<n;i++)memcpy(bones[i].bind_correction,view(metadata+i*160+96,64),64);
 if(uc_model_pose_palette(&m,(const float(*)[16])view(input,n*64),output)!=UC_POSE_OK||!finite(output,n*48))return 0;
 memcpy(view(out,n*48),output,n*48);return 1;
}
int uc_hybrid_animation_locals(uint32_t state,uint32_t stack,uint32_t*last_enabled){
 if(!active()||!last_enabled)return 0;unsigned csr=_mm_getcsr();int ok=locals(state,stack,last_enabled);if(!ok)_mm_setcsr(csr);return ok;
}
int uc_hybrid_animation_world(uint32_t model,uint32_t stack){
 if(!active())return 0;unsigned csr=_mm_getcsr();int ok=world(model,stack);if(!ok)_mm_setcsr(csr);return ok;
}
int uc_hybrid_animation_palette(uint32_t model,uint32_t stack){
 if(!active())return 0;unsigned csr=_mm_getcsr();int ok=palette(model,stack);if(!ok)_mm_setcsr(csr);return ok;
}
