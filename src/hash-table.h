#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef size_t (*hash_fun)(const void *, size_t);
typedef bool (*eq_fun)(const void *, const void *, size_t);

struct hash_table {
    hash_fun hash;
    eq_fun eq;
    size_t key_size, value_size;
    size_t entry_size, value_offset;
    size_t num_entries, capacity;
    unsigned char *entries;
    struct bitvec *valid;
};

#define hash_table_make(key, value, hash, eq)				\
    hash_table_make1(sizeof (key), sizeof (value),			\
		     sizeof (struct { key __k; value __v; }),		\
		     offsetof (struct { key __k; value __v; }, __v),	\
		     hash, eq, 8)

struct hash_table *hash_table_make1(size_t key_size, size_t value_size,
				    size_t entry_size, size_t value_offset,
				    hash_fun hash, eq_fun eq, size_t capacity);
void hash_table_free(struct hash_table *hash_table);
const void *hash_table_get(const struct hash_table *hash_table, void *key);
void hash_table_set(struct hash_table *hash_table, const void *key,
		    const void *value);
void hash_table_remove(struct hash_table *hash_table, void *key);

#endif	// HASH_TABLE_H
