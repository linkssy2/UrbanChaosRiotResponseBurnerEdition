#ifndef UC_ENTITY_ATTACHED_CREATE_H
#define UC_ENTITY_ATTACHED_CREATE_H
#include "entity_registration.h"
#include "entity_properties.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Whole20A260/207D20 producer. Uses the SAME current world's creation fields;
 * mode is published in the supplied descriptor before parent ID is read.
 * The required factory completes1FBC50 and returns its real entity+54, including
 * -1 when registration exhausted. It owns class lookup, active descriptor,
 * allocation, constructor and any callback effects; never a success-only stub.
 * Successful source domain has a nonnull live returned entity. Factory failure
 * is committed2; preserve its partial owners, do not retry or roll back fields.
 * Borrowed inputs/output/creation/descriptor are distinct stable objects;
 * class_name is a live NUL string; factory may mutate their live values, but
 * retains the supplied descriptor until return. No restore after construction.
 *0 complete,1 invalid before effects,2 committed failure. No allocation here. */
int uc_entity_attached_create(UCEntityCreation *,const int32_t *parent_id,
    UCEntityProperties *,uint32_t placement_mode,const char *class_name,
    void *owner,int (*factory)(void *,const char *,UCEntityProperties *,int32_t *),
    int32_t *out_id);
#ifdef __cplusplus
}
#endif
#endif
