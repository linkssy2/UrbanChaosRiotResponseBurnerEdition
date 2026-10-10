#include "migration_interface_missionlabels.h"
#include "recomp_types.h"
#include "xbox_memory_layout.h"
#include <stdlib.h>
extern void sub_001F92B0(void);
int migration_interface_missionlabels(unsigned op){
 static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_NATIVE_INTERFACE_MISSIONLABELS");enabled=s&&!strcmp(s,"1");}if(!enabled||op>1)return 0;
 unsigned s=g_esp;if(s<0x10800||(s&3)||(uint64_t)s+32>g_xbox_total_ram)return 0;if(op){unsigned a=MEM32(s+4),b=MEM32(s+8);if(a==0xffffffff?b>6:a>11)return 0;}
 static const unsigned labels[]={0x488328,0x488318,0x488308,0x4882f8,0x4882e8,0x4882d8,0x4882c8,0x4882b8,0x4882a8,0x488294,0x488280,0x48826c,0x488258,0x488244,0x488230,0x48821c,0x488208,0x4881f4,0x4881e0,0x4881cc,0x4881b8,0x4881a4,0x488190};
 static const unsigned label_ret[]={0x26eac1,0x26ead0,0x26eadf,0x26eaee,0x26eafd,0x26eb0c,0x26eb1b,0x26eb2a,0x26eb3c,0x26eb4b,0x26eb5a,0x26eb69,0x26eb78,0x26eb87,0x26eb96,0x26eba5,0x26ebb7,0x26ebc6,0x26ebd5,0x26ebe4,0x26ebf3,0x26ec02,0x26ec14};
 static const unsigned thumbs[]={0x4884dc,0x4884c4,0x4884ac,0x488494,0x48847c,0x488464,0x48844c,0x488434,0x48841c,0x488400,0x4883e4,0x4883c8,0x4883ac,0x488390,0x488374,0x488358,0x48833c};
 static const unsigned thumb_ret[]={0x26ec71,0x26ec80,0x26ec8f,0x26ec9e,0x26ecad,0x26ecbc,0x26eccb,0x26ecda,0x26ecec,0x26ecfb,0x26ed0a,0x26ed19,0x26ed28,0x26ed37,0x26ed46,0x26ed55,0x26ed67};
 unsigned bytes=op?68:92;g_esp-=bytes;unsigned base=g_esp,count=op?17:23;
 for(unsigned i=0;i<count;i++){
  if(i==8||i==16)g_esp+=64;
  unsigned *reg=i%3==0?&g_eax:i%3==1?&g_ecx:&g_edx;unsigned text=op?thumbs[i]:labels[i];
  if(i==19){PUSH32(g_esp,text);*reg=base+i*4;}else{*reg=base+i*4;PUSH32(g_esp,text);}PUSH32(g_esp,*reg);PUSH32(g_esp,op?thumb_ret[i]:label_ret[i]);sub_001F92B0();
 }
 g_eax=MEM32(g_esp+(op?80:152));g_esp+=op?8:56;
 if(op){if(g_eax==0xffffffff){g_edx=MEM32(g_esp+76);g_eax=MEM32(g_esp+g_edx*4+44);}else g_eax=MEM32(g_esp+g_eax*4);}
 else if(g_eax!=0xffffffff){g_eax=(int32_t)g_eax<0||(int32_t)g_eax>11?0:MEM32(g_esp+g_eax*4);}
 else{g_eax=MEM32(g_esp+100);if((int32_t)g_eax<0||(int32_t)g_eax>6)g_eax=0;else{SET_LO8(g_ecx,MEM8(g_esp+104));g_eax=MEM32(g_esp+g_eax*4+(LO8(g_ecx)?68:44));}}
 g_esp+=bytes;g_recomp_zf=!g_esp;g_esp+=4;return 1;
}
