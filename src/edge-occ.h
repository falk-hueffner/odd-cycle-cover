#ifndef EDGE_OCC_H
#define EDGE_OCC_H

#include <stddef.h>
#include <stdbool.h>

#include "graph.h"

struct edge {
    vertex v, w;
};

struct edge_occ {
    size_t size;
    struct edge edges[];
};

struct edge_occ *edge_occ_make(size_t capacity);

bool edge_occ_is_occ(const struct graph *g, const struct edge_occ *occ);
struct edge_occ *edge_occ_shrink(const struct graph *g,
				 const struct edge_occ *occ,
				 bool use_gray);

void edge_occ_dump(const struct edge_occ *occ);

#endif	// EDGE_OCC_H
