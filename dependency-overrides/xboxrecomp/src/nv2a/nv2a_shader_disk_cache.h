#ifndef NV_SHADER_DISK_CACHE_H
#define NV_SHADER_DISK_CACHE_H
/* Optional local cache. Hashes select files; exact source/contract equality
 * authorizes reuse. Corruption, incompatible versions and I/O failure miss. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
typedef struct NvShaderDiskHeader {
    uint32_t magic,version,contract[6],source_bytes,code_bytes;
    uint64_t code_hash;
} NvShaderDiskHeader;
static uint64_t nv_shader_hash(const void *p,size_t n,uint64_t h){
    const unsigned char *b=p;while(n--){h^=*b++;h*=1099511628211ull;}return h;
}
static int nv_shader_path(char path[1024],const char *dir,const char *source,const uint32_t contract[6]){
    if(!dir||!*dir)return 0;
    uint64_t h=nv_shader_hash(contract,24,14695981039346656037ull);
    h=nv_shader_hash(source,strlen(source),h);
    int n=snprintf(path,1024,"%s/v1-%016llx.bin",dir,(unsigned long long)h);
    return n>0&&n<1024;
}
static void *nv_shader_disk_load(const char *dir,const char *source,const uint32_t contract[6],size_t *size){
    char path[1024];*size=0;if(!nv_shader_path(path,dir,source,contract))return NULL;
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    NvShaderDiskHeader h;void *code=NULL;char *saved=NULL;size_t n=strlen(source);
    if(fread(&h,1,sizeof(h),f)!=sizeof(h)||h.magic!=0x5343564E||h.version!=1||
       memcmp(h.contract,contract,24)||n>2*1024*1024||h.source_bytes!=n||
       !h.code_bytes||h.code_bytes>16*1024*1024)goto done;
    saved=malloc(n?n:1);if(!saved)goto done;
    if(fread(saved,1,n,f)!=n||memcmp(saved,source,n))goto done;
    code=malloc(h.code_bytes);if(!code)goto done;
    if(fread(code,1,h.code_bytes,f)!=h.code_bytes||fgetc(f)!=EOF||
       nv_shader_hash(code,h.code_bytes,14695981039346656037ull)!=h.code_hash){free(code);code=NULL;}
    if(code)*size=h.code_bytes;
done:free(saved);fclose(f);return code;
}
static int nv_shader_disk_save(const char *dir,const char *source,const uint32_t contract[6],const void *code,size_t size){
    char path[1024],temp[1100];size_t n=strlen(source);
    if(!size||size>16*1024*1024||n>2*1024*1024||!nv_shader_path(path,dir,source,contract))return 0;
    CreateDirectoryA(dir,NULL); /* Parent must already exist. */
    snprintf(temp,sizeof(temp),"%s.%lu.%lu.tmp",path,GetCurrentProcessId(),GetCurrentThreadId());
    FILE *f=fopen(temp,"wb");if(!f)return 0;
    NvShaderDiskHeader h={0};h.magic=0x5343564E;h.version=1;memcpy(h.contract,contract,24);
    h.source_bytes=(uint32_t)n;h.code_bytes=(uint32_t)size;
    h.code_hash=nv_shader_hash(code,size,14695981039346656037ull);
    int ok=fwrite(&h,1,sizeof(h),f)==sizeof(h)&&fwrite(source,1,n,f)==n&&fwrite(code,1,size,f)==size;
    if(fclose(f))ok=0;
    if(ok)ok=MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileA(temp);return ok;
}
#endif
