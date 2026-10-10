/* Production input + original HUD visibility gate/container + current artwork.
 * Only raw OS input and final sprite virtual setters are fixture boundaries. */
#define main existing_input_checks
#include "../test_pc_input_backend.c"
#undef main
#undef CHECK
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);exit(1);}}while(0)
#include "support.inc"
#include "d3d/d3d8_xbox.h"
#include "nv2a_pc_hud.h"

static IDirect3DDevice8 device;
static IDirect3DTexture8 texture,controller_texture;
static uint32_t uploaded[256*256];
static int create_failure,lock_failure,upload_count;
static ULONG __stdcall texture_release(IDirect3DTexture8*t){CHECK(t==&texture);return 0;}
static HRESULT __stdcall texture_lock(IDirect3DTexture8*t,UINT level,D3DLOCKED_RECT*r,const RECT*rect,DWORD flags){
 CHECK(t==&texture&&!level&&!rect&&!flags);if(lock_failure)return E_FAIL;r->Pitch=256*4;r->pBits=uploaded;return S_OK;
}
static HRESULT __stdcall texture_unlock(IDirect3DTexture8*t,UINT level){CHECK(t==&texture&&!level);upload_count++;return S_OK;}
static HRESULT __stdcall create_texture(IDirect3DDevice8*d,UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,IDirect3DTexture8**out){
 CHECK(d==&device&&w==256&&h==256&&levels==1&&!usage&&format==D3DFMT_LIN_A8R8G8B8&&pool==D3DPOOL_DEFAULT);
 if(create_failure)return E_FAIL;*out=&texture;return S_OK;
}
IDirect3DDevice8*xbox_GetD3DDevice(void){return &device;}

enum {HUD=0x580000,HEAD=0x581000,NODE=0x581040,SPRITE=0x582000,VT=0x583000,PAD_ID=17};
static unsigned char *ram;
static unsigned setter_calls,color_calls,cases;
static float sprite_alpha;
static void sprite_call(uint32_t target){
    CHECK(g_ecx==SPRITE);
    if(target==1){uint32_t color=MEM32(g_esp+4);CHECK(MEMF(color)==1&&MEMF(color+4)==1&&MEMF(color+8)==1);color_calls++;g_esp+=8;return;}
    CHECK(target==2&&MEM32(g_esp+8)==1);sprite_alpha=MEMF(g_esp+4);setter_calls++;g_esp+=12;
}
#undef RECOMP_ICALL_SAFE
#define RECOMP_ICALL_SAFE(target,saved) sprite_call(target)
static void sub_001F9010(void){CHECK(0);}
#include "hud.inc"

