#include "archive_lookup.h"
#include <string.h>
int uc_resource_name(const char *name,char *out,size_t capacity){
    if(!name||!out||!capacity)return 1;char key[256];size_t n=0;unsigned slash=0;
    for(size_t i=0;;i++){
        if(i==1024)return 1;unsigned c=(unsigned char)name[i];
        if(!c||c=='#')break;if(c>=128)return 1;
        if(c=='/'||c=='\\'){if(slash)continue;c='\\';slash=1;}
        else{slash=0;if(c>='A'&&c<='Z')c+=32;}
        if(n==sizeof key-1)return 1;key[n++]=(char)c;
    }
    if(n>=capacity)return 1;key[n]=0;memcpy(out,key,n+1);return 0;
}
static int ascii_name(const char *p){
    if(!p)return 0;
    for(unsigned i=0;i<1024;i++){unsigned c=(unsigned char)p[i];if(c>=128)return 0;if(!c)return 1;}
    return 0;
}
static int equal(const char *a,const char *b){
    for(;;a++,b++){
        unsigned x=(unsigned char)*a,y=(unsigned char)*b;
        if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;
        if(x!=y)return 0;if(!x)return 1;
    }
}
static int accepted(const UCArchiveEntry *e,uint16_t tag){return !e->selection_tag||e->selection_tag==tag;}
int uc_archive_lookup_init(UCArchiveLookup *state,const UCArchive *archive,uint16_t tag){
    if(!state||!archive||uc_archive_count(archive)>INT32_MAX)return 1;
    for(size_t i=0;i<uc_archive_count(archive);i++){
        const UCArchiveEntry *e=uc_archive_entry(archive,i);
        if(accepted(e,tag)&&!ascii_name((const char*)e->name))return 1;
    }
    *state=(UCArchiveLookup){archive,SIZE_MAX,tag};return 0;
}
int uc_archive_lookup(UCArchiveLookup *state,const char *name,size_t *selected){
    if(!state||!state->archive||!selected||!ascii_name(name))return 1;
    size_t count=uc_archive_count(state->archive),found=SIZE_MAX;
    if(state->cursor!=SIZE_MAX&&(state->cursor>=count||!accepted(uc_archive_entry(state->archive,state->cursor),state->selection_tag)))return 1;
    for(size_t i=state->cursor==SIZE_MAX?0:state->cursor+1;i<count;i++){
        const UCArchiveEntry *e=uc_archive_entry(state->archive,i);
        if(!accepted(e,state->selection_tag))continue;
        if(equal((const char*)e->name,name))found=i;
        break;
    }
    /* Same lookup result as the original last-index map; loading-only scans
     * avoid allocating a second directory. Add an index if measured load time
     * warrants it. This function does not run during particle drawing. */
    if(found==SIZE_MAX)for(size_t i=count;i>0;i--){
        const UCArchiveEntry *e=uc_archive_entry(state->archive,i-1);
        if(accepted(e,state->selection_tag)&&equal((const char*)e->name,name)){found=i-1;break;}
    }
    if(found!=SIZE_MAX)state->cursor=found;
    *selected=found;return 0;
}
