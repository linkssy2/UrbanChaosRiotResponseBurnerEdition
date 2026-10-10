#ifndef UCRR_PC_BUTTON_PROMPTS_H
#define UCRR_PC_BUTTON_PROMPTS_H
/* Original font encoding is established by Font_Buttons_Xbox.shader and its
 * 32x1024 atlas, not by ordinary A/B/X/Y text. See button_prompt_audit evidence.
 * Include after resource_readable and the recompilation register definitions. */
#include "../third_party/xboxrecomp/include/host_controller.h"

#define UCRR_PROMPT_CAPACITY 512u
#define UCRR_PROMPT_STACK_BYTES 0x500u
#define UCRR_PROMPT_FRAME_HEADROOM 0x1000u
extern size_t g_xbox_total_ram;
static int ucrr_pc_prompt_extent(uint32_t address,uint32_t bytes){
 return address>=0x10000u&&(size_t)address<g_xbox_total_ram&&
  (size_t)bytes<=g_xbox_total_ram-(size_t)address;
}

static int ucrr_pc_prompt_action(unsigned byte_value){
 switch(byte_value){
 case 0x02:return 0;  /* A: reload / interact / menu confirm */
 case 0x03:return 3;  /* Y: thrown weapons */
 case 0x04:return 2;  /* X: firearms */
 case 0x05:return 1;  /* B: melee / menu back */
 case 0x07:return 7;  /* right trigger */
 case 0x08:return 4;  /* Black: alternate action */
 case 0x09:return 6;  /* left trigger */
 case 0x0a:return 5;  /* White: thermal breather */
 case 0x0b:return 9;  /* Back: objectives */
 case 0x0c:return 8;  /* Start: pause */
 case 0x10:return 10; /* left-stick click: crouch */
 case 0x11:return 11; /* right-stick click: zoom */
 default:return -1;  /* bullet, composite controls, tools, 81 substitution */
 }
}

/* Return -1 for invalid/oversize input; 0 for unchanged; 1 for expanded.
 * A single algorithm feeds both original width and draw/layout operations. */
static int ucrr_pc_prompt_expand(const unsigned char*source,char*dest,unsigned capacity){
 unsigned at=0;int changed=0;
 if(!source||!dest||!capacity)return -1;
 /* Exact English text from newfrontend-pressstart-1 and -2. Other prose
  * containing START is not rewritten. Both width and draw see this template. */
 if(!strcmp((const char*)source,"PRESS START BUTTON")){source=(const unsigned char*)"PRESS \x0c TO START";changed=1;}
 for(unsigned i=0;i<UCRR_PROMPT_CAPACITY;++i){
  unsigned c=source[i];
  if(!c){dest[at]=0;return changed;}
  /* Native glyph bytes use the binding artwork supplied by the renderer. */
  if(at+1>=capacity)return -1;dest[at++]=(char)c;
 }
 return -1;
}

static int ucrr_pc_prompt_read(uint32_t address,unsigned char*dest){
 if(!address)return 0;
 for(unsigned i=0;i<UCRR_PROMPT_CAPACITY;++i){
  if(!ucrr_pc_prompt_extent(address,i+1))return 0;
  dest[i]=MEM8(address+i);if(!dest[i])return 1;
 }
 return 0;
}

/* Relocate only the by-value call arguments, keeping caller data untouched.
 * The generated function restores this reservation at EVERY return. Buffers
 * sit above its new entry ESP, so original locals/nested calls cannot alias.
 * 171A0 has 3 arguments; 18A70 has 6, including optional 81 substitution. */
uint32_t ucrr_pc_prompt_enter(unsigned parameter_count){
 if((parameter_count!=3&&parameter_count!=6)||!xbox_KeyboardMode())return 0;
 unsigned frame_bytes=(parameter_count+1)*4;
 /* Cover both the relocated argument/text area and original guest locals plus
  * known nested text calls below it; 18A70 alone needs 0x228 bytes. */
 unsigned required=UCRR_PROMPT_STACK_BYTES+UCRR_PROMPT_FRAME_HEADROOM;
 if(!ucrr_pc_prompt_extent(g_esp,frame_bytes)||g_esp<required||
    !ucrr_pc_prompt_extent(g_esp-required,required+frame_bytes))return 0;
 unsigned char original[UCRR_PROMPT_CAPACITY],dynamic_original[UCRR_PROMPT_CAPACITY];
 char expanded[UCRR_PROMPT_CAPACITY],dynamic_expanded[UCRR_PROMPT_CAPACITY];
 if(!ucrr_pc_prompt_read(MEM32(g_esp+4),original))return 0;
 int main_changed=ucrr_pc_prompt_expand(original,expanded,sizeof(expanded));
 if(main_changed<0)return 0;
 int dynamic_changed=0,has_dynamic=0;
 if(parameter_count==6&&strchr(expanded,(char)0x81)&&MEM32(g_esp+24)){
  has_dynamic=1;
  if(!ucrr_pc_prompt_read(MEM32(g_esp+24),dynamic_original))return 0;
  dynamic_changed=ucrr_pc_prompt_expand(dynamic_original,dynamic_expanded,sizeof(dynamic_expanded));
  if(dynamic_changed<0)return 0;
 }
 if(!main_changed&&!dynamic_changed)return 0;
 if(parameter_count==6){
  unsigned processed=0,dynamic_length=has_dynamic?(unsigned)strlen(dynamic_expanded):0;
  for(unsigned i=0;expanded[i];++i){processed+=(unsigned char)expanded[i]==0x81?dynamic_length:1;
   if(processed>=UCRR_PROMPT_CAPACITY)return 0;}
 }
 uint32_t previous=g_esp,next=previous-UCRR_PROMPT_STACK_BYTES;
 uint32_t primary_pointer=MEM32(previous+4),dynamic_pointer=has_dynamic?MEM32(previous+24):0;
 if((primary_pointer<previous&&primary_pointer+strlen((const char*)original)+1>next)||
    (has_dynamic&&dynamic_pointer<previous&&dynamic_pointer+strlen((const char*)dynamic_original)+1>next))return 0;
 for(unsigned i=0;i<frame_bytes;i+=4)MEM32(next+i)=MEM32(previous+i);
 for(unsigned i=0;;++i){MEM8(next+0x40+i)=(unsigned char)expanded[i];if(!expanded[i])break;}
 MEM32(next+4)=next+0x40;
 if(has_dynamic){
  for(unsigned i=0;;++i){MEM8(next+0x240+i)=(unsigned char)dynamic_expanded[i];if(!dynamic_expanded[i])break;}
  MEM32(next+24)=next+0x240;
 }
 g_esp=next;return UCRR_PROMPT_STACK_BYTES;
}
#endif
