void ucrr_failure_loop_probe(unsigned iteration,unsigned phase);
static LONG CALLBACK failure_probe_exception(PEXCEPTION_POINTERS ep) {
 static LONG busy;
 if(ep->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION&&!InterlockedExchange(&busy,1))ucrr_failure_loop_probe(256,0);
 return EXCEPTION_CONTINUE_SEARCH;
}
/* Opt-in, bounded diagnostic only. Does not change guest state or terminate loops. */
static uint32_t failure_probe_word(uint32_t address) {
 if(address<0x10000u||(uint64_t)address+4>g_xbox_total_ram)return 0;
 return MEM32(address);
}
void ucrr_failure_loop_probe(unsigned iteration,unsigned phase) {
 typedef struct {unsigned iteration,phase;uint32_t esp,owner,item,ecx,eax,edx,ebx,words[16],stack[12];} Record;
 static Record ring[64];static unsigned written,used;static int enabled=-1;static unsigned records;static FILE *out;
 if(enabled<0){enabled=getenv("UCRR_FAILURE_LOOP_PROBE")!=NULL;}
 if(!enabled||records>=128)return;
 Record *r=&ring[written++%64];if(used<64)++used;
 r->iteration=iteration;r->phase=phase;r->esp=g_esp;r->owner=g_esi;r->item=g_edi;r->ecx=g_ecx;r->eax=g_eax;r->edx=g_edx;r->ebx=g_ebx;
 for(unsigned j=0;j<16;++j)r->words[j]=failure_probe_word(g_esi+0x80+j*4);
 for(unsigned j=0;j<12;++j)r->stack[j]=failure_probe_word(g_esp+j*4);
 if(!out&&!(phase>0x5CF30&&phase<0x5D4A2&&g_ebx<0x10000u)&&(phase>2||(iteration<256&&g_esi>=0x10000u&&(uint64_t)g_esi+0x110<=g_xbox_total_ram)))return;
 if(!out){char name[96];snprintf(name,sizeof(name),"failure-loop-%lu.txt",GetCurrentProcessId());out=fopen(name,"wx");if(!out){enabled=0;return;}}
 unsigned begin=records?written-1:written-used;
 for(unsigned at=begin;at<written&&records<128;++at){Record *v=&ring[at%64];
 fprintf(out,"record=%u iteration=%u phase=%u esp=%08X esi=%08X edi=%08X ecx=%08X eax=%08X edx=%08X ebx=%08X\n",records++,v->iteration,v->phase,v->esp,v->owner,v->item,v->ecx,v->eax,v->edx,v->ebx);
 fprintf(out,"owner+80:");for(unsigned j=0;j<16;++j)fprintf(out," %08X",v->words[j]);
 fprintf(out,"\nstack:");for(unsigned j=0;j<12;++j)fprintf(out," %08X",v->stack[j]);fputc('\n',out);}
 fflush(out);
}