static void setup(unsigned available,uint32_t target){
    memset(ram,0,RAM_BYTES);MEMF(0x44BF94)=1;
    MEM32(HUD+0xBC)=HEAD;MEM32(HEAD)=NODE;MEM32(NODE)=HEAD;
    MEM32(NODE+8)=PAD_ID;MEM32(NODE+12)=SPRITE;MEM32(SPRITE)=VT;MEM32(VT+0x14)=1;MEM32(VT+0x18)=2;
    MEM8(HUD+0xD5)=(uint8_t)available;MEM32(HUD+0xD8)=target;MEM32(HUD+0xD0)=PAD_ID;
    MEM32(HUD+0xC8)=92; /* A different selected command must not be changed. */
    g_esi=HUD;g_esp=STACK;g_ebp=STACK+0x200;g_eax=0xAABBCCDD;g_ebx=0x1234;g_edi=0x2345;
    setter_calls=color_calls=0;sprite_alpha=-1;
}
static void check_gate(unsigned available,uint32_t target,int held){
    setup(available,target);reference_gate();CHECK(setter_calls==1&&sprite_alpha==(float)(available&&target==UINT32_MAX));
    setup(available,target);unsigned char old_hud[0x110];memcpy(old_hud,ram+HUD,sizeof old_hud);
    unsigned char old_gesture[sizeof mouse_orders];memcpy(old_gesture,&mouse_orders,sizeof old_gesture);
    uint32_t stack=g_esp;candidate_gate();
    int visible=available&&(target==UINT32_MAX||xbox_KeyboardMode());
    CHECK(setter_calls==1&&color_calls==(unsigned)visible&&sprite_alpha==(float)visible);
    CHECK(g_esp==stack&&g_esi==HUD&&g_ebx==0x1234&&g_edi==0x2345);
    CHECK(!memcmp(old_hud,ram+HUD,sizeof old_hud)&&!memcmp(old_gesture,&mouse_orders,sizeof old_gesture));cases++;
}
static void matrix(int held){
    for(unsigned available=0;available<3;++available){check_gate(available,UINT32_MAX,held);check_gate(available,0,held);check_gate(available,42,held);}
}
static void preview(const char *path){
    unsigned bindings[13]={0};char labels[13][64]={{0}};unsigned w,h;bindings[12]=VK_MBUTTON;
    uint32_t *pixels=pc_hud_pixels(2,bindings,labels,-60,0,4,1,&w,&h);CHECK(pixels&&w==256&&h==256);
    unsigned opaque=0;for(unsigned i=0;i<w*h;++i)if(pixels[i]){opaque++;CHECK((pixels[i]>>24)==255&&(pixels[i]&0xffffff));}
    CHECK(opaque>1000&&opaque<18000&&!pixels[50*w+180]&&!pixels[50*w+80]);
    BITMAPFILEHEADER file={0};BITMAPINFOHEADER info={0};file.bfType=0x4D42;file.bfOffBits=sizeof file+sizeof info;file.bfSize=file.bfOffBits+w*h*4;
    info.biSize=sizeof info;info.biWidth=w;info.biHeight=-(int)h;info.biPlanes=1;info.biBitCount=32;info.biSizeImage=w*h*4;
    FILE*f=fopen(path,"wb");CHECK(f);CHECK(fwrite(&file,sizeof file,1,f)==1&&fwrite(&info,sizeof info,1,f)==1&&fwrite(pixels,w*h*4,1,f)==1);fclose(f);free(pixels);
}
static void art_modes(void){
    IDirect3DDevice8Vtbl dv={0};IDirect3DTexture8Vtbl tv={0};dv.CreateTexture=create_texture;
    tv.Release=texture_release;tv.LockRect=texture_lock;tv.UnlockRect=texture_unlock;device.lpVtbl=&dv;texture.lpVtbl=&tv;
    reset_backend(0,0);pad_result[0]=ERROR_DEVICE_NOT_CONNECTED;xbox_MouseSetActive(TRUE);xbox_KeyboardNoteInput();
    keyboard_config.binding[24][0]=VK_MBUTTON;keyboard_config.binding[24][1]=0;
    for(unsigned a=0;a<24;++a)for(unsigned s=0;s<2;++s)if(keyboard_config.binding[a][s]==VK_MBUTTON)keyboard_config.binding[a][s]=0;
    unsigned char unrelated[4096]={0};CHECK(pc_hud_match(64,64,unrelated,sizeof unrelated)==0);
    create_failure=1;CHECK(!pc_hud_replace(2,&controller_texture));create_failure=0;
    CHECK(pc_hud_replace(2,&controller_texture)==&texture&&upload_count==1);
    uint32_t idle[256*256];memcpy(idle,uploaded,sizeof idle);
    xbox_KeyboardSourceEvent(VK_MBUTTON,1);xbox_MouseMotion(-60,0);
    CHECK(pc_hud_replace(2,&controller_texture)==&texture&&upload_count==2&&memcmp(idle,uploaded,sizeof idle));
    xbox_KeyboardSourceEvent(VK_MBUTTON,0);
    CHECK(pc_hud_replace(2,&controller_texture)==&texture&&upload_count==3&&!memcmp(idle,uploaded,sizeof idle));
    /* Physical controller ownership uses the original art immediately; a real
     * keyboard/mouse event takes it back, even while the controller stays in. */
    pad_result[0]=ERROR_SUCCESS;memset(&pad_state[0].Gamepad,0,sizeof pad_state[0].Gamepad);pad_state[0].Gamepad.wButtons=XINPUT_GAMEPAD_A;
    XBOX_INPUT_STATE pad;xbox_InputGetState(0,&pad);CHECK(!xbox_KeyboardMode()&&pc_hud_replace(2,&controller_texture)==&controller_texture);
    xbox_KeyboardSourceEvent(VK_MBUTTON,1);xbox_InputGetState(0,&pad);CHECK(xbox_KeyboardMode()&&pc_hud_replace(2,&controller_texture)==&texture);
    fake_ms+=34;keyboard_config.binding[24][0]='O';lock_failure=1;CHECK(!pc_hud_replace(2,&controller_texture));lock_failure=0;
    CHECK(pc_hud_replace(2,&controller_texture)==&texture&&memcmp(idle,uploaded,sizeof idle));
    pc_hud_shutdown();xbox_MouseSetActive(FALSE);
    puts("PASS production HUD texture mode: PC mouse/key art while idle/held/released, immediate physical-controller takeover/back, changed bindings, exact source texture match, allocation/upload failure never returns controller art");
}
int main(int argc,char**argv){
    CHECK(argc==2);CHECK(!order_gestures()&&!configurable_orders_and_wheel()&&!flashlight_rebinding()&&!prompt_handoff());
    ram=calloc(1,RAM_BYTES);CHECK(ram);g_xbox_mem_offset=(ptrdiff_t)ram;keyboard_init();
    UCRRKeyboardConfig saved=keyboard_config;
    const unsigned source[]={VK_MBUTTON,'O',VK_XBUTTON1};
    for(unsigned k=0;k<3;++k){
        reset_backend(0,0);pad_result[0]=ERROR_DEVICE_NOT_CONNECTED;xbox_MouseSetActive(TRUE);
        keyboard_config.binding[24][0]=source[k];keyboard_config.binding[24][1]=0;
        for(unsigned a=0;a<24;++a)for(unsigned s=0;s<2;++s)if(keyboard_config.binding[a][s]==source[k])keyboard_config.binding[a][s]=0;
        xbox_KeyboardNoteInput();matrix(0);xbox_KeyboardSourceEvent(source[k],1);matrix(1);
        xbox_MouseOrdersPublish();CHECK(mouse_orders.buttons==2);matrix(1);xbox_MouseOrdersPublish();CHECK(!mouse_orders.buttons);matrix(1);
        const LONG delta[][2]={{0,-60},{60,60},{-60,60},{-60,-60},{-60,0}};
        for(unsigned d=0;d<5;++d){xbox_MouseMotion(delta[d][0],delta[d][1]);matrix(1);}
        /* The HUD reads do not advance opener/neutral/selection publication. */
        CHECK(mouse_orders.held&&mouse_orders.phase==3&&!mouse_orders.buttons);
        xbox_KeyboardSourceEvent(source[k],0);matrix(0);xbox_MouseOrdersPublish();CHECK(mouse_orders.buttons==4);matrix(0);xbox_MouseOrdersPublish();CHECK(!mouse_orders.phase);
        /* Focus loss, capture loss, overlay blocking and controller takeover. */
        xbox_KeyboardSourceEvent(source[k],1);focused=(HWND)2;matrix(0);focused=(HWND)1;xbox_MouseSetActive(FALSE);matrix(0);xbox_MouseSetActive(TRUE);
        xbox_KeyboardSourceEvent(source[k],1);xbox_InputSetBlocked(TRUE);matrix(0);xbox_InputSetBlocked(FALSE);xbox_MouseSetActive(TRUE);
        xbox_KeyboardSourceEvent(source[k],1);InterlockedExchange(&keyboard_mode,0);matrix(0);xbox_MouseSetActive(FALSE);
    }
    art_modes();keyboard_config=saved;preview(argv[1]);free(ram);
    printf("PASS %u original gate-to-sprite cases: targeted healing/other orders expose mouse cue while available, before and during PC hold, rebound sources, original input publication, release/cancel/controller behavior, unchanged HUD command data and nonconsuming feedback; current in-game pad artwork rasterized\n",cases);
    return 0;
}
