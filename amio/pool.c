#include "pool.h"

#include <assert.h>
#include <stdlib.h>

void pool_create(struct Pool *pool, int num_slots)
{
    pool->slots = malloc(num_slots * sizeof(struct Slot));
    for (int i = 0; i < num_slots; ++i) {
        pool->slots[i].id = -1;
        pool->slots[i].allocated = false;
        pool->slots[i].prev_free_slot = -1;
        pool->slots[i].next_free_slot = -1;
        pool->slots[i].object = NULL;
    }
    pool->num_slots = num_slots;
    pool->first_free_slot = 0;
    pool->next_object_id = 1;
}

int pool_put(struct Pool *pool, void *object)
{
    if (pool->slots[pool->next_object_id % pool->num_slots].allocated) {
        pool->next_object_id =
            (pool->next_object_id / pool->num_slots + 1) * pool->num_slots
            + pool->first_free_slot;
    }

    int id = pool->next_object_id;
    pool->next_object_id += 1;

    int slot_index = id % pool->num_slots;
    struct Slot *slot = &pool->slots[slot_index];

    if (slot->allocated)
        return -1;

    if (slot_index == pool->first_free_slot)
        pool->first_free_slot = slot->next_free_slot;

    if (slot->prev_free_slot != -1)
        pool->slots[slot->prev_free_slot].next_free_slot =
            slot->next_free_slot;

    if (slot->next_free_slot != -1)
        pool->slots[slot->next_free_slot].prev_free_slot =
            slot->prev_free_slot;

    slot->allocated = true;
    slot->object = object;
    slot->id = id;

    return id;
}

void * pool_find(struct Pool *pool, int id)
{
    struct Slot *slot = &pool->slots[id % pool->num_slots];
    if (!slot->allocated)
        return NULL;
    if (slot->id != id)
        return NULL;
    return slot->object;
}

int pool_get_key(struct Pool *pool, int id)
{
    int slot_number = id % pool->num_slots;
    struct Slot *slot = &pool->slots[slot_number];
    if (!slot->allocated)
        return -1;
    if (slot->id != id)
        return -1;
    return slot_number;
}

void pool_for_each(struct Pool *pool, void (*callback)(int id))
{
    for (int i = 0; i < pool->num_slots; ++i) {
        if (pool->slots[i].allocated)
            callback(pool->slots[i].id);
    }
}

void pool_remove(struct Pool *pool, int id)
{
    int slot_index = id % pool->num_slots;
    struct Slot *slot = &pool->slots[slot_index];
    assert(slot->allocated);
    assert(slot->id == id);

    slot->allocated = false;

    if (pool->first_free_slot != -1) {
        struct Slot *a = &pool->slots[pool->first_free_slot];

        slot->prev_free_slot = pool->first_free_slot;
        slot->next_free_slot = a->next_free_slot;

        if (a->next_free_slot != -1)
            pool->slots[a->next_free_slot].prev_free_slot = slot_index;
        a->next_free_slot = slot_index;
    } else {
        slot->prev_free_slot = -1;
        slot->next_free_slot = -1;
    }

    pool->first_free_slot = id;
}

static void remove_object_if_allocated(struct Pool *pool, int slot)
{
    if (pool->slots[slot].allocated)
        pool_remove(pool, pool->slots[slot].id);
}

void pool_destroy(struct Pool *pool)
{
    for (int i = 0; i < pool->num_slots; ++i) {
        remove_object_if_allocated(pool, i);
    }
    free(pool->slots);
}
