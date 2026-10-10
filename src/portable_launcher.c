#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "cpu_headroom_launcher.h"
/* A real GUI executable: no PowerShell, source-tree path or console required.
 * Only the preserved performance preset is enabled; experiments are cleared. */
static const wchar_t *preset[]={
#ifdef UCRR_PACKAGE_CPU20
 L"UCRR_CPU_METADATA",L"UCRR_CPU_STATE_PACKET",L"UCRR_CPU_CAPTURE_CORE",
 L"UCRR_CPU_BULK_STATE",L"UCRR_CPU_CALL_CACHE",L"UCRR_PACKED_PACKET",
 L"UCRR_BOUNDS_LOCAL",L"UCRR_PACKED_SPAN",L"UCRR_PACKED_PROGRESS",
 L"UCRR_INLINE_PREPARED",L"UCRR_EXTREME_PACKET",L"UCRR_CPU_ALU_BUNDLE",L"UCRR_CPU_VP_DECODE",
 L"UCRR_PACKET_LAYOUT_KERNEL",
 L"UCRR_COMMAND_BLOCK",
#ifdef UCRR_PACKAGE_RAWGPU
 L"UCRR_RAW_GPU_PACKETS",
#ifndef UCRR_PACKAGE_AUDIT
 L"UCRR_GEOMETRY_RESIDENT",
#endif
 L"UCRR_ASYNC_VERTEX_COMPILE",
 L"UCRR_GPU_LIGHT_CONVERT",
 L"UCRR_RAW_DRAW_BATCH",
 L"UCRR_STATE_PROGRAM",
 L"UCRR_STATE_WIDE",
 L"UCRR_TEXTURE_REVISIONS",
#else
 L"UCRR_STATE_WIDE",L"UCRR_STATE_SEQUENCE",L"UCRR_STATE_CONSTANTS",
#endif
#endif
#ifdef UCRR_PACKAGE_RETAINED18
 L"UCRR_FIRST_VERTEX_CAPTURE",L"UCRR_PACKED_REUSE",L"UCRR_LIGHT_WRITE_SKIP",
 L"UCRR_LIGHT_WRITE_WATCH",L"UCRR_LIGHT_WATCH_VOLUME",L"UCRR_PACKED_REUSE_LEAN",
 L"UCRR_TEXTURE_LOOKUP_ADAPTIVE",L"UCRR_SAMPLER_CACHE",L"UCRR_INDEX_SCALAR_FAST_ROUTE",
 L"UCRR_INDEX_NONFORWARD_BATCH",L"UCRR_PIXEL_CONSTANT_KEY",L"UCRR_VERTEX_SHADER_HINT",
 L"UCRR_SELECTIVE_PROGRAM_COPY",L"UCRR_RAW_COMBINER_KEY",L"UCRR_BLEND_RESIDENT",
 L"UCRR_DEPTH_INPUT_KEY",L"UCRR_SAMPLER_STAGE_BATCH",L"UCRR_SAMPLER_INPUT_KEY",L"UCRR_RASTER_INPUT_KEY",
#endif
 L"UCRR_INDEX_BATCH",L"UCRR_COARSE_TIMING",L"UCRR_TEXTURE_CACHE_EXPANDED",
 L"UCRR_JOINT_QUERY_SUBMIT",L"UCRR_AUDIO_SPATIAL_STEREO",L"UCRR_TARGET_BINDING_CACHE",
 L"UCRR_CAPTURE_DIRECT",L"UCRR_LIGHT_AVX512",L"UCRR_QUERY_BATCH_FIXUPS",L"UCRR_LIGHTING_TEXTURES",
 L"UCRR_STENCIL_STATE",L"UCRR_SAMPLER_BINDING_CACHE",L"UCRR_SURFACE_CACHE",L"UCRR_TARGET_60",
 L"UCRR_PACKED_VERTICES",L"UCRR_VERTEX_ATTRIBUTE_CACHE",L"UCRR_BUFFERED_LOGS",L"UCRR_RAW_COMBINERS",
 L"UCRR_GPU_VERTICES",L"UCRR_MULTI_TEXTURES",L"UCRR_VIEWPORT_BINDING_CACHE",L"UCRR_QUERY_REUSE_BATCH",
 L"UCRR_VERTEX_STREAM",L"UCRR_PIXEL_SHADER_CACHE",L"UCRR_VERTEX_DMA_CACHE",L"UCRR_AUDIO_BRIDGE",
 L"UCRR_ORDERED_GPU",L"UCRR_LIGHT_AVX2",L"UCRR_VOICE_RETIRE_SERVICE",L"UCRR_PACKED_FAST_ENTRY",
 L"UCRR_DEPTH_REINTERPRET",L"UCRR_CONSTANT_BATCH",L"UCRR_GPU_VERTEX_RELATIVE",L"UCRR_VERTEX_FETCH_PLAN",
 L"UCRR_INPUT_TRACE", /* Existing input-delivery semantic hook must stay active. */
 L"UCRR_INLINE_ARRAYS",L"UCRR_LIGHT_INTERVAL_CACHE",L"UCRR_FLAT_DISPATCH",L"UCRR_SPECIALIZED_VERTICES",
 L"UCRR_QUERY_ADMISSION_BATCH",L"UCRR_TEXTURE_SOURCE_COPIES",L"UCRR_PFB_IMMEDIATE",L"UCRR_INLINE_BATCH"
};
#include "diagnostic_launcher.h"
static int error_message(const wchar_t*message){MessageBoxW(NULL,message,L"Urban Chaos - Recompiled",MB_OK|MB_ICONERROR);return 1;}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show){
 (void)instance;(void)previous;(void)command;(void)show;
 HANDLE error_output=INVALID_HANDLE_VALUE;
#ifdef UCRR_PACKAGE_RAWGPU
 /* Explicit crash reproduction only; ordinary launches remain log-free.
  * Keep the exact performance preset: broad diagnostics disable raw capture. */
 wchar_t error_path[MAX_PATH]={0};
 int capture_errors=command&&!wcscmp(command,L"--capture-errors");
 int command_probe=command&&!wcscmp(command,L"--command-probe");
 int zero_run=command&&!wcscmp(command,L"--zero-run");
 int lean_nop=command&&!wcscmp(command,L"--lean-nop");
 int leaf_trace=command&&!wcscmp(command,L"--native-leaf-trace");
 UCCpuHeadroomMode headroom;
#ifdef UCRR_PACKAGE_AUDIT
 /* Residency is an unproved performance policy, not a required GPU path.
  * Keep an explicit same-binary comparison; never inherit it accidentally. */
 int audit_residency=command&&!wcscmp(command,L"--with-residency");
 if(command&&*command&&!audit_residency&&wcscmp(command,L"--accumulated"))
  return error_message(L"This lean candidate accepts no arguments or --with-residency.");
 command=L"--accumulated";
#elif defined(UCRR_PACKAGE_CPU_HEADROOM)
 if(!command||!*command)command=L"--cpu-headroom";
#endif
 if(uc_cpu_headroom_parse(command,&headroom)<0)return error_message(L"Unknown CPU headroom test mode.");
 int query_kick_counts=command&&(!wcscmp(command,L"--query-kick-batch-frames")||!wcscmp(command,L"--query-kick-control-frames"));
 int query_kick_frames=query_kick_counts||(command&&(!wcscmp(command,L"--query-kick-batch-timing")||!wcscmp(command,L"--query-kick-control-timing")));
 int query_kick=command&&(!wcscmp(command,L"--query-kick-batch")||!wcscmp(command,L"--query-kick-batch-frames")||!wcscmp(command,L"--query-kick-batch-timing"));
 int index_cache_counts=headroom.counts||query_kick_counts||(command&&(!wcscmp(command,L"--index-cache-frames")||!wcscmp(command,L"--index-cache-control-frames")));
 int index_cache_frames=headroom.frames||query_kick_frames||index_cache_counts||(command&&(!wcscmp(command,L"--index-cache-timing")||!wcscmp(command,L"--index-cache-control-timing")));
 /* Both query A/B modes retain the entire index-cache/ownership base. */
 int index_cache=headroom.active||query_kick||query_kick_frames||(command&&(!wcscmp(command,L"--index-cache")||!wcscmp(command,L"--index-cache-frames")||!wcscmp(command,L"--index-cache-timing")));
 int ownership_counts=index_cache_counts||(command&&(!wcscmp(command,L"--ownership-batch-frames")||!wcscmp(command,L"--ownership-control-frames")));
 int ownership_frames=index_cache_frames||ownership_counts||(command&&(!wcscmp(command,L"--ownership-batch-timing")||!wcscmp(command,L"--ownership-control-timing")));
 /* The previous ownership additions stay on in BOTH new index-cache modes. */
 int ownership=index_cache||index_cache_frames||(command&&(!wcscmp(command,L"--ownership-batch")||!wcscmp(command,L"--ownership-batch-frames")||!wcscmp(command,L"--ownership-batch-timing")));
 int raw_retain_counts=ownership_counts||(command&&(!wcscmp(command,L"--raw-retain-batch-frames")||!wcscmp(command,L"--raw-retain-control-frames")));
 int raw_retain_frames=ownership_frames||raw_retain_counts||(command&&(!wcscmp(command,L"--raw-retain-batch-timing")||!wcscmp(command,L"--raw-retain-control-timing")));
 int raw_retain=ownership||ownership_frames||(command&&(!wcscmp(command,L"--raw-retain-batch")||!wcscmp(command,L"--raw-retain-batch-frames")||!wcscmp(command,L"--raw-retain-batch-timing")));
 int raw_strip_counts=raw_retain_counts||(command&&(!wcscmp(command,L"--raw-strip-batch-frames")||!wcscmp(command,L"--raw-strip-control-frames")));
 int raw_strip_frames=raw_retain_frames||raw_strip_counts||(command&&(!wcscmp(command,L"--raw-strip-batch-timing")||!wcscmp(command,L"--raw-strip-control-timing")));
 int raw_strip=raw_retain||raw_retain_frames||(command&&(!wcscmp(command,L"--raw-strip-batch")||!wcscmp(command,L"--raw-strip-batch-frames")||!wcscmp(command,L"--raw-strip-batch-timing")));
 int raw_nop_counts=command&&(!wcscmp(command,L"--raw-nop-batch-frames")||!wcscmp(command,L"--raw-nop-control-frames"));
 int raw_nop_frames=raw_nop_counts||(command&&(!wcscmp(command,L"--raw-nop-batch-timing")||!wcscmp(command,L"--raw-nop-control-timing")));
 /* Strip control and candidate retain the same zero-NOP policy; only strip
  * eligibility differs. Counters are omitted from the clean timing pair. */
 int raw_nop=raw_strip||raw_strip_frames||(command&&(!wcscmp(command,L"--raw-nop-batch")||!wcscmp(command,L"--raw-nop-batch-frames")||!wcscmp(command,L"--raw-nop-batch-timing")));
 int frame_capture=raw_strip_frames||raw_nop_frames||leaf_trace||(command&&(!wcscmp(command,L"--native-leaf-frames")||!wcscmp(command,L"--packet-cursor-frames")));
 int packet_cursor=command&&(!wcscmp(command,L"--packet-cursor")||!wcscmp(command,L"--packet-cursor-frames"));
 int native_leaf=raw_nop||raw_nop_frames||leaf_trace||(command&&(!wcscmp(command,L"--native-leaf")||!wcscmp(command,L"--native-leaf-frames")));
#endif
 wchar_t root[MAX_PATH],exe[MAX_PATH],xbe[MAX_PATH],cache[MAX_PATH],cmd[MAX_PATH+4];
 DWORD n=GetModuleFileNameW(NULL,root,MAX_PATH);if(!n||n>=MAX_PATH)return error_message(L"The folder path is too long.");
 wchar_t*slash=wcsrchr(root,L'\\');if(!slash)return 1;*slash=0;
 if(wcslen(root)>MAX_PATH-80)return error_message(L"Move the game to a shorter folder path.");
#ifdef UCRR_PACKAGE_AUDIT
 swprintf(exe,MAX_PATH,L"%s\\runtime-audit\\urban_chaos_riot_response_recompiled.exe",root);
#elif defined(UCRR_PACKAGE_RAWGPU)
 swprintf(exe,MAX_PATH,L"%s\\runtime-gpu\\urban_chaos_riot_response_recompiled.exe",root);
#else
 swprintf(exe,MAX_PATH,L"%s\\runtime\\urban_chaos_riot_response_recompiled.exe",root);
#endif
 swprintf(xbe,MAX_PATH,L"%s\\game_files\\Default.xbe",root);
 if(GetFileAttributesW(exe)==INVALID_FILE_ATTRIBUTES||GetFileAttributesW(xbe)==INVALID_FILE_ATTRIBUTES)
  return error_message(L"The runtime or original game files are missing. Keep the whole game folder together; do not move only this EXE.");
 HANDLE mutex=CreateMutexW(NULL,FALSE,L"Local\\UrbanChaosPortableLauncher");
 if(!mutex)return error_message(L"Could not initialize the launcher.");
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);return error_message(L"This portable build is already running.");}
#ifdef UCRR_PACKAGE_DIAGNOSTICS
 /* Preserve only this diagnostic request across the preset environment reset. */
 wchar_t residual_request[2]={0};
 int residual_requested=GetEnvironmentVariableW(L"UCRR_RESIDUAL_CAPTURE",residual_request,2)==1&&residual_request[0]==L'1';
 wchar_t candidate_request[2]={0};
 int candidate_requested=GetEnvironmentVariableW(L"UCRR_SLOWDOWN_CANDIDATE",candidate_request,2)==1&&candidate_request[0]==L'1';
 wchar_t cpu_request[2]={0};
 int cpu_requested=GetEnvironmentVariableW(L"UCRR_CPU_BUNDLE",cpu_request,2)==1&&cpu_request[0]==L'1';
 wchar_t extreme_request[2]={0};
 int extreme_requested=GetEnvironmentVariableW(L"UCRR_EXTREME_PACKET",extreme_request,2)==1&&extreme_request[0]==L'1';
