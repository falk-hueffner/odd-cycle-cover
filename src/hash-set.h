#ifndef HASH_SET_H
#define HASH_SET_H

#include <stdbool.h>
#include <stddef.h>

#include "hash-table.h"

#define hash_set_make(key, hash, eq) hash_table_make(key, char, hash, eq)

static inline void hash_set_free(struct hash_table *hash_set) {
    hash_table_free(hash_set);
}

static inline void hash_set_insert(struct hash_table *hash_table, void *key) {
    char c = 0;
    hash_table_set(hash_table, key, &c);
}

static inline bool hash_set_contains(struct hash_table *hash_table,
				     void *key) {
    return hash_table_get(hash_table, key) != NULL;
}

static inline void hash_set_remove(struct hash_table *hash_table,
				   void *key) {
    hash_table_remove(hash_table, key);
}

#define HASH_SET_ITER(h, p)					\
    BITVEC_ITER(h->valid, i)					\
	if (p = (void *) (h->entries + i * h->entry_size), 1)


#endif	// HASH_SET_H
