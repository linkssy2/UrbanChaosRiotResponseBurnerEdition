#include "audio_source_busy.h"
int uc_audio_source_busy(const UCAudioBusyInstance*v,size_t n,uint8_t*out){
 if(!out||n>4096||(n&&!v))return 1;
 for(size_t i=0;i<n;i++)if(!(v[i].flags&0x20)&&v[i].phase!=8){*out=1;return 0;}
 *out=0;return 0;
}
