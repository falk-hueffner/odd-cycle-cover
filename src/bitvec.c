#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitvec.h"

struct bitvec *bitvec_make(size_t num_bits) {
    struct bitvec *v = calloc(sizeof (struct bitvec) + bitvec_bytes(num_bits), 1);
    v->num_bits = num_bits;
    return v;
}

void bitvec_copy(struct bitvec *d, const struct bitvec *s) {
    memcpy(d, s, sizeof (struct bitvec) + bitvec_bytes(s->num_bits));
}

size_t bitvec_find(const struct bitvec *v, size_t n) {
    if (n >= v->num_bits)
        return BITVEC_NOT_FOUND;

    unsigned long w = n / BITS_PER_WORD, words = bitvec_words(v->num_bits);
    unsigned long word = v->data[w];
    word &= ~0UL << (n % BITS_PER_WORD);

    while (true) {
        if (word) {
            n = w * BITS_PER_WORD + ctzl(word);
            if (n >= v->num_bits)
                break;
            else
                return n;
        }
        if (++w >= words)
            break;
        word = v->data[w];
    }

    return BITVEC_NOT_FOUND;
}

size_t bitvec_count(const struct bitvec *v) {
    size_t count = 0;
    for (size_t w = 0; w < bitvec_words(v->num_bits); ++w)
        count += popcountl(v->data[w]);

    return count;
}

static inline size_t min(size_t x, size_t y) { return x < y ? x : y; }
void bitvec_setminus(struct bitvec *d, const struct bitvec *s) {
    size_t words = min(bitvec_words(s->num_bits), bitvec_words(d->num_bits));
    for (size_t w = 0; w < words; ++w)
	d->data[w] &= ~s->data[w];
}

void bitvec_output(const struct bitvec *v, FILE *stream) {
    fprintf(stream, "[%zu/%lu:", bitvec_count(v), v->num_bits);
    for (size_t i = bitvec_find(v, 0); i != BITVEC_NOT_FOUND;
	 i = bitvec_find(v, i + 1))
	fprintf(stream, " %zd", i);
    fprintf(stream, "]");
}

void bitvec_clear(struct bitvec *v) {
    memset(v->data, 0, bitvec_bytes(v->num_bits));
}

void bitvec_invert(struct bitvec *v) {
    size_t words = bitvec_words(v->num_bits);

    for (size_t w = 0; w < words; ++w)
	v->data[w] = ~v->data[w];

    unsigned long padbits = BITS_PER_WORD * words - v->num_bits;
    if (padbits)
	v->data[words - 1] &= ~0UL >> padbits;
}
