#ifndef OCC_H
#define OCC_H

#include <stdbool.h>

#include "graph.h"

struct bitvec;
struct flow;

struct occ_problem {
    const struct graph *g;	// The input graph
    struct graph *h;		// G' as described by Reed et al.
    const struct bitvec *occ;
    vertex *occ_vertices;
    vertex *clones;
    struct bitvec *sources, *targets;
    struct flow *flow;
    size_t num_sources;
    bool use_graycode;
    bool last_not_in_occ;
    size_t occ_size, first_clone;
};

bool occ_is_occ(const struct graph *g, const struct bitvec *occ);
struct bitvec *occ_shrink(const struct graph *g, const struct bitvec *occ,
			  bool enum2col, bool use_graycode,
			  bool last_not_in_occ);
struct bitvec *occ_heuristic(const struct graph *g);

struct bitvec *occ_shrink_gray(struct occ_problem *problem);


#endif // OCC_H
