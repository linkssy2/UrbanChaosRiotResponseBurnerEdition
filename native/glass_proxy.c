#include "glass_proxy.h"
static int valid(const char*s){if(!s)return 0;for(unsigned i=0;i<1024;i++){if(!s[i])return 1;if((unsigned char)s[i]>127)return 0;}return 0;}
static int lower(int c){return c>='A'&&c<='Z'?c+32:c;}
static int slash(int c){return c=='/'||c=='\\';}
static int compare(const char*a,const char*b){
    int unc_a=a[0]=='\\'&&a[1]=='\\',unc_b=b[0]=='\\'&&b[1]=='\\';
    if(unc_a!=unc_b){while(*a&&lower(*a)==lower(*b)){a++;b++;}return lower(*a)-lower(*b);}
    if(unc_a){a+=2;b+=2;}
    while(*a&&*b){
        if(slash(*a)&&slash(*b)){do{a++;}while(slash(*a));do{b++;}while(slash(*b));continue;}
        int x=lower(*a),y=lower(*b);if(x!=y)return x<y?-1:1;a++;b++;
    }
    return *a?1:*b?-1:0;
}
int uc_model_support_find(const UCModelSupportMap*m,const char*name,const void**out){
    if(!m||!out||!valid(name)||m->count>65536||(m->count&&!m->entries))return 1;
    for(size_t i=0;i<m->count;i++)if(!m->entries[i].value||!valid(m->entries[i].name)||
       (i&&compare(m->entries[i-1].name,m->entries[i].name)>=0))return 1;
    size_t lo=0,hi=m->count;
    while(lo<hi){size_t mid=lo+(hi-lo)/2;if(compare(m->entries[mid].name,name)<0)lo=mid+1;else hi=mid;}
    *out=lo<m->count&&compare(name,m->entries[lo].name)>=0?m->entries[lo].value:0;return 0;
}
int uc_glass_proxy_init(UCGlassProxy*p,const UCGlassModelFactory*f,const char*name,
                       const UCModelSupportMap*c,const UCModelSupportMap*e){
    const void *found;
    if(!p||!f||!f->identity||!f->current_primary||!f->capacity||f->capacity>4096||
       (name&&!valid(name))||uc_model_support_find(c,".glass_panehk",&found)||found||
       uc_model_support_find(e,".ENTSupp",&found)||found)return 1;
    int status=uc_glass_model_create(f,".glass_pane",f->identity,name,&p->model);
    if(status)return status==1?1:2;
    p->visible14=p->enabled15=1;p->collision4=p->support8=0;p->phase18=0;
    p->state34=0;p->state3C=0;p->flag39=0;p->state30=0;p->state0C=0;
    for(unsigned i=0;i<4;i++)p->tint20[i]=1;
    return 0;
}
