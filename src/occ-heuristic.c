#include <assert.h>

#include "bitvec.h"
#include "graph.h"
#include "occ.h"
#include "util.h"

// Return a possibly non-optimal OCC from a heuristic from A. Abdullah.
// Result is malloced.
struct bitvec *occ_heuristic(const struct graph *g) {
    size_t size = graph_size(g);
    ALLOCA_BITVEC(colors, size);
    struct bitvec *occ = bitvec_make(size);
    
    for (size_t i = 0; i < size; i++)
	bitvec_put(colors, i, rand() % 2);
    
    for (size_t i = 0; i < size; ) {
	size_t conflicts = 0, ok = 0;
	vertex w;
	GRAPH_NEIGHBORS_ITER(g, i, w) {
	    if (bitvec_get(colors, w) == bitvec_get(colors, i))
		conflicts++;
	    else
		ok++;
	}
	if (conflicts > ok) {
	    bitvec_toggle(colors, i);
	    i = 0;
	} else {
	    i++;
	}
    }

    while (true) {
	// Find node with minimum number of conflicts, and delete
#if 1
	// * it
	vertex worst = 0, max_conflicts = 0;
	for (size_t i = 0; i < size; i++) {
	    if (bitvec_get(occ, i))
		continue;
	    size_t conflicts = 0;
	    vertex w;
	    GRAPH_NEIGHBORS_ITER(g, i, w) {
		if (!bitvec_get(occ, w)
		    && bitvec_get(colors, w) == bitvec_get(colors, i))
		    conflicts++;
	    }
	    if (conflicts > max_conflicts) {
		worst = i;
		max_conflicts = conflicts;
	    }
	}
	if (max_conflicts == 0)
	    break;
	bitvec_set(occ, worst);
#else
	// * all its conflicting neighbors [Abdullah]; doesn't seem any better
	vertex best = 0, least_conflicts = size;
	for (size_t i = 0; i < size; i++) {
	    if (bitvec_get(occ, i))
		continue;
	    size_t conflicts = 0;
	    vertex w;
	    GRAPH_NEIGHBORS_ITER(g, i, w) {
		if (!bitvec_get(occ, w)
		    && bitvec_get(colors, w) == bitvec_get(colors, i))
		    conflicts++;
	    }
	    if (conflicts > 0 && conflicts < least_conflicts) {
		best = i;
		least_conflicts = conflicts;
	    }
	}
	if (least_conflicts == size)
	    break;
	vertex w;
	GRAPH_NEIGHBORS_ITER(g, best, w) {
	    if (!bitvec_get(occ, w)
		&& bitvec_get(colors, w) == bitvec_get(colors, best))
		bitvec_set(occ, w);
	}
#endif
    }
    assert(occ_is_occ(g, occ));
    return occ;
}
