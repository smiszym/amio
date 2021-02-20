#ifndef POOL_H
#define POOL_H

#include <stdbool.h>

/*
 * A GENERIC OBJECT POOL
 *
 * This module (pool.c / pool.h) provides a Pool class
 * which can be used to manage a set of objects, identifiable by IDs.
 * The main purpose is to avoid sharing raw pointers with Python code.
 * Throughout the lifetime of a Pool, the same ID is never reused, and
 * attempting to get an object by ID that is no longer present in the Pool
 * results in a NULL pointer.
 *
 * Both allocating and freeing memory for objects (put into slots as
 * void*) is not the responsibility of the Pool. For the Pool, the objects
 * are just opaque pointers.
 */

struct Slot
{
    /* Object id. It's unique throughout the pool lifetime */
    int id;

    /* Whether this slot is currently used by an object */
    bool allocated;

    /*
     * Note that the linked list created by prev_free_slot and next_free_slot
     * pointers doesn't need to be ordered by the index in the slot array.
     * In other words, prev_free_slot may point to an index higher than
     * the current one, and next_free_slot may point to a lower index as well
     */

    /*
     * Index of previous unallocated slot in the array, or -1 if none.
     * Only meaningful if this slot is unallocated
     */
    int prev_free_slot;

    /*
     * Index of next unallocated slot in the array, or -1 if none.
     * Only meaningful if this slot is unallocated
     */
    int next_free_slot;

    /* Object's data - the stored opaque pointer */
    void *object;
};

struct Pool
{
    /* An array of object slots */
    struct Slot *slots;

    /* Total number of slots (length of the slot array) */
    int num_slots;

    /* Index of the first unallocated slot, or -1 if none */
    int first_free_slot;

    /* The ID that the object created next will get */
    int next_object_id;
};

/*
 * Allocate memory for the pool and initialize it.
 */
void pool_create(struct Pool *pool, int num_slots);

/*
 * Allocate a slot for an object, put the object in the slot and return its ID.
 * On failure, returns -1.
 */
int pool_put(struct Pool *pool, void *object);

/*
 * Find an object with a given ID and return it.
 * If it doesn't exist, returns NULL.
 */
void * pool_find(struct Pool *pool, int id);

/*
 * Return the slot number of an object, or -1 if the object doesn't exist
 * in the pool. If the object is present, 0 <= key < num_slots.
 * This can be used as a key in associative arrays.
 */
int pool_get_key(struct Pool *pool, int id);

/*
 * Call the callback for each object in the pool.
 */
void pool_for_each(struct Pool *pool, void (*callback)(int id));

/*
 * Remove an object from its slot.
 */
void pool_remove(struct Pool *pool, int id);

/*
 * Free memory used by the slots. It doesn't free struct Pool itself.
 */
void pool_destroy(struct Pool *pool);

#endif
