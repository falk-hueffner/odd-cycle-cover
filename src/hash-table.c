#include <stdlib.h>
#include <string.h>

#include "bitvec.h"
#include "hash-table.h"

static size_t hash_generic(const void *p, size_t n) {
  const unsigned char *s = (const unsigned char *) p;
  size_t r = 0;
  for (size_t i = 0; i < n; i++)
      r = r * 67 + s[i] - 113;
  return r;    
}

static bool eq_generic(const void *s1, const void* s2, size_t n) {
    return memcmp(s1, s2, n) == 0;
}

struct hash_table *hash_table_make1(size_t key_size, size_t value_size,
				    size_t entry_size, size_t value_offset,
				    hash_fun hash, eq_fun eq, size_t capacity) {
    struct hash_table *hash_table = malloc(sizeof (struct hash_table));
    hash_table->hash = hash ? hash : hash_generic;
    hash_table->eq   = eq   ? eq   : eq_generic;
    hash_table->key_size = key_size;
    hash_table->value_size = value_size;
    hash_table->entry_size = entry_size;
    hash_table->value_offset = value_offset;
    hash_table->num_entries = 0;
    hash_table->capacity = capacity;
    hash_table->entries = calloc(capacity, entry_size);
    hash_table->valid = bitvec_make(capacity);
    return hash_table;
}

void hash_table_free(struct hash_table *hash_table) {
    free(hash_table->entries);
    free(hash_table->valid);
    free(hash_table);
}

const void *hash_table_get(const struct hash_table *hash_table, void *key) {
    size_t h = hash_table->hash(key, hash_table->key_size);
    size_t i0 = h & (hash_table->capacity - 1), i = i0;
    do {
	unsigned char *entry = (hash_table->entries
				+ i * hash_table->entry_size);
	if (bitvec_get(hash_table->valid, i)) {
	    if (hash_table->eq(entry, key, hash_table->key_size)) 
		return entry + hash_table->value_offset;	    
	} else {
	    if (hash_table->entries[i] == 0)
		 return NULL;
	}
	if (++i >= hash_table->capacity)
	    i = 0;
    } while (i != i0);
    return NULL;
}

static void hash_table_grow(struct hash_table *hash_table) {
    size_t old_capacity = hash_table->capacity;
    void *old_entries = hash_table->entries;
    struct bitvec *old_valid = hash_table->valid;

    hash_table->capacity *= 2;
    hash_table->entries = calloc(hash_table->capacity, hash_table->entry_size);
    hash_table->valid = bitvec_make(hash_table->capacity);
    hash_table->num_entries = 0;
    
    for (size_t i = 0; i < old_capacity; i++) {
	if (bitvec_get(old_valid, i)) {
	    const unsigned char *entry = (old_entries
					  + i * hash_table->entry_size);
	    hash_table_set(hash_table, entry,
			   entry + hash_table->value_offset);
	}
    }
    free(old_entries);
    bitvec_free(old_valid);
}

void hash_table_set(struct hash_table *hash_table, const void *key,
		    const void *value) {
    if (hash_table->num_entries + 1 > hash_table->capacity / 2)
	hash_table_grow(hash_table);
    size_t h = hash_table->hash(key, hash_table->key_size);
    size_t i = h & (hash_table->capacity - 1); // capacity is power of 2
    while (true) {
	unsigned char *entry = (hash_table->entries
				+ i * hash_table->entry_size);
	if (!bitvec_get(hash_table->valid, i)) {
	    //fprintf(stderr, "set %d -> %d at %d\n", *(int*)key, *(int*)value, (int)i);
	    memcpy(entry, key, hash_table->key_size);
	    memcpy(entry + hash_table->value_offset,
		   value, hash_table->value_size);
	    bitvec_set(hash_table->valid, i);
	    hash_table->num_entries++;
	    return;
	}
	if (hash_table->eq(entry, key, hash_table->key_size)) {
	    //fprintf(stderr, "update %d -> %d at %d\n", *(int*)key, *(int*)value, (int)i);
	    memcpy(entry + hash_table->value_offset,
		   value, hash_table->value_size);
	    return;
	}
	if (++i >= hash_table->capacity)
	    i = 0;
    }
}

void hash_table_remove(struct hash_table *hash_table, void *key) {
    size_t h = hash_table->hash(key, hash_table->key_size);
    size_t i0 = h & (hash_table->capacity - 1), i = i0;
    //fprintf(stderr, "hash %d -> %d\n", *(int*)key, (int)i0);	
    do {
	unsigned char *entry = (hash_table->entries
				+ i * hash_table->entry_size);
	if (bitvec_get(hash_table->valid, i)) {
	    if (hash_table->eq(entry, key, hash_table->key_size)) {
		//fprintf(stderr, "delete %d -> at %d\n", *(int*)key, (int)i);	
		entry[0] = 1;
		bitvec_unset(hash_table->valid, i);
		hash_table->num_entries--;
		return;
	    }
	} else if (entry[0] == 0) {
	    //fprintf(stderr, "not found for deletion %d\n", *(int*)key);	
	    if (hash_table->entries[i] == 0)
		 return;
	}
	if (++i >= hash_table->capacity)
	    i = 0;
    } while (i != i0);
}