#endif
 LPWCH environment=GetEnvironmentStringsW();
 if(!environment){CloseHandle(mutex);return 1;}
 for(wchar_t*p=environment;*p;p+=wcslen(p)+1)if(!_wcsnicmp(p,L"UCRR_",5)){
  wchar_t name[256];wchar_t*equal=wcschr(p,L'=');if(equal&&(size_t)(equal-p)<256){wcsncpy(name,p,equal-p);name[equal-p]=0;SetEnvironmentVariableW(name,NULL);}
 }
 FreeEnvironmentStringsW(environment);
#ifdef UCRR_PACKAGE_RAWGPU
 if(headroom.active)for(unsigned i=0;i<7;i++)if(headroom.mask&(1u<<i))SetEnvironmentVariableW(uc_cpu_headroom_flags[i],L"1");
 if(headroom.prepared_raw)SetEnvironmentVariableW(L"UCRR_PREPARED_RAW_COMBINER",L"1");
 if(headroom.host_upload_counts)SetEnvironmentVariableW(L"UCRR_HOST_UPLOAD_COVERAGE",L"1");
 if(headroom.accumulated)for(unsigned i=0;i<sizeof(uc_accumulated_flags)/sizeof(uc_accumulated_flags[0]);++i)
  SetEnvironmentVariableW(uc_accumulated_flags[i],L"1");
 if(command_probe)SetEnvironmentVariableW(L"UCRR_COMMAND_PROBE",L"1");
 if(zero_run)SetEnvironmentVariableW(L"UCRR_ZERO_RUN",L"1");
 if(lean_nop)SetEnvironmentVariableW(L"UCRR_LEAN_NOP",L"1");
 if(packet_cursor)SetEnvironmentVariableW(L"UCRR_PACKET_CURSOR",L"1");
 if(native_leaf){SetEnvironmentVariableW(L"UCRR_NATIVE_BOUNDS_LEAF",L"1");SetEnvironmentVariableW(L"UCRR_PACKET_CURSOR",L"1");}
 if(frame_capture)SetEnvironmentVariableW(L"UCRR_FRAME_CAPTURE",L"1");
 if(leaf_trace)SetEnvironmentVariableW(L"UCRR_LEAF_COVERAGE",L"1");
 if(raw_nop)SetEnvironmentVariableW(L"UCRR_RAW_NOP_BATCH",L"1");
 if(raw_strip)SetEnvironmentVariableW(L"UCRR_RAW_STRIP_BATCH",L"1");
 if(raw_retain)SetEnvironmentVariableW(L"UCRR_RAW_RETAIN_BATCH",L"1");
 if(index_cache)SetEnvironmentVariableW(L"UCRR_STRIP_INDEX_CACHE",L"1");
 if(query_kick)SetEnvironmentVariableW(L"UCRR_QUERY_KICK_BATCH",L"1");
 if(ownership){
  SetEnvironmentVariableW(L"UCRR_RAW_ARENA_TRANSFER",L"1");
  SetEnvironmentVariableW(L"UCRR_RAW_BOUNDS_ONCE",L"1");
  SetEnvironmentVariableW(L"UCRR_EQUAL_STATE_BATCH",L"1");
  SetEnvironmentVariableW(L"UCRR_TEXTURE_COPY_RECOVERY",L"1");
 }
 if(raw_nop_counts||raw_strip_counts)SetEnvironmentVariableW(L"UCRR_BATCH_COVERAGE",L"1");
