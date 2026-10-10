/* Retain CONTROL's movie screen, skip delays, fades and successor callbacks.
 * Only its decoder/texture owner is replaced after a native open succeeds. */
#include "hybrid_frontend_guest.h"
#include "hybrid_guest_cpu.h"
#include <windows.h>

extern size_t g_xbox_total_ram;
enum { MOVIE=0x4EAAE8u };
static DWORD movie_thread;
static int movie_owned;
static int readable(uint32_t p,unsigned n){return p>=0x10000u&&(uint64_t)p+n<=g_xbox_total_ram;}
static int finite_float(float f){uint32_t u;memcpy(&u,&f,4);return (u&0x7FFFFFFFu)<0x7F800000u;}
/* 874C0 scales by16, truncates/clamps, then applies these hundredths of dB. */
static float movie_gain(float f){
    static const int db[16]={-10000,-6000,-5000,-4000,-3667,-3334,-3001,-2668,-2335,-2002,-1669,-1336,-1003,-670,-337,0};
    int i=f<=0?0:f>=0.9375f?15:(int)(f*16.0f);
    return powf(10.0f,(float)db[i]/2000.0f);
}
static int owner_empty(void){
    if(MEM8(MOVIE+0x68))return 0;
    if(MEM32(MOVIE)||MEM32(MOVIE+0x28)||MEM32(MOVIE+0x2C))return 0;
    for(unsigned i=0;i<8;i++)if(MEM32(MOVIE+0x40+i*4))return 0;
    return 1;
}
int uc_hybrid_frontend_movie(unsigned op){
    HybridCPU saved;save_cpu(&saved);
    int consumed=0;uint32_t ret_bytes=4;unsigned result=0;
    /* A failed host frontend revokes new opens, but an already-owned movie
     * must reach native EOF/close rather than enter the absent Xbox decoder. */
    if((!ucrr_native_frontend_enabled()&&!movie_owned)||!readable(MOVIE,0x6A)||!readable(g_esp,4))goto done;
    if(movie_thread&&movie_thread!=GetCurrentThreadId())goto done;
    uint32_t caller=MEM32(g_esp);
    if(op==0){
        if(movie_owned||g_ecx!=MOVIE||caller!=0x87B05||!readable(g_esp,28)||!owner_empty())goto done;
        uint32_t path=MEM32(g_esp+4);float volume=MEMF(g_esp+8);
        if(!readable(path,1)||!finite_float(volume)||MEM32(g_esp+12)!=0x24||!MEM32(g_esp+16))goto done;
        char name[1024];unsigned i;
        for(i=0;i<sizeof(name);i++){if(!readable(path+i,1))goto done;name[i]=(char)MEM8(path+i);if(!name[i])break;}
        if(!i||i==sizeof(name))goto done;
        int loop=!!MEM8(g_esp+20);
        if(!ucrr_native_frontend_movie_open(name,loop,movie_gain(volume)))goto done;
        movie_thread=GetCurrentThreadId();movie_owned=1;
        /* No guest resource handles are fabricated. Original status queries
         * see active/loop; original outer wrappers retain their own globals. */
        MEM32(MOVIE+0x30)=1;MEM32(MOVIE+0x34)=MEM32(MOVIE+0x38)=MEM32(MOVIE+0x3C)=0;
        MEM32(MOVIE+0x64)=0xFFFFFFFFu;MEM8(MOVIE+0x68)=1;MEM8(MOVIE+0x69)=(uint8_t)loop;
        ret_bytes=28;consumed=1;
    }else if(movie_owned){
        if(op==1&&g_ecx==MOVIE&&(caller==0x877ED||caller==0x87B30)){
            MEM8(MOVIE+0x68)=ucrr_native_frontend_movie_tick()?1:0;consumed=1;
        }else if(op==2&&caller==0x9EA5C){
            if(MEM8(MOVIE+0x68))ucrr_native_frontend_movie_draw();consumed=1;
        }else if(op==3&&g_ecx==MOVIE){
            ucrr_native_frontend_movie_close();movie_owned=0;
            memset((void*)XBOX_PTR(MOVIE),0,0x30);
            memset((void*)XBOX_PTR(MOVIE+0x40),0,0x20);
            MEM32(MOVIE+0x64)=0xFFFFFFFFu;MEM8(MOVIE+0x68)=MEM8(MOVIE+0x69)=0;consumed=1;
        }else if(op==4&&g_ecx==MOVIE&&readable(g_esp,8)){
            float volume=MEMF(g_esp+4);
            if(finite_float(volume))ucrr_native_frontend_movie_volume(movie_gain(volume));
            ret_bytes=8;consumed=1;
        }else if(op==5&&readable(g_esp,12)&&MEM32(g_esp+4)==0){
            uint32_t action=MEM32(g_esp+8);
            int screen=(caller==0x272C5C&&action==0x16)||(caller==0x272C6B&&action==0x1D);
            /* 271C00 builds the splash/gamelogo's separate Start/A widgets.
             * 276690 reaches this query only after its own delay expires;
             * its existing modal gate and callback dispatch still run. */
            int title=caller==0x276702&&readable(g_edi,0x20)&&MEM32(g_edi)==0x489974&&
                MEM32(g_edi+8)==0x2717B0&&MEM32(g_edi+4)==action&&(action==0x16||action==0x1D)&&
                MEM8(g_edi+0x14)&&finite_float(MEMF(g_edi+0x1C))&&MEMF(g_edi+0x1C)<=0;
            if((screen||title)&&ucrr_native_frontend_movie_take_skip()){result=1;consumed=1;}
        }
    }
done:
    restore_cpu(&saved);
    if(consumed){g_esp+=ret_bytes;g_eax=result;g_recomp_zf=!result;}
    return consumed;
}
