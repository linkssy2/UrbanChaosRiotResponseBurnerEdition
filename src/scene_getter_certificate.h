#ifndef UCRR_SCENE_GETTER_CERTIFICATE_H
#define UCRR_SCENE_GETTER_CERTIFICATE_H
/* Include after recomp_types.h. Published before guest execution; immutable
 * manual/generated dispatch mappings only, never guest vtables/resource data. */
extern unsigned g_uc_scene_getter_mask;
void uc_scene_getter_init(void);
static __forceinline void uc_scene_getter_call(unsigned mask,uint32_t va,
 uint32_t saved,uint32_t expected_va,unsigned bit,unsigned offset){
 if(va==expected_va&&(mask&bit)){
  recomp_icall_trace_record(va);
  RECOMP_ICALL_OBSERVE(va,RECOMP_ICALL_SEEN_RESOLVED);
  g_eax=MEM32(g_ecx+offset);g_esp+=4;
 }else{RECOMP_ICALL_SAFE(va,saved);}
}
#endif