#endif
#ifdef UCRR_PACKAGE_DIAGNOSTICS
 if(residual_requested)SetEnvironmentVariableW(L"UCRR_RESIDUAL_CAPTURE",L"1");
 if(candidate_requested)SetEnvironmentVariableW(L"UCRR_SLOWDOWN_CANDIDATE",L"1");
 if(cpu_requested)SetEnvironmentVariableW(L"UCRR_CPU_BUNDLE",L"1");
 if(extreme_requested)SetEnvironmentVariableW(L"UCRR_EXTREME_PACKET",L"1");
#endif
 for(unsigned i=0;i<sizeof(preset)/sizeof(preset[0]);++i)SetEnvironmentVariableW(preset[i],L"1");
#ifdef UCRR_PACKAGE_AUDIT
 if(audit_residency)SetEnvironmentVariableW(L"UCRR_GEOMETRY_RESIDENT",L"1");
#endif
#ifdef UCRR_PACKAGE_CPU20
 /* Quiet user test: no diagnostic_prepare, F8 recorder or inherited tracing.
  * INPUT_TRACE is a legacy input-delivery semantic hook, not this recorder. */
 SetEnvironmentVariableW(L"XBOX_LOG_PATH",L"NUL");
 SetEnvironmentVariableW(L"XBOX_LOG_LEVEL",L"0");
