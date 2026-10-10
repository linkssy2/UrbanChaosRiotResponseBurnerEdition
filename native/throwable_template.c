#include "throwable_template.h"
#include <string.h>
int uc_throwable_template_construct(UCThrowableTemplate *t,const UCEntityFreshContext *c,
 const UCEntityFreshView *v,const UCEntityAuthored *const *active,UCEntityListenerList *list,
 UCEntityNames *names,const uint32_t *code,void *owner,
 int (*receive)(void *,const void *,uint32_t *)){
 int inserted;UCEntityEventSubscription entry;
 if(!t||t->subscriptions||t->view||!c||!c->registration||!v||
    (active&&(!*active||!(*active)->definition||!(*active)->definition->class_name||
    strcmp((*active)->definition->class_name,"ThrownWeaponTemplateMolotov")))||
    !list||!names||!code||!receive)return 1;
 if(active?uc_entity_base_construct_authored(c,v,active):uc_entity_base_construct_unplaced(c,v))return 2;
 t->view=v;t->names=names;
 if(uc_entity_subscriptions_create(owner,&t->subscriptions))return 2;
 if(uc_entity_listener_register(c->registration->registry,*v->id,*v->id,
    uc_entity_subscriptions_listener(t->subscriptions),list,&inserted))return 2;
 entry.event=*code;entry.receive=receive;
 if(uc_entity_subscriptions_append(t->subscriptions,&entry))return 2;
 if(uc_entity_names_bind(names,"TEMPLATE_MOLOTOV",*v->id))return 2;
 return 0;
}
int uc_throwable_template_idle(const UCThrowableTemplate *t){return !t||!t->view;}
int uc_throwable_template_close_detached(UCThrowableTemplate *t){
 if(!t)return 1;
 if(t->view){UCEntityEventListener *l=uc_entity_subscriptions_listener(t->subscriptions);
  for(unsigned i=0;i<3;i++)if(t->view->target.identity->listeners[i]==l)t->view->target.identity->listeners[i]=NULL;
  if(uc_entity_names_remove(t->names,"TEMPLATE_MOLOTOV",*t->view->id))return 1;
 }
 uc_entity_subscriptions_close(t->subscriptions);memset(t,0,sizeof *t);return 0;
}
