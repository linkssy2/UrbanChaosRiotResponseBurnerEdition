#include "collision_notifications.h"
#include <string.h>

static UCCollisionStatus valid(const UCCollisionNotificationList *l)
{
    if (!l || l->count > l->capacity || (l->capacity && !l->data)) return UC_COLLISION_ARGUMENT;
    return l->capacity > 65536 ? UC_COLLISION_LIMIT : UC_COLLISION_OK;
}
static UCCollisionStatus invoke(UCCollisionNotificationDispatch *d, const void *listener,
    const void *body, UCCollisionNotificationEvent event)
{
    if (!listener) return UC_COLLISION_FORMAT;
    if (!d->remaining) return UC_COLLISION_LIMIT;
    --d->remaining;
    d->invoke(d->context, listener, body, event);
    return UC_COLLISION_OK;
}
static UCCollisionStatus reverse(UCCollisionNotificationList *l, const void *body,
    UCCollisionNotificationEvent event, UCCollisionNotificationDispatch *d)
{
    for (uint32_t i = l->count; i; ) {
        --i;
        if (i >= l->capacity) return UC_COLLISION_ARGUMENT;
        if (l->data[i]) {
            UCCollisionStatus status = invoke(d, l->data[i], body, event);
            if (status) return status;
            status = valid(l);
            if (status) return status;
        }
    }
    return UC_COLLISION_OK;
}
static UCCollisionStatus drain(UCCollisionNotificationList *l, const void *body,
    UCCollisionNotificationEvent event, UCCollisionNotificationDispatch *d)
{
    UCCollisionStatus status = valid(l);
    if (status) return status;
    while (l->count) {
        status = invoke(d, l->data[0], body, event);
        if (status) return status;
        status = valid(l);
        if (status) return status;
    }
    return UC_COLLISION_OK;
}
static UCCollisionStatus begin(const void *body, int added, UCCollisionNotificationDispatch *d)
{
    if (!body || !d || !d->invoke || (added != 0 && added != 1)) return UC_COLLISION_ARGUMENT;
    if (d->active) return UC_COLLISION_UNSUPPORTED;
    d->active = 1;
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_notify_world(UCCollisionNotificationList *l, int added,
    const void *body, UCCollisionNotificationDispatch *d)
{
    UCCollisionStatus status = valid(l);
    if (status) return status;
    status = begin(body, added, d);
    if (status) return status;
    status = reverse(l, body, added ? UC_COLLISION_NOTIFY_ADDED : UC_COLLISION_NOTIFY_REMOVED, d);
    if (!status) for (uint32_t i = l->count; i; ) {
        --i;
        if (!l->data[i]) {
            --l->count;
            memmove(l->data + i, l->data + i + 1, (l->count - i) * sizeof(*l->data));
        }
    }
    d->active = 0;
    return status;
}
UCCollisionStatus uc_collision_notify_body(UCCollisionBodyNotifications *b, int added,
    const void *body, UCCollisionNotificationDispatch *d)
{
    if (!b) return UC_COLLISION_ARGUMENT;
    UCCollisionStatus status = valid(&b->entity);
    if (!status && !added) status = valid(&b->drain_first);
    if (!status && !added) status = valid(&b->drain_second);
    if (!status && !added) status = valid(&b->final_remove);
    if (status) return status;
    status = begin(body, added, d);
    if (status) return status;
    status = reverse(&b->entity, body, added ? UC_COLLISION_NOTIFY_ADDED : UC_COLLISION_NOTIFY_REMOVED, d);
    if (!status && !added) status = drain(&b->drain_first, body, UC_COLLISION_NOTIFY_DRAIN_FIRST, d);
    if (!status && !added) status = drain(&b->drain_second, body, UC_COLLISION_NOTIFY_DRAIN_SECOND, d);
    if (!status && !added) {
        UCCollisionNotificationList *l = &b->final_remove;
        status = valid(l);
        for (uint32_t i = 0; !status && i < l->count; ++i) if (l->data[i]) {
            status = invoke(d, l->data[i], body, UC_COLLISION_NOTIFY_FINAL_REMOVE);
            if (!status) status = valid(l);
        }
    }
    d->active = 0;
    return status;
}
