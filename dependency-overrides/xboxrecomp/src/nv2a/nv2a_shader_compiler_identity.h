#ifndef NV_SHADER_COMPILER_IDENTITY_H
#define NV_SHADER_COMPILER_IDENTITY_H
#include <stdint.h>
#include <windows.h>
#include <d3dcompiler.h>
/* Identify the DLL export, not an EXE import thunk. Unrelated game rebuilds
 * must not invalidate compatible cached bytecode. Unknown compiler => no reuse. */
static HMODULE nv_shader_compiler_identity(uint32_t contract[6]){
    HMODULE compiler=GetModuleHandleA(D3DCOMPILER_DLL_A);
    FARPROC entry=compiler?GetProcAddress(compiler,"D3DCompile"):NULL;
    HMODULE owner=NULL;
    if(!entry||!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)(uintptr_t)entry,&owner))return NULL;
    const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER*)owner;
    const IMAGE_NT_HEADERS *nt=(const IMAGE_NT_HEADERS*)((const char*)owner+dos->e_lfanew);
    contract[3]=nt->FileHeader.TimeDateStamp;contract[4]=nt->OptionalHeader.SizeOfImage;
    return owner;
}
#endif
