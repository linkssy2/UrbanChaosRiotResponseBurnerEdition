#include "migration_audio_commands_guest.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
#include <string.h>
extern void sub_00241940(void);
extern void sub_00241610(void);
extern void sub_00244AB0(void);
extern void sub_00244FE0(void);
extern void sub_002449E0(void);
extern void sub_00244F80(void);
extern void sub_00245020(void);
extern void sub_00244B80(void);
extern void sub_0024B530(void);
extern void sub_00243F60(void);
extern void sub_002D5CCD(void);
static int commands_enabled(void){static int f=-1;if(f<0){const char*v=getenv("UCRR_MIGRATION_AUDIO_COMMANDS");f=v&&!strcmp(v,"1");}return f;}
static int commands_span(uint32_t p,uint32_t n){return p>=0x10000u&&p<0x80000000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int commands_safe(uint32_t p,uint32_t n){return commands_span(p,n)&&((uint64_t)p+n<=g_esp-4096||(uint64_t)g_esp+12<=p);}
static int commands_queue(uint32_t q,uint32_t offset){
    if(!commands_safe(q,12))return 0;uint32_t count=MEM32(q+8),capacity=MEM32(q+4),items=MEM32(q);
    if(!capacity||capacity>65535||count>capacity||!commands_safe(items,capacity*4))return 0;
    /* ponytail: full live-array admission is O(n); move it to a shared validated owner when queue publication owns that invariant. */
    for(uint32_t j=0;j<count;j++){uint32_t p=MEM32(items+j*4);if(!commands_safe(p,96)||MEM16(p+offset)!=j+1)return 0;}
    return 1;
}
static int commands_admit(uint32_t a){
    if(!commands_enabled()||!commands_span(0x51c178,1)||MEM8(0x51c178)||(g_esp&3)||g_esp<0x11000||!commands_span(g_esp-4096,4108))return 0;
    uint32_t arg=MEM32(g_esp+4),q=g_ecx;
    if(a==0x24d200||a==0x24b530){if(!commands_safe(q,96))return 0;uint32_t n=q+28;return commands_safe(MEM32(n),8)&&commands_safe(MEM32(n+4),8);}
    if(a==0x23fea0||a==0x241940||a==0x23ff00||a==0x23ff50){
        if(!commands_safe(q,24)||!commands_queue(MEM32(q+16),88))return 0;
        if(a==0x241940)return commands_safe(arg,8);
        if(!commands_safe(arg,96))return 0;
        if(a==0x23fea0)return 1;
        if(!commands_queue(MEM32(q+20),90))return 0;
        uint32_t head=arg+28,p=MEM32(head),previous=head,count=0;
        while(p!=head){
            if(++count>65535||p<28||!commands_safe(p-28,96)||MEM32(p+4)!=previous)return 0;
            if(a==0x23ff00&&(MEM32(p-4)!=1||!commands_span(MEM32(p-28)+4,4)||MEM32(MEM32(p-28)+4)!=0x24d200))return 0;
            uint32_t record=p-28,heap=MEM32(q+16),ready=MEM32(q+20),hi=MEM16(record+88),ri=MEM16(record+90);
            if(hi&&ri)return 0;
            if(hi&&(hi>MEM32(heap+8)||MEM32(MEM32(heap)+(hi-1)*4)!=record))return 0;
            if(ri&&(ri>MEM32(ready+8)||MEM32(MEM32(ready)+(ri-1)*4)!=record))return 0;
            previous=p;p=MEM32(p);
        }
        return MEM32(head+4)==previous;
    }
    uint32_t offset=(a==0x244b80||a==0x245020)?90:88;
    if(a!=0x2449e0&&a!=0x244ab0&&a!=0x244b80&&a!=0x244f80&&a!=0x244fe0&&a!=0x245020)return 0;
    if(!commands_queue(q,offset))return 0;if(a==0x2449e0)return 1;if(a==0x244fe0)return commands_safe(arg,8);
    if(!commands_safe(arg,96))return 0;
    if(a==0x244f80||a==0x245020){uint32_t index=MEM16(arg+offset);return !index||(index<=MEM32(q+8)&&MEM32(MEM32(q)+(index-1)*4)==arg);}
    return MEM16(arg+88)==0&&MEM16(arg+90)==0;
}
static void commands_heap_push(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(g_ecx+8);g_edx=MEM32(g_ecx+4);g_esp-=8;
    if(g_eax==g_edx){SET_LO8(g_eax,0);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;return;}
    g_edx=g_eax+1;g_eax=MEM32(g_esp+12);PUSH32(g_esp,g_esi);g_esi=MEM32(g_ecx);MEM32(g_ecx+8)=g_edx;MEM32(g_esi+g_edx*4-4)=g_eax;MEM16(g_eax+88)=LO16(g_edx);
    if(g_edx>1){PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
        do{
            g_esi=MEM32(g_ecx);g_edi=MEM32(g_esi+g_edx*4-4);g_eax=g_edx>>1;frame=MEM32(g_esi+g_eax*4-4);g_ebx=MEM32(frame+8);uint32_t borrow=g_ebx<MEM32(g_edi+8);g_ebx-=MEM32(g_edi+8);frame=MEM32(frame+12);uint32_t before=frame,other=MEM32(g_edi+12);frame-=other+borrow;int overflow=((before^other)&(before^frame)&0x80000000u)!=0;MEM32(g_esp+20)=frame;
            if((int32_t)frame<0||!((frame!=0&&!overflow)||g_ebx!=0))break;
            g_ebx=MEM32(g_esi+g_eax*4-4);MEM32(g_esi+g_eax*4-4)=g_edi;MEM16(g_edi+88)=LO16(g_eax);g_esi=MEM32(g_ecx);MEM32(g_esi+g_edx*4-4)=g_ebx;MEM16(g_ebx+88)=LO16(g_edx);g_edx=g_eax;
        }while(g_eax>1);
        POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
    }SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void commands_heap_pop(void){
    uint32_t frame=g_seh_ebp;g_eax=MEM32(g_ecx+8);g_esp-=12;
    if(!g_eax){g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;return;}
    g_edx=MEM32(g_ecx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_edx);MEM32(g_esp+4)=g_esi;
    if(g_eax!=1){g_esi=MEM32(g_edx+g_eax*4-4);MEM32(g_edx)=g_esi;g_edx=MEM32(g_ecx);g_edx=MEM32(g_edx);MEM16(g_edx+88)=1;g_edx=MEM32(g_ecx);MEM32(g_edx+g_eax*4-4)=0;}g_edx=1;
    if(MEM32(g_ecx+8)>2){PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);PUSH32(g_esp,g_edi);
        for(;;){
            g_eax=MEM32(g_ecx+8);g_esi=g_edx*2+1;
            if(g_esi<g_eax){g_eax=MEM32(g_ecx);g_ebx=MEM32(g_eax+g_edx*8-4);frame=MEM32(g_eax+g_edx*8);g_edi=MEM32(g_ebx+8);g_eax+=g_edx*8;uint32_t borrow=g_edi<MEM32(frame+8);g_edi-=MEM32(frame+8);g_eax=MEM32(g_ebx+12);g_eax-=MEM32(frame+12)+borrow;if((int32_t)g_eax<0)g_esi=g_edx*2;}
            else g_esi=g_edx*2;
            g_edi=MEM32(g_ecx);frame=MEM32(g_edi+g_edx*4-4);g_ebx=MEM32(frame+8);g_eax=MEM32(g_edi+g_esi*4-4);uint32_t borrow=g_ebx<MEM32(g_eax+8);g_ebx-=MEM32(g_eax+8);frame=MEM32(frame+12);uint32_t before=frame,other=MEM32(g_eax+12);frame-=other+borrow;int overflow=((before^other)&(before^frame)&0x80000000u)!=0;MEM32(g_esp+24)=frame;
            if((int32_t)frame<0||!((frame!=0&&!overflow)||g_ebx!=0))break;
            g_ebx=MEM32(g_edi+g_edx*4-4);MEM32(g_edi+g_edx*4-4)=g_eax;MEM16(g_eax+88)=LO16(g_edx);g_edx=MEM32(g_ecx);MEM32(g_edx+g_esi*4-4)=g_ebx;g_eax=g_esi*2;MEM16(g_ebx+88)=LO16(g_esi);g_edx=g_esi;if(g_eax>=MEM32(g_ecx+8))break;
        }POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);
    }MEM32(g_ecx+8)--;g_eax=MEM32(g_esp+4);MEM16(g_eax+88)=0;POP32(g_esp,g_esi);g_esp+=12;g_recomp_zf=g_esp==0;g_esp+=4;
}
static void commands_ready_push(void){g_eax=MEM32(g_ecx+8);if(g_eax==MEM32(g_ecx+4)){SET_LO8(g_eax,0);g_recomp_zf=1;}else{g_edx=MEM32(g_ecx);g_eax++;uint32_t count=g_eax;MEM32(g_ecx+8)=g_eax;g_ecx=MEM32(g_esp+4);MEM32(g_edx+g_eax*4-4)=g_ecx;MEM16(g_ecx+90)=LO16(g_eax);SET_LO8(g_eax,1);g_recomp_zf=count==0;}g_esp+=8;}
static void commands_heap_remove(void){
    g_eax=MEM32(g_esp+4);SET_LO16(g_eax,MEM16(g_eax+88));if(!LO16(g_eax)){SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=8;return;}g_edx=LO16(g_eax);
    if(g_edx>1){PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);do{g_esi=MEM32(g_ecx);g_edi=MEM32(g_esi+g_edx*4-4);g_eax=g_edx>>1;g_ebx=MEM32(g_esi+g_eax*4-4);MEM32(g_esi+g_edx*4-4)=g_ebx;MEM16(g_ebx+88)=LO16(g_edx);g_edx=MEM32(g_ecx);MEM32(g_edx+g_eax*4-4)=g_edi;MEM16(g_edi+88)=LO16(g_eax);g_edx=g_eax;}while(g_eax>1);POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}PUSH32(g_esp,0x244fcf);sub_002449E0();SET_LO8(g_eax,1);g_esp+=8;
}
static void commands_ready_remove(void){
    g_eax=MEM32(g_esp+4);SET_LO16(g_eax,MEM16(g_eax+90));if(!LO16(g_eax)){SET_LO8(g_eax,0);g_recomp_zf=1;g_esp+=8;return;}
    g_edx=LO16(g_eax);g_eax=MEM32(g_ecx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_eax);PUSH32(g_esp,g_edi);g_edi=MEM32(g_eax+g_edx*4-4);MEM32(g_eax)=g_edi;g_eax=1;MEM16(g_edi+90)=LO16(g_eax);g_edi=MEM32(g_ecx);MEM32(g_edi+g_edx*4-4)=g_esi;MEM16(g_esi+90)=LO16(g_edx);g_edx=MEM32(g_ecx+8);
    if(g_edx){g_esi=MEM32(g_ecx);g_edi=MEM32(g_esi);if(g_edx!=g_eax){g_edx=MEM32(g_esi+g_edx*4-4);MEM32(g_esi)=g_edx;g_edx=MEM32(g_ecx);g_edx=MEM32(g_edx);MEM16(g_edx+90)=LO16(g_eax);g_edx=MEM32(g_ecx+8);g_esi=MEM32(g_ecx);MEM32(g_esi+g_edx*4-4)=0;}MEM32(g_ecx+8)--;MEM16(g_edi+90)=0;}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;
}
static void commands_due(void){
    g_edx=MEM32(g_ecx+8);g_eax=0;if(g_edx){g_edx=MEM32(g_ecx);g_edx=MEM32(g_edx);PUSH32(g_esp,g_ebx);PUSH32(g_esp,g_esi);g_esi=MEM32(g_esp+12);g_ebx=MEM32(g_esi+4);PUSH32(g_esp,g_edi);g_edi=MEM32(g_edx+12);int due=(int32_t)g_edi<(int32_t)g_ebx;if(g_edi==g_ebx){g_edx=MEM32(g_edx+8);due=g_edx<=MEM32(g_esi);}if(due){PUSH32(g_esp,0x24500c);sub_002449E0();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,g_ebx);}g_esp+=8;
}
static void commands_drain(void){
    PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+12);g_esi=g_ecx;g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x241951);sub_00244FE0();
    while(g_eax){PUSH32(g_esp,g_eax);g_ecx=g_esi;PUSH32(g_esp,0x24195d);sub_00241610();if(!LO8(g_eax))break;g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x24196a);sub_00244FE0();}POP32(g_esp,g_edi);SET_LO8(g_eax,1);POP32(g_esp,g_esi);g_esp+=8;
}
static void commands_submit(void){
    g_esp-=8;PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=MEM32(g_esp+20);g_edx=MEM32(g_edi+12);g_esi=g_ecx;g_eax=MEM32(g_esi+12);g_ecx=MEM32(g_esi+8);MEM32(g_esp+8)=g_ecx;MEM32(g_esp+12)=g_eax;int due=(int32_t)g_edx<(int32_t)g_eax;
    if(g_edx==g_eax){g_eax=MEM32(g_edi+8);due=g_eax<=g_ecx;}
    if(due){g_ecx=g_esp+8;PUSH32(g_esp,g_ecx);g_ecx=g_esi;PUSH32(g_esp,0x23fed5);sub_00241940();PUSH32(g_esp,g_edi);g_ecx=g_esi;PUSH32(g_esp,0x23fedd);sub_00241610();}
    else{g_ecx=MEM32(g_esi+16);PUSH32(g_esp,g_edi);PUSH32(g_esp,0x23feee);sub_00244AB0();}POP32(g_esp,g_edi);POP32(g_esp,g_esi);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=8;
}
static void commands_cancel(void){
    uint32_t frame=g_seh_ebp;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+12)+28;PUSH32(g_esp,g_edi);g_edi=MEM32(frame);g_ebx=g_ecx;
    if(g_edi!=frame){PUSH32(g_esp,g_esi);do{g_ecx=MEM32(g_ebx+16);g_esi=g_edi-28;g_edi=MEM32(g_edi);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23ff22);sub_00244F80();if(!LO8(g_eax)){g_ecx=MEM32(g_ebx+20);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23ff2f);sub_00245020();}if(!--MEM32(g_esi+24)){g_eax=MEM32(g_esi);uint32_t saved=g_esp;PUSH32(g_esp,1);g_ecx=g_esi;uint32_t target=MEM32(g_eax+4);PUSH32(g_esp,0x23ff3d);RECOMP_ICALL_SAFE(target,saved);}}while(MEM32(frame)!=frame);POP32(g_esp,g_esi);}POP32(g_esp,g_edi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;
}
static void commands_unlink(void){PUSH32(g_esp,0xffffffff);PUSH32(g_esp,0x39e858);g_eax=MEM32(0);PUSH32(g_esp,g_eax);MEM32(0)=g_esp;PUSH32(g_esp,g_ecx);PUSH32(g_esp,g_esi);g_esi=g_ecx;MEM32(g_esp+4)=g_esi;MEM32(g_esi)=0x483eb0;g_eax=g_esi+28;PUSH32(g_esp,g_eax);MEM32(g_esp+20)=0;PUSH32(g_esp,0x24b564);sub_00243F60();g_ecx=MEM32(g_esp+12);g_esp+=4;MEM32(g_esi)=0x483e1c;POP32(g_esp,g_esi);MEM32(0)=g_ecx;g_esp+=16;g_recomp_zf=g_esp==0;g_esp+=4;}
static void commands_pause(void){
    uint32_t frame=g_seh_ebp;g_esp-=8;PUSH32(g_esp,g_ebx);PUSH32(g_esp,frame);frame=MEM32(g_esp+20);g_ebx=MEM32(frame+28);PUSH32(g_esp,g_esi);PUSH32(g_esp,g_edi);g_edi=g_ecx;g_eax=MEM32(g_edi+8);g_ecx=MEM32(g_edi+12);MEM32(g_esp+16)=g_eax;SET_LO8(g_eax,MEM8(g_esp+32));frame+=28;MEM32(g_esp+20)=g_ecx;
    if(LO8(g_eax)){
        while(g_ebx!=frame){g_ecx=MEM32(g_edi+16);g_esi=g_ebx-28;g_ebx=MEM32(g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23ff8e);sub_00244F80();
            if(LO8(g_eax)){g_eax=MEM32(g_esi+8);g_edx=MEM32(g_esp+16);g_ecx=MEM32(g_esi+12);uint32_t borrow=g_eax<g_edx;g_eax-=g_edx;MEM32(g_esi+8)=g_eax;g_eax=MEM32(g_esp+20);g_ecx-=g_eax+borrow;MEM32(g_esi+12)=g_ecx;g_ecx=MEM32(g_edi+20);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23ffb3);sub_00244B80();}}
    }else{
        while(g_ebx!=frame){g_ecx=MEM32(g_edi+20);g_esi=g_ebx-28;g_ebx=MEM32(g_ebx);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23ffd3);sub_00245020();
            if(LO8(g_eax)){g_edx=MEM32(g_esi+8);g_ecx=MEM32(g_esp+16);uint32_t carry=(uint32_t)(((uint64_t)g_edx+g_ecx)>>32);g_edx+=g_ecx;g_ecx=MEM32(g_esi+12);MEM32(g_esi+8)=g_edx;g_edx=MEM32(g_esp+20);g_ecx+=g_edx+carry;MEM32(g_esi+12)=g_ecx;g_ecx=MEM32(g_edi+16);PUSH32(g_esp,g_esi);PUSH32(g_esp,0x23fff8);sub_00244AB0();}}
    }POP32(g_esp,g_edi);POP32(g_esp,g_esi);POP32(g_esp,frame);POP32(g_esp,g_ebx);g_esp+=8;g_recomp_zf=g_esp==0;g_esp+=12;
}
static void commands_destroy(void){PUSH32(g_esp,g_esi);g_esi=g_ecx;PUSH32(g_esp,0x24d208);sub_0024B530();if(MEM8(g_esp+8)&1){PUSH32(g_esp,g_esi);PUSH32(g_esp,0x24d215);sub_002D5CCD();g_esp+=4;}g_eax=g_esi;POP32(g_esp,g_esi);g_esp+=8;}
int uc_migration_audio_commands(uint32_t a){if(!commands_admit(a))return 0;switch(a){case 0x23fea0:commands_submit();break;case 0x241940:commands_drain();break;case 0x2449e0:commands_heap_pop();break;case 0x244ab0:commands_heap_push();break;case 0x244b80:commands_ready_push();break;case 0x244f80:commands_heap_remove();break;case 0x244fe0:commands_due();break;case 0x245020:commands_ready_remove();break;case 0x23ff00:commands_cancel();break;case 0x24b530:commands_unlink();break;case 0x24d200:commands_destroy();break;case 0x23ff50:commands_pause();break;}return 1;}
