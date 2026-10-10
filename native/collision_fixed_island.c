#include "collision_fixed_island.h"
#include <stdlib.h>

struct UCCollisionFixedIsland {
    UCCollisionFixedIslandView view;
    UCCollisionFixedMember **members;
    uint32_t capacity;
};

UCCollisionStatus uc_collision_fixed_island_create(const void *world,
    uint32_t capacity, uint32_t config_1bf8, uint32_t config_1bfc,
    UCCollisionFixedIsland **out)
{
    if (!world || !out) return UC_COLLISION_ARGUMENT;
    if (!capacity || capacity > UC_COLLISION_FIXED_MAX_MEMBERS) return UC_COLLISION_LIMIT;
    UCCollisionFixedIsland *s = calloc(1, sizeof(*s));
    if (!s) return UC_COLLISION_MEMORY;
    s->members = calloc(capacity, sizeof(*s->members));
    if (!s->members) { free(s); return UC_COLLISION_MEMORY; }
    s->capacity = capacity;
    s->view.world = world;
    s->view.members = s->members;
    s->view.storage_index = UINT16_MAX;
    s->view.config_1bf8 = (uint16_t)config_1bf8;
    s->view.config_1bfc = (uint16_t)config_1bfc;
    *out = s;
    return UC_COLLISION_OK;
}

UCCollisionStatus uc_collision_fixed_island_free(UCCollisionFixedIsland *s)
{
    if (!s) return UC_COLLISION_ARGUMENT;
    if (s->view.count) return UC_COLLISION_ARGUMENT;
    free(s->members);
    free(s);
    return UC_COLLISION_OK;
}

const UCCollisionFixedIslandView *uc_collision_fixed_island_view(const UCCollisionFixedIsland *s)
{ return s ? &s->view : NULL; }

UCCollisionStatus uc_collision_fixed_island_add(UCCollisionFixedIsland *s,
    UCCollisionFixedMember *m)
{
    if (!s || !m || !m->identity || m->world || m->island || m->index != UINT16_MAX)
        return UC_COLLISION_ARGUMENT;
    if (m->fixed != 1) return UC_COLLISION_UNSUPPORTED;
    if (s->view.count == s->capacity) return UC_COLLISION_LIMIT;
    m->world = s->view.world;
    m->island = s;
    m->index = (uint16_t)s->view.count;
    s->members[s->view.count++] = m;
    s->view.accumulator += 0x80u;
    return UC_COLLISION_OK;
}

UCCollisionStatus uc_collision_fixed_island_remove(UCCollisionFixedIsland *s,
    UCCollisionFixedMember *m)
{
    if (!s || !m || m->world != s->view.world || m->island != s ||
        m->index >= s->view.count || s->members[m->index] != m)
        return UC_COLLISION_ARGUMENT;
    if (m->fixed != 1) return UC_COLLISION_UNSUPPORTED;
    m->world = NULL;
    UCCollisionFixedMember *last = s->members[s->view.count - 1];
    s->members[m->index] = last;
    last->index = m->index;
    --s->view.count;
    m->island = NULL;
    m->index = UINT16_MAX;
    s->view.accumulator -= 0x80u;
    s->view.dirty = 1;
    return UC_COLLISION_OK;
}
