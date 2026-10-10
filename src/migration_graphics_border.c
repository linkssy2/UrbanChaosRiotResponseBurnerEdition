#include "migration_graphics_border.h"
#include <stdlib.h>
#include <string.h>
int uc_migration_graphics_border_enabled(void){static int flag=-1;if(flag<0){const char*v=getenv("UCRR_NATIVE_GRAPHICS_BORDER");flag=v&&!strcmp(v,"1");}return flag;}
void uc_migration_graphics_border_color(uint32_t argb,float rgba[4]){
 rgba[0]=(float)((argb>>16)&255)/255.0f;rgba[1]=(float)((argb>>8)&255)/255.0f;rgba[2]=(float)(argb&255)/255.0f;rgba[3]=(float)(argb>>24)/255.0f;
}
