#ifndef BITVEC_H
#define BITVEC_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bit-twiddling.h"

struct bitvec {
    unsigned long num_bits;
    unsigned long data[];
};

#define BITS_PER_WORD (CHAR_BIT * sizeof (unsigned long))

static inline unsigned long bitvec_size(const struct bitvec *v) {
    return v->num_bits;
}

static inline unsigned long bitvec_words(unsigned long num_bits) {
    return (num_bits + BITS_PER_WORD - 1) / BITS_PER_WORD;
}

static inline unsigned long bitvec_bytes(unsigned long num_bits) {
    return bitvec_words(num_bits) * sizeof (unsigned long);
}

#define ALLOCA_U_BITVEC(name, n)					 \
    unsigned char name##_char[sizeof (unsigned long) + bitvec_bytes(n)]; \
    struct bitvec *name = (struct bitvec *) name##_char;		 \
    name->num_bits = n;

#define ALLOCA_BITVEC(name, n)						 \
    unsigned char name##_char[sizeof (unsigned long) + bitvec_bytes(n)]; \
    memset(name##_char, 0, sizeof name##_char);				 \
    struct bitvec *name = (struct bitvec *) name##_char;		 \
    name->num_bits = n;

#define BITVEC_ITER(v, n)						\
    for (size_t __w = 0; __w < bitvec_words(v->num_bits); __w++)	\
	for (unsigned long __word = v->data[__w],			\
             n = BITS_PER_WORD * __w + ctzl(__word);			\
             __word != 0;						\
             __word &= __word - 1, n = BITS_PER_WORD * __w + ctzl(__word))

static inline bool bitvec_get(const struct bitvec *v, size_t n) {
    assert (n < v->num_bits);
    return (v->data[n / BITS_PER_WORD] >> (n % BITS_PER_WORD)) & 1;
}

static inline void bitvec_set(struct bitvec *v, size_t n) {
    assert (n < v->num_bits);
    v->data[n / BITS_PER_WORD] |= 1UL << (n % BITS_PER_WORD);
}

static inline void bitvec_unset(struct bitvec *v, size_t n) {
    assert (n < v->num_bits);
    v->data[n / BITS_PER_WORD] &= ~(1UL << (n % BITS_PER_WORD));
}

static inline void bitvec_toggle(struct bitvec *v, size_t n) {
    assert (n < v->num_bits);
    v->data[n / BITS_PER_WORD] ^= 1UL << (n % BITS_PER_WORD);
}

static inline void bitvec_put(struct bitvec *v, size_t n, bool b) {
    assert (n < v->num_bits);
    unsigned long word =  v->data[n / BITS_PER_WORD];
    unsigned long mask1 = 1UL		    << (n % BITS_PER_WORD);
    unsigned long mask2 = (unsigned long) b << (n % BITS_PER_WORD);

    word &= ~mask1;
    word |=  mask2;

    v->data[n / BITS_PER_WORD] = word;
}

struct bitvec *bitvec_make(size_t num_bits);
void bitvec_copy(struct bitvec *d, const struct bitvec *s);
static inline void bitvec_free(struct bitvec *v) { free(v); }

size_t bitvec_count(const struct bitvec *v);
#define BITVEC_NOT_FOUND ((size_t) -1)
size_t bitvec_find(const struct bitvec *v, size_t n);

void bitvec_clear(struct bitvec *v);
void bitvec_fill(struct bitvec *v);
void bitvec_invert(struct bitvec *v);
void bitvec_setminus(struct bitvec *d, const struct bitvec *s);
void bitvec_join(struct bitvec *d, const struct bitvec *s);
void bitvec_output(const struct bitvec *v, FILE *stream);
static inline void bitvec_dump(const struct bitvec *v) {
    bitvec_output(v, stderr);
}

#endif	// BITVEC_H
