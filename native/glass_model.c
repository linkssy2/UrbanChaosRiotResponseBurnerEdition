#include "glass_model.h"
#include <string.h>
typedef struct Context{const UCGlassModelFactory*factory;int failure;}Context;
static int ascii(const char*s){if(!s)return 0;for(unsigned n=0;n<4096;n++){if((unsigned char)s[n]>127)return 0;if(!s[n])return 1;}return 0;}
static int preflight(void*c,const void*r,void*m,const char*n){Context*x=c;return x->factory&&x->factory->capacity&&x->factory->capacity<=4096&&x->factory->identity&&x->factory->current_primary&&ascii(r)&&m&&(!n||ascii(n));}
static void*create(void*c,void*f,const void*r){Context*x=c;(void)f;const char*ext=strrchr(r,'.');if(!ext)return NULL;const char*end=strchr(ext,'=');size_t n=end?(size_t)(end-ext):strlen(ext);const char*type=".glass_pane";for(size_t i=0;i<n;i++){unsigned a=(unsigned char)ext[i];if(a>='A'&&a<='Z')a+='a'-'A';if(!type[i]||a!=(unsigned char)type[i])return NULL;}UCGlassRegistry*model=NULL;if(uc_glass_registry_create(x->factory->capacity,x->factory->identity,&model))x->failure=2;return model;}
static void initialize(void*c,void*model,void*m){Context*x=c;if(uc_glass_registry_set_matrix(model,m))x->failure=2;}
static UCSceneModelGroup*primary(void*c){Context*x=c;return x->factory->current_primary(x->factory->context);}
static void name(void*c,void*model,const char*n){Context*x=c;if(uc_glass_registry_set_name(model,n))x->failure=2;}
int uc_glass_model_create(const UCGlassModelFactory*f,const char*r,const float m[16],const char*n,UCGlassRegistry**out){if(!out)return 1;Context x={f,0};const UCSceneModelCreateHooks h={preflight,create,initialize,primary,name};void*factory=(void*)f,*model=NULL;int result=uc_scene_model_create(&factory,r,(void*)m,n,&h,&x,&model);if(result!=1)*out=model;return x.failure?2:result;}
