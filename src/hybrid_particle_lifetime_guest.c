#include "hybrid_particle_lifetime_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00238F40(void);
extern void sub_0023A190(void);
extern void sub_0023A1C0(void);
extern void sub_0023AB60(void);
extern void sub_0023AB90(void);
extern void sub_0023B3E0(void);
extern void sub_0023B410(void);
extern void sub_0023BAF0(void);
extern void sub_0023BB20(void);
extern void sub_0023CFF0(void);
extern void sub_0023D020(void);
extern void sub_0023D620(void);
extern void sub_0023D650(void);
extern void sub_002D5CCD(void);
static int lifetime_enabled(void){static int value=-1;if(value<0){const char*p=getenv("UCRR_NATIVE_PARTICLE_LIFETIME");value=p&&!strcmp(p,"1");}return value;}
/* Six original concrete particle owners. Metadata below is only the literal
 * per-family vtable/SEH/return-address difference; all guest fields and callback
 * reloads remain authoritative. There is no second pool or host object graph. */
typedef struct LifetimeFamily {
 uint32_t ctor,adjust,deleting,dtor,primary,render,base,ctor_seh,dtor_seh;
 uint32_t constructed,registered,unregistered,freed_buffer,freed_source,freed_emitter,freed_descriptor;
 unsigned register_slot,handle_offset;
 void (*destroy)(void),(*delete_owner)(void);
} LifetimeFamily;
static const LifetimeFamily lifetime_families[]={
 {0x239020,0x239100,0x23a190,0x23a1c0,0x482fac,0x482f90,0x4502c0,0x39e17e,0x39e1ca,0x23908c,0x2390a3,0x23a206,0x23a212,0x23a222,0x23a22f,0x23a242,0x70,0xb4,sub_0023A1C0,sub_0023A190},
 {0x23a270,0x23a320,0x23ab60,0x23ab90,0x482fe0,0x482fc0,0x457b6c,0x39e1fe,0x39e24a,0x23a2dc,0x23a2f6,0x23abd9,0x23abe5,0x23abf5,0x23ac02,0x23ac15,0x80,0xb4,sub_0023AB90,sub_0023AB60},
 {0x23ac40,0x23acf0,0x23b3e0,0x23b410,0x483010,0x482ff0,0x457b6c,0x39e1fe,0x39e24a,0x23acac,0x23acc6,0x23b459,0x23b465,0x23b475,0x23b482,0x23b495,0x80,0xb4,sub_0023B410,sub_0023B3E0},
 {0x23b4c0,0x23b560,0x23baf0,0x23bb20,0x48303c,0x483020,0x472cbc,0x39e27e,0x39e2ca,0x23b52c,0x23b543,0x23bb66,0x23bb72,0x23bb82,0x23bb8f,0x23bba2,0x78,0xb4,sub_0023BB20,sub_0023BAF0},
 {0x23bbd0,0x23bd30,0x23cff0,0x23d020,0x483070,0x483050,0x457b6c,0x39e1fe,0x39e24a,0x23bc3c,0x23bc56,0x23d069,0x23d075,0x23d085,0x23d092,0x23d0a5,0x80,0xb4,sub_0023D020,sub_0023CFF0},
 {0x23d0d0,0x23d5f0,0x23d620,0x23d650,0x48309c,0x483080,0x482f44,0x39e2fe,0x39e34a,0x23d13c,0x23d161,0x23d699,0x23d6a5,0x23d6b5,0x23d6c2,0x23d6d5,0x88,0xb8,sub_0023D650,sub_0023D620}
};
static int lifetime_span(uint32_t p,unsigned n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int lifetime_apart(uint32_t p,unsigned n,uint32_t q,unsigned m){return(uint64_t)p+n<=q||(uint64_t)q+m<=p;}
static void lifetime_base_construct(void){
 uint32_t frame=g_ebp,saved=g_esp;PUSH32(g_esp,frame);frame=g_esp;g_ebp=frame;g_esp=(g_esp&~15u)-68;
 g_xmm0=XMM_ZERO();g_edx=MEM32(frame+16);g_eax=MEM32(frame+8);PUSH32(g_esp,g_ebx);g_ebx=g_ecx;
 g_ecx=MEM32(frame+12);MEM32(g_ebx+4)=g_ecx;g_ecx=MEM32(frame+28);PUSH32(g_esp,g_esi);MEM32(g_ebx+8)=g_edx;
 g_edx=MEM32(frame+32);MEM32(g_ebx+20)=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_ebx+24)=g_edx;MEM32(g_ebx)=g_eax;
 g_eax=MEM32(frame+20);g_edx=g_ebx+96;for(unsigned i=0;i<12;i++)MEMF(g_esp+16+i*4)=g_xmm0.f[0];
 g_edi=g_edx;MEM32(g_ebx+12)=g_eax;g_eax=MEM32(frame+24);g_ecx=12;g_esi=g_esp+16;
 memcpy((void*)XBOX_PTR(g_edi),(const void*)XBOX_PTR(g_esi),48);g_esi+=48;g_edi+=48;g_ecx=0;
 PUSH32(g_esp,g_edx);g_ecx=g_esp+20;MEM32(g_ebx+16)=g_eax;MEM32(g_ebx+144)=0;g_esi=MEM32(g_eax);PUSH32(g_esp,g_ecx);g_ecx=g_eax;
 {uint32_t target=MEM32(g_esi+8);PUSH32(g_esp,0x238fe9);RECOMP_ICALL_SAFE(target,saved);}
 g_xmm0=XMM_MEM(g_eax);XMM_STORE(g_ebx+32,g_xmm0);g_xmm0=XMM_MEM(g_eax+16);XMM_STORE(g_ebx+48,g_xmm0);
 g_xmm0=XMM_MEM(g_eax+32);POP32(g_esp,g_edi);XMM_STORE(g_ebx+64,g_xmm0);g_xmm0=XMM_MEM(g_eax+48);POP32(g_esp,g_esi);XMM_STORE(g_ebx+80,g_xmm0);
 g_eax=g_ebx;POP32(g_esp,g_ebx);g_esp=frame;POP32(g_esp,frame);g_recomp_zf=g_esp==0;g_esp+=32;
}
static void lifetime_construct(const LifetimeFamily*f){
 PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,f->ctor_seh);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,g_edi);MEM32(g_esp+12)=g_esi;
 g_edi=g_esi+4;MEM32(g_esp+24)=0;MEM32(g_edi)=f->base;g_ebx=MEM32(g_esp+56);g_eax=MEM32(g_esp+52);g_ecx=MEM32(g_esp+48);g_edx=MEM32(g_esp+44);
 PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_eax);g_eax=MEM32(g_esp+48);PUSH32(g_esp,g_ecx);g_ecx=MEM32(g_esp+48);PUSH32(g_esp,g_edx);g_edx=MEM32(g_esp+48);
 PUSH32(g_esp,g_eax);PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_edx);g_ecx=g_esi+16;MEM8(g_esp+52)=1;MEM32(g_esi)=f->primary;MEM32(g_edi)=f->render;
 PUSH32(g_esp,f->constructed);sub_00238F40();
 if(f->handle_offset==0xb8){g_xmm0=XMM_ZERO();MEM32(g_esi+180)=g_ebx;MEMF(g_esi+176)=g_xmm0.f[0];}else MEM32(g_esi+176)=g_ebx;
 g_ecx=MEM32(0x5150b0);g_eax=MEM32(g_ecx);
 {uint32_t saved=g_esp;PUSH32(g_esp,g_edi);MEM8(g_esp+28)=2;uint32_t target=MEM32(g_eax+f->register_slot);PUSH32(g_esp,f->registered);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esp+16);MEM32(g_esi+f->handle_offset)=g_eax;POP32(g_esp,g_edi);g_eax=g_esi;POP32(g_esp,g_esi);POP32(g_esp,g_ebx);MEM32(0)=g_ecx;
 g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=32;
}
static void lifetime_destroy(const LifetimeFamily*f){
 uint32_t saved=g_esp;PUSH32(g_esp,0xffffffffu);PUSH32(g_esp,f->dtor_seh);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;
 g_esp-=8;PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+8)=g_esi;MEM32(g_esi)=f->primary;MEM32(g_esi+4)=f->render;
 g_ecx=MEM32(0x5150b0);g_edx=MEM32(g_esi+f->handle_offset);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_edx);MEM32(g_esp+24)=2;
 {uint32_t target=MEM32(g_eax+f->register_slot+4);PUSH32(g_esp,f->unregistered);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esi+20);saved=g_esp;PUSH32(g_esp,g_eax);MEM8(g_esp+24)=1;
 {uint32_t target=MEM32(g_esi+24);PUSH32(g_esp,f->freed_buffer);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esi+32);g_esp+=4;
 if(g_ecx){g_edx=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_edx);PUSH32(g_esp,f->freed_source);RECOMP_ICALL_SAFE(target,saved);}
 g_ecx=MEM32(g_esi+28);
 if(g_ecx){g_eax=MEM32(g_ecx);saved=g_esp;PUSH32(g_esp,1);uint32_t target=MEM32(g_eax);PUSH32(g_esp,f->freed_emitter);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=MEM32(g_esi+36);SET_LO8(g_ecx,MEM8(g_eax+272));
 if(LO8(g_ecx)){PUSH32(g_esp,g_eax);PUSH32(g_esp,f->freed_descriptor);sub_002D5CCD();g_esp+=4;}
 g_ecx=MEM32(g_esp+12);MEM32(g_esi+4)=f->base;MEM32(g_esi)=0x482f80;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=20;g_recomp_zf=g_esp==0;g_esp+=4;
}
int uc_hybrid_particle_lifetime(uint32_t entry){
 if(!lifetime_enabled())return 0;
 uint32_t s=g_esp,p=g_ecx;const LifetimeFamily*f=NULL;
 for(unsigned i=0;i<6;i++)if(entry==lifetime_families[i].ctor||entry==lifetime_families[i].adjust||entry==lifetime_families[i].deleting||entry==lifetime_families[i].dtor){f=&lifetime_families[i];break;}
 if(!f&&entry!=0x238f40)return 0;
 if((s&3)||s<0x10800u||!lifetime_span(s-2048,2080)||!lifetime_span(0x51c178,1)||!lifetime_apart(s-2048,2080,0x51c178,1)||MEM8(0x51c178))return 0;
 if(f&&entry==f->adjust){if(p<4)return 0;p-=4;}
 if(!lifetime_span(p,f?192:148)||!lifetime_apart(p,f?192:148,s-2048,2080)||!lifetime_apart(p,f?192:148,0x51c178,1))return 0;
 if(f&&entry!=f->ctor&&(MEM32(p)!=f->primary||MEM32(p+4)!=f->render))return 0;
 /* The original completing callbacks retain live object/stack ownership.
  * They may change later fields and registers; never reject after effects. */
 if(!f){lifetime_base_construct();return 1;}
 if(entry==f->ctor){lifetime_construct(f);return 1;}
 if(entry==f->dtor){lifetime_destroy(f);return 1;}
 if(entry==f->adjust){uint32_t frame=g_seh_ebp;g_ecx-=4;g_seh_ebp=frame;f->delete_owner();return 1;}
 PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,f->deleting+8);f->destroy();int deleting=MEM8(g_esp+8)&1u;
 if(deleting){g_ecx=MEM32(0x51ca90);g_eax=MEM32(g_ecx);uint32_t saved=g_esp;PUSH32(g_esp,g_esi);uint32_t target=MEM32(g_eax+8);PUSH32(g_esp,f->deleting+27);RECOMP_ICALL_SAFE(target,saved);}
 g_eax=g_esi;POP32(g_esp,g_esi);g_recomp_zf=!deleting;g_esp+=8;return 1;
}
