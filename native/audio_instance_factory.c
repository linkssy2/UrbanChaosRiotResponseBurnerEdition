#include "audio_instance_factory.h"
int uc_audio_instance_factory_create(const UCAudioBufferGroup *group,
    const UCAudioInstanceFactory *h,UCAudioInstance **out){
    UCAudioInstance *p;
    if(!h||!out||!h->preflight||!h->current||!h->allocate||
       (group&&(!group->identity||!group->references))||h->preflight(h->context,group))return 1;
    if(!h->current(h->context))return 1;
    p=h->allocate(h->context);
    if(p){uc_audio_instance_begin(p);if(group){p->group=group->identity;++*group->references;}}
    *out=p;return 0;
}
