/* Original pixels are derived locally from the verified user disc, never embedded. */
#include <stdio.h>
#include <stdlib.h>
static uint32_t pc_hud_buttons_pixels[32*1024];
static int pc_hud_load_pixels(void){
 static int loaded;
 if(loaded)return 1;
 const char *path=getenv("UCRR_PC_HUD_PIXELS");if(!path||!*path)return 0;
 FILE *f=fopen(path,"rb");if(!f)return 0;
 size_t n=fread(pc_hud_buttons_pixels,1,sizeof(pc_hud_buttons_pixels),f);
 int end=fgetc(f);fclose(f);
 if(n!=sizeof(pc_hud_buttons_pixels)||end!=EOF)return 0;
 loaded=1;return 1;
}
static uint64_t pc_hud_source_hash(const void *source,size_t size){
 const unsigned char *p=(const unsigned char*)source;uint64_t h=1469598103934665603ull;
 for(size_t i=0;i<size;++i)h=(h^p[i])*1099511628211ull;
 return h;
}
