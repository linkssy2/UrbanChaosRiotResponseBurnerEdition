/* Experimental specialization. Reuse the interpreter's arithmetic verbatim while
 * replacing its instruction fetch/decode loop with constant instruction blocks.
 * Cached runtime use remains opt-in pending live geometry/pacing acceptance. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static char *specialized_vertex_source(const char *source,const uint32_t p[136][4],unsigned start){
 const char *loop=strstr(source,"    [loop]for(uint pc=start_pc;pc<136;++pc){");
 const char *body=loop?strstr(loop,"        uint mac="):NULL;
 const char *end=body?strstr(body,"\n    }\n    return false;"):NULL;
 if(!loop||!body||!end)return NULL;
 size_t prefix=(size_t)(loop-source),body_len=(size_t)(end-body);
 size_t capacity=strlen(source)+136*(body_len+160)+1;
 char *result=malloc(capacity);if(!result)return NULL;
 memcpy(result,source,prefix);size_t used=prefix;
 for(unsigned pc=start;pc<136;++pc){
  int n=snprintf(result+used,capacity-used,"    { const uint4 w=uint4(0x%08Xu,0x%08Xu,0x%08Xu,0x%08Xu);\n",p[pc][0],p[pc][1],p[pc][2],p[pc][3]);
  if(n<0||(size_t)n>=capacity-used){free(result);return NULL;}used+=(size_t)n;
  memcpy(result+used,body,body_len);used+=body_len;
  memcpy(result+used,"\n    }\n",7);used+=7;
  if(p[pc][3]&1)break;
 }
 /* Drop only the loop's closing brace; retain failure and both entrypoints. */
 strcpy(result+used,end+6);
 return result;
}