#endif
 SetEnvironmentVariableW(L"UCRR_DIAGNOSTIC_RAM_MB",L"128");
 swprintf(cache,MAX_PATH,L"%s\\ShaderCache",root);CreateDirectoryW(cache,NULL);SetEnvironmentVariableW(L"UCRR_SHADER_CACHE_DIR",cache);
 swprintf(cmd,MAX_PATH+4,L"\"%s\"",exe);
#ifdef UCRR_PACKAGE_DIAGNOSTICS
 if(!diagnostic_prepare(root,exe)){CloseHandle(mutex);return error_message(L"Could not create diagnostic logs and capture build/settings. Check folder access.");}
 wchar_t log_path[MAX_PATH];swprintf(log_path,MAX_PATH,L"%s\\runtime.log",diagnostic_session);
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE output=CreateFileW(log_path,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
#else
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE output=CreateFileW(L"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
#endif
 if(output==INVALID_HANDLE_VALUE){CloseHandle(mutex);return error_message(L"Could not initialize output handles.");}
 STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES;si.hStdInput=si.hStdOutput=si.hStdError=output;
#ifdef UCRR_PACKAGE_RAWGPU
 /* A hidden launcher/helper must not pass its show state into the game. */
 si.dwFlags|=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWNORMAL;
#endif
#ifdef UCRR_PACKAGE_RAWGPU
 if(capture_errors){
  swprintf(error_path,MAX_PATH,L"%s\\runtime-gpu\\crash-%lu-%llu.log",root,GetCurrentProcessId(),(unsigned long long)GetTickCount64());
  error_output=CreateFileW(error_path,GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
  if(error_output==INVALID_HANDLE_VALUE){CloseHandle(output);CloseHandle(mutex);return error_message(L"Could not retain crash output. No game was launched.");}
  si.hStdError=error_output;
 }
#endif
 PROCESS_INFORMATION process={0};BOOL ok=CreateProcessW(exe,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,root,&si,&process);
 DWORD launch_error=ok?0:GetLastError();CloseHandle(output);
 if(error_output!=INVALID_HANDLE_VALUE)CloseHandle(error_output);
 if(!ok){wchar_t message[160];swprintf(message,160,L"The game could not start (Windows error %lu).",launch_error);CloseHandle(mutex);return error_message(message);}
 CloseHandle(process.hThread);
 WaitForSingleObject(process.hProcess,INFINITE);DWORD code=0;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);CloseHandle(mutex);
#ifdef UCRR_PACKAGE_RAWGPU
 if(capture_errors){
  FILE*result=_wfopen(error_path,L"a");if(result){fprintf(result,"\n[LAUNCH-RESULT] pid=%lu exit=0x%08lX raw_gpu=1\n",process.dwProcessId,code);fclose(result);}
 }
#endif
#ifdef UCRR_PACKAGE_DIAGNOSTICS
 diagnostic_result(process.dwProcessId,code);
#endif
 if(code){wchar_t message[160];swprintf(message,160,L"The game closed unexpectedly (exit code 0x%08lX).",code);return error_message(message);}return 0;
}
