#include "resource_handlers.h"
#include <stddef.h>

static int ascii(const char *s){size_t n=0;if(!s)return 0;while(s[n]){if((unsigned char)s[n]>127||++n>1023)return 0;}return 1;}
static unsigned lower(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int match(const char *a,const char *b,uint32_t n){
    while(n--){unsigned x=lower((unsigned char)*a++),y=lower((unsigned char)*b++);if(x!=y)return 0;if(!x)return 1;}return 1;
}
int uc_resource_handler_load(UCResourceHandlerKind kind,const UCResourceHandlerServices *h,
    void *provider,const char *name,const char *extension,uint32_t length,UCResourceCache *cache,void **out){
    static const char *const extensions[4][4]={{".tga",".dds",".bmdds",".ifl"},{".shader",".csf",".skcsf",NULL},{".mdl",".amf",".mtxt",NULL},{".sttxt",NULL,NULL,NULL}};
    void *stream=NULL,*aux=NULL,*storage,*result=NULL;UCResourceObjectKind object;unsigned i;
    if((unsigned)kind>3||!h||!out||!cache||!ascii(name)||!ascii(extension)||length>0x7FFFFFFFu||
        !h->preflight||!h->open||!h->material_provider||!h->texture_allocator||!h->allocate||!h->construct||!h->release_stream)return 1;
    if(!h->preflight(h->context,kind,provider,name,extension,length,cache))return 1;
    for(i=0;i<4&&extensions[kind][i];++i)if(match(extension,extensions[kind][i],length))break;
    if(i==4||!extensions[kind][i]){*out=NULL;return 0;}
    if(kind==UC_RESOURCE_STRING)object=UC_RESOURCE_STRING_OBJECT;
    else{
        if(kind==UC_RESOURCE_MATERIAL)provider=h->material_provider(h->context);
        stream=h->open(h->context,provider,name,kind==UC_RESOURCE_TEXTURE?1u:kind==UC_RESOURCE_MATERIAL?2u:3u);
        if(!stream){*out=NULL;return 0;}
        if(kind==UC_RESOURCE_TEXTURE){
            aux=h->texture_allocator(h->context,cache);
            object=match(extension,".ifl",length)?UC_RESOURCE_SEQUENCE_OBJECT:UC_RESOURCE_IMAGE_OBJECT;
            if(object==UC_RESOURCE_SEQUENCE_OBJECT)aux=cache;
        }else object=kind==UC_RESOURCE_MATERIAL?UC_RESOURCE_MATERIAL_OBJECT:UC_RESOURCE_MODEL_OBJECT;
        if(kind!=UC_RESOURCE_TEXTURE)aux=cache;
    }
    storage=h->allocate(h->context,object);
    if(storage)result=h->construct(h->context,object,storage,kind==UC_RESOURCE_STRING?(const void*)name:stream,aux);
    if(stream)h->release_stream(h->context,stream,1);
    *out=result;return 0;
}
