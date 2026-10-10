#ifdef UCRR_PACKAGE_DIAGNOSTICS
static wchar_t diagnostic_session[MAX_PATH];
static int diagnostic_prepare(const wchar_t*root,const wchar_t*exe){
 SYSTEMTIME t;GetLocalTime(&t);wchar_t logs[MAX_PATH],path[MAX_PATH],source[MAX_PATH];
 if(wcslen(root)>MAX_PATH-115)return 0;
 swprintf(logs,MAX_PATH,L"%s\\Logs",root);
 if(!CreateDirectoryW(logs,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 swprintf(diagnostic_session,MAX_PATH,L"%s\\%04u%02u%02u-%02u%02u%02u-%lu",logs,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
 if(!CreateDirectoryW(diagnostic_session,NULL))return 0;
 const wchar_t*flags[]={L"UCRR_HOSTAGE_TRACE",L"UCRR_TIMING_TRACE",L"UCRR_RENDER_PROFILE",L"UCRR_MARK_FRAME_COST"};
 SetEnvironmentVariableW(L"UCRR_PACKED_HIT_FAST",NULL);
 SetEnvironmentVariableW(L"UCRR_RENDER_PROFILE_VERTEX_EXACT",NULL);
 for(unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);++i)SetEnvironmentVariableW(flags[i],L"1");
 /* Stack-capture mode retains F8 ledger but avoids unrelated script and
  * per-draw profiler traffic. Do not enable performance candidates here. */
 int residual=GetEnvironmentVariableW(L"UCRR_RESIDUAL_CAPTURE",NULL,0)!=0;
 if(residual){SetEnvironmentVariableW(L"UCRR_HOSTAGE_TRACE",NULL);SetEnvironmentVariableW(L"UCRR_RENDER_PROFILE",NULL);SetEnvironmentVariableW(L"UCRR_PACKED_PACKET",NULL);}
 int candidate=residual&&GetEnvironmentVariableW(L"UCRR_SLOWDOWN_CANDIDATE",NULL,0)!=0;
 if(candidate){SetEnvironmentVariableW(L"UCRR_PACKED_PACKET",L"1");SetEnvironmentVariableW(L"UCRR_BOUNDS_LOCAL",L"1");SetEnvironmentVariableW(L"UCRR_PACKED_SPAN",L"1");SetEnvironmentVariableW(L"UCRR_PACKED_PROGRESS",L"1");SetEnvironmentVariableW(L"UCRR_INLINE_PREPARED",L"1");}
 /* Suppress the known per-frame IRQL warning flood in this diagnostic mode.
  * Errors remain recorded. Compare FPS with this capture policy disclosed. */
 SetEnvironmentVariableW(L"XBOX_LOG_LEVEL",L"0");
 int cpu_bundle=candidate&&GetEnvironmentVariableW(L"UCRR_CPU_BUNDLE",NULL,0)!=0;
 const wchar_t*cpu_flags[]={L"UCRR_CPU_METADATA",L"UCRR_CPU_STATE_PACKET",L"UCRR_CPU_CAPTURE_CORE",L"UCRR_CPU_BULK_STATE",L"UCRR_CPU_CALL_CACHE"};
 if(cpu_bundle)for(unsigned i=0;i<5;++i)SetEnvironmentVariableW(cpu_flags[i],L"1");
 if(residual)SetEnvironmentVariableW(L"UCRR_DEEP_TRACE",L"1");
 swprintf(path,MAX_PATH,L"%s\\kernel.log",diagnostic_session);SetEnvironmentVariableW(L"XBOX_LOG_PATH",path);
 swprintf(path,MAX_PATH,L"%s\\session.txt",diagnostic_session);FILE*f=_wfopen(path,L"w");if(!f)return 0;
 LARGE_INTEGER frequency,qpc;QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&qpc);
 fwprintf(f,L"Diagnostic capture v1\nlocal_start=%04u-%02u-%02u %02u:%02u:%02u\nruntime=%s\nstart_tick_ms=%llu\nqpc=%lld frequency=%lld\nHostage binding/script provenance, not decoded animation poses.\nTiming is instrumented; render profile adds per-draw clock calls.\nKernel warning flood suppressed (XBOX_LOG_LEVEL=0), errors kept.\nF8 marks an observed problem. No audio/video recording.\n",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,exe,(unsigned long long)GetTickCount64(),qpc.QuadPart,frequency.QuadPart);
 for(unsigned i=0;i<sizeof(preset)/sizeof(preset[0]);++i)fwprintf(f,L"%s=1\n",preset[i]);
 for(unsigned i=0;i<sizeof(flags)/sizeof(flags[0]);++i)if(GetEnvironmentVariableW(flags[i],NULL,0))fwprintf(f,L"%s=1\n",flags[i]);
 if(residual)fwprintf(f,L"residual_stack_capture=1; hostage/render profile disabled\n");
 if(residual)fwprintf(f,L"UCRR_DEEP_TRACE=1; bounded F8 frame command/geometry/reset trace and nonblocking GPU intervals\n");
 if(candidate)fwprintf(f,L"effect_isolation=disabled; all effects visible\n");
 if(cpu_bundle)for(unsigned i=0;i<5;++i)fwprintf(f,L"%s=1\n",cpu_flags[i]);
 if(GetEnvironmentVariableW(L"UCRR_EXTREME_PACKET",NULL,0))fwprintf(f,L"UCRR_EXTREME_PACKET=1; experimental packet-local ten-change bundle\n");
 if(candidate)fwprintf(f,L"UCRR_PACKED_PACKET=1\nUCRR_BOUNDS_LOCAL=1\nUCRR_PACKED_SPAN=1\nUCRR_PACKED_PROGRESS=1\nUCRR_INLINE_PREPARED=1\n");fclose(f);
 swprintf(source,MAX_PATH,L"%s\\candidate.json",root);swprintf(path,MAX_PATH,L"%s\\candidate.json",diagnostic_session);if(!CopyFileW(source,path,TRUE))return 0;
 swprintf(source,MAX_PATH,L"%s\\runtime\\xboxrecomp.ini",root);swprintf(path,MAX_PATH,L"%s\\settings.ini",diagnostic_session);if(!CopyFileW(source,path,TRUE))return 0;
 return 1;
}
static void diagnostic_result(DWORD pid,DWORD code){
 wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%s\\session.txt",diagnostic_session);FILE*f=_wfopen(path,L"a");
 if(f){fwprintf(f,L"game_pid=%lu\nexit_code=0x%08lX\nend_tick_ms=%llu\n",pid,code,(unsigned long long)GetTickCount64());fclose(f);}
}
#endif
