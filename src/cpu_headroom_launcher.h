#ifndef UC_CPU_HEADROOM_LAUNCHER_H
#define UC_CPU_HEADROOM_LAUNCHER_H
#include <wchar.h>
typedef struct UCCpuHeadroomMode {unsigned active,mask,frames,counts,prepared_raw,host_upload_counts,accumulated;}UCCpuHeadroomMode;
static const wchar_t*uc_cpu_headroom_names[]={L"visibility",L"raw-plan",L"dma-cert",L"arrays",L"state",L"copy",L"emitter"};
static const wchar_t*uc_cpu_headroom_flags[]={L"UCRR_CPU_BUNDLE_VISIBILITY",L"UCRR_CPU_BUNDLE_RAW_PLAN",L"UCRR_CPU_BUNDLE_DMA_CERT",L"UCRR_CPU_BUNDLE_ARRAYS",L"UCRR_CPU_BUNDLE_STATE",L"UCRR_CPU_BUNDLE_COPY",L"UCRR_CPU_BUNDLE_EMITTER"};
/* Only implemented, verified entries belong here. The twenty-entry research
 * list is not twenty active runtime changes. Both arms retain prepared-raw. */
static const wchar_t*uc_accumulated_flags[]={L"UCRR_CPU_COMPACT2",L"UCRR_RAW_COMPACT_DIRECTORY",L"UCRR_RAW_CONSTANT_DOMAIN",L"UCRR_COMPILER_READY_POLL",L"UCRR_COMBINER_ZERO_HASH",L"UCRR_RAW_RESIDENT_BYTE_BUDGET",L"UCRR_RAW_PACKET_VERTEX_PLAN"};
/* No inherited override: the launcher clears UCRR_* before applying this mask.
 * The rejected matrix candidate is deliberately not representable. */
static int uc_cpu_headroom_parse(const wchar_t*command,UCCpuHeadroomMode*out){
 *out=(UCCpuHeadroomMode){0};
 if(command&&!wcsncmp(command,L"--accumulated",13)){
  const wchar_t*tail=command+13;
  out->active=1;out->mask=127;out->prepared_raw=1;out->accumulated=1;
  if(!wcsncmp(tail,L"-control",8)){out->accumulated=0;tail+=8;}
  if(!*tail)return 1;
  if(!wcscmp(tail,L"-frames")){out->frames=out->counts=out->host_upload_counts=1;return 1;}
  if(!wcscmp(tail,L"-timing")){out->frames=1;return 1;}
  return -1;
 }
 /* Same seven-part base in both arms. The new switch covers immediate raw
  * combiner preparation only, not deferred rendering or a backend rewrite. */
 if(command&&!wcsncmp(command,L"--prepared-raw",14)){
  const wchar_t*tail=command+14;
  out->active=1;out->mask=127;out->prepared_raw=1;
  if(!wcsncmp(tail,L"-control",8)){out->prepared_raw=0;tail+=8;}
  if(!*tail)return 1;
  if(!wcscmp(tail,L"-frames")){out->frames=out->counts=out->host_upload_counts=1;return 1;}
  if(!wcscmp(tail,L"-timing")){out->frames=1;return 1;}
  return -1;
 }
 const wchar_t*prefix=L"--cpu-headroom";size_t n=wcslen(prefix);
 if(!command||wcsncmp(command,prefix,n))return 0;
 wchar_t tail[80];size_t length=wcslen(command+n);if(length>=80)return -1;
 wcscpy(tail,command+n);out->active=1;out->mask=127;
 if(length>=7&&!wcscmp(tail+length-7,L"-frames")){tail[length-7]=0;out->frames=out->counts=1;}
 else if(length>=7&&!wcscmp(tail+length-7,L"-timing")){tail[length-7]=0;out->frames=1;}
 if(!*tail)return 1;
 if(!wcscmp(tail,L"-control")){out->mask=0;return 1;}
 int without=!wcsncmp(tail,L"-without-",9),only=!wcsncmp(tail,L"-only-",6);
 if(without||only)for(unsigned i=0;i<7;i++)if(!wcscmp(tail+(without?9:6),uc_cpu_headroom_names[i])){
  out->mask=without?(127u^(1u<<i)):(1u<<i);return 1;
 }
 return -1;
}
#endif
