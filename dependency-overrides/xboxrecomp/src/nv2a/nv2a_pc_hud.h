#include "nv2a_pc_hud_art.h"
static unsigned pc_hud_stage[4];
static IDirect3DTexture8*pc_hud_texture[2];
static unsigned pc_hud_bindings[13];static char pc_hud_labels[13][64];
static long pc_hud_dx,pc_hud_dy;static unsigned pc_hud_direction;
static int pc_hud_held;
static ULONGLONG pc_hud_refresh[2];
static unsigned pc_hud_match(unsigned w,unsigned h,const void*source,size_t size){
 if(w==32&&h==1024&&size>=sizeof(pc_hud_buttons_source)&&!memcmp(source,pc_hud_buttons_source,sizeof(pc_hud_buttons_source)))return 1;
 if(w==64&&h==64&&size>=sizeof(pc_hud_orders_source)&&!memcmp(source,pc_hud_orders_source,sizeof(pc_hud_orders_source)))return 2;
 return 0;
}
static void pc_hud_shutdown(void){for(unsigned i=0;i<2;++i)if(pc_hud_texture[i]){pc_hud_texture[i]->lpVtbl->Release(pc_hud_texture[i]);pc_hud_texture[i]=NULL;}}
static IDirect3DTexture8*pc_hud_replace(unsigned id,IDirect3DTexture8*original){
 if(!id||!xbox_KeyboardMode())return original;
 long dx=0,dy=0;unsigned direction=0;int held=0;
 if(id==2)held=xbox_MouseOrdersFeedback(&dx,&dy,&direction);
 ULONGLONG now=GetTickCount64();unsigned slot=id-1;
 if(pc_hud_texture[slot]&&now-pc_hud_refresh[slot]<33&&(id!=2||held==pc_hud_held))return pc_hud_texture[slot];
 pc_hud_refresh[slot]=now;int changed=!pc_hud_texture[slot];
 if(id==1){for(unsigned a=0;a<12;++a){unsigned b=xbox_KeyboardBinding(a,0);if(!b)b=xbox_KeyboardBinding(a,1);char label[64];xbox_KeyboardActionLabel(a,label,sizeof(label));if(b!=pc_hud_bindings[a]||strcmp(label,pc_hud_labels[a]))changed=1;pc_hud_bindings[a]=b;strcpy(pc_hud_labels[a],label);}}
 else{unsigned b=xbox_KeyboardBinding(24,0);if(!b)b=xbox_KeyboardBinding(24,1);char label[64];xbox_KeyboardActionLabel(24,label,sizeof(label));
  if(b!=pc_hud_bindings[12]||strcmp(label,pc_hud_labels[12]))changed=1;pc_hud_bindings[12]=b;strcpy(pc_hud_labels[12],label);
  dx=dx/2*2;dy=dy/2*2;if(dx!=pc_hud_dx||dy!=pc_hud_dy||direction!=pc_hud_direction||held!=pc_hud_held)changed=1;pc_hud_dx=dx;pc_hud_dy=dy;pc_hud_direction=direction;pc_hud_held=held;}
 if(!changed)return pc_hud_texture[slot];
 unsigned w,h;uint32_t*p=pc_hud_pixels(id,pc_hud_bindings,pc_hud_labels,dx,dy,direction,held,&w,&h);if(!p)return id==2?NULL:original;
 IDirect3DDevice8*dev=xbox_GetD3DDevice();if(!dev){free(p);return id==2?NULL:original;}
 if(!pc_hud_texture[slot]&&FAILED(dev->lpVtbl->CreateTexture(dev,w,h,1,0,D3DFMT_LIN_A8R8G8B8,D3DPOOL_DEFAULT,&pc_hud_texture[slot]))){free(p);return id==2?NULL:original;}
 D3DLOCKED_RECT lr={0};HRESULT hr=pc_hud_texture[slot]->lpVtbl->LockRect(pc_hud_texture[slot],0,&lr,NULL,0);
 if(SUCCEEDED(hr)&&lr.pBits){for(unsigned y=0;y<h;++y)memcpy((char*)lr.pBits+(size_t)y*lr.Pitch,p+(size_t)y*w,w*4);hr=pc_hud_texture[slot]->lpVtbl->UnlockRect(pc_hud_texture[slot],0);}else hr=E_FAIL;
 free(p);if(FAILED(hr)){pc_hud_texture[slot]->lpVtbl->Release(pc_hud_texture[slot]);pc_hud_texture[slot]=NULL;return id==2?NULL:original;}return pc_hud_texture[slot];
}
