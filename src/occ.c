#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bitvec.h"
#include "flow.h"
#include "graph.h"
#include "occ.h"
#include "util.h"

extern bool verbose;
extern unsigned long long augmentations;

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

struct problem {
    const struct graph *g;	// The input graph
    struct graph *h;		// G' as described by Reed et al.
    const struct bitvec *occ;	//
    vertex *occ_vertices;
    vertex *clones;
    struct bitvec *sources, *targets;
    struct flow *flow;
    size_t num_sources;
    bool use_graycode;
    bool last_not_in_occ;
    size_t occ_size, first_clone;
};

// Construct auxiliary graph à la Reed et al.
static struct graph *occ_construct_h(struct problem *problem) {
    size_t size = graph_size(problem->g);
    assert (bitvec_size(problem->occ) == size);
    problem->occ_vertices = calloc(sizeof *problem->occ_vertices, problem->occ_size);
    problem->clones = calloc(sizeof *problem->clones, size);
    ALLOCA_BITVEC(coloring, size + problem->occ_size);
    ALLOCA_BITVEC(not_occ, size);
    bitvec_copy(not_occ, problem->occ);
    bitvec_invert(not_occ);
    problem->h = graph_subgraph(problem->g, not_occ);
    graph_two_coloring(problem->h, coloring);
    problem->h = graph_grow(problem->h, size + problem->occ_size);
    size_t clone = 0;
    BITVEC_ITER(problem->occ, v) {
	problem->occ_vertices[clone] = v;
	problem->clones[v] = clone;

	vertex w;
	GRAPH_NEIGHBORS_ITER(problem->g, v, w) {
	    if (bitvec_get(problem->occ, w) && v > w)
		continue;
	    if (bitvec_get(coloring, w))
		graph_connect(problem->h, v, w);
	    else
		graph_connect(problem->h, size + clone, w);
	}
	clone++;
    }

    assert(graph_is_bipartite(problem->h));
    return problem->h;
}

bool occ_is_occ(const struct graph *g, const struct bitvec *occ) {
    assert(g->size == occ->num_bits);
    ALLOCA_U_BITVEC(not_occ, g->size);
    bitvec_copy(not_occ, occ);
    bitvec_invert(not_occ);    
    struct graph *g2 = graph_subgraph(g, not_occ);
    bool occ_is_occ = graph_is_bipartite(g2);
    graph_free(g2);
    return occ_is_occ;
}

enum code { SOURCE, DISABLED, TARGET };

static inline void update_vertex(struct problem *problem, size_t i, int g[],
				 enum code new_role) {
    enum code old_role = g[i];
    vertex v1 = problem->occ_vertices[i], v2 = problem->first_clone + i;
    if (old_role != DISABLED) {
	vertex s, t;
	if (old_role == SOURCE)
	    s = v1, t = v2;
	else
	    t = v1, s = v2;
	bitvec_unset(problem->sources, s);
	bitvec_unset(problem->targets, t);
	problem->num_sources--;
	if (problem->use_graycode)
	    if (flow_drain_source(problem->flow, s) != t)
		flow_drain_target(problem->flow, t);
    }
    if (new_role != DISABLED) {
	vertex s, t;
	if (new_role == SOURCE)
	    s = v1, t = v2;
	else
	    t = v1, s = v2;
	bitvec_set(problem->sources, s);
	bitvec_set(problem->targets, t);
	graph_vertex_enable(problem->h, s);
	graph_vertex_enable(problem->h, t);
	problem->num_sources++;
    } else {
	graph_vertex_disable(problem->h, v1);
	graph_vertex_disable(problem->h, v2);
    }
    g[i] = new_role;
}

uint64_t ipow(uint64_t a, uint64_t b) {
    if (b == 0)
        return 1;
    else
        return a * ipow(a, b - 1);
}

/*
  Example (3, k)-ary gray code. +: source, -: target, o: disabled (not in Y)
  [ + + + ]
  [ + + o ]
  [ + + - ]
  [ + o - ]
  [ + o o ]
  [ + o + ]
  [ + - + ]
  [ + - o ]
  [ + - - ]
  [ o - - ] #
  [ o - o ] #
  [ o - + ] #
  [ o o + ] #
  [ o o o ] #
  [ o o - ] *
  [ o + - ] *
  [ o + o ] *
  [ o + + ] *
  [ - + + ] *
  [ - + o ] *
  [ - + - ] *
  [ - o - ] *
  [ - o o ] *
  [ - o + ] *
  [ - - + ] *
  [ - - o ] *
  [ - - - ] *

  * can be ommitted for symmetry
  # can be ommitted if we know last is in Y  */
struct bitvec *occ_shrink_simple(struct problem *problem) {
    int u[problem->occ_size];	// +1 or -1, current Gray change direction
    int g[problem->occ_size];
    for (size_t i = 0; i < problem->occ_size; i++) {
	u[i] = +1;
	g[i] = DISABLED;	
	update_vertex(problem, i, g, SOURCE);
    }

    size_t num_codes;
    if (problem->last_not_in_occ)
	num_codes = ipow(3, problem->occ_size) / 3; // see comment above
    else
	num_codes = ipow(3, problem->occ_size) / 2 + 1;

    while (true) {
	if (!problem->use_graycode)
	    flow_clear(problem->flow);
	while (flow_flow(problem->flow) < problem->num_sources
               && flow_augment(problem->flow, problem->sources, problem->targets))
            augmentations++;

	if (flow_flow(problem->flow) < problem->num_sources) {
	    if (verbose)
		fprintf(stderr, "found small cut; ");
            struct bitvec *cut = flow_vertex_cut(problem->flow, problem->sources);
	    struct bitvec *new_occ = bitvec_make(problem->g->size);
	    bitvec_copy(new_occ, problem->occ);
	    bitvec_setminus(new_occ, problem->sources);
	    bitvec_setminus(new_occ, problem->targets);
	    BITVEC_ITER(cut, v) {
		if (v >= problem->g->size)
		    v = problem->occ_vertices[v - problem->first_clone];
		bitvec_set(new_occ, v);
	    }
	    assert(occ_is_occ(problem->g, new_occ));
	    bitvec_free(cut);
	    return new_occ;	    
        }
	if (--num_codes == 0)
	    break;

	// Generate next gray code.
	size_t i = 0;
	int j = g[0] + u[0];
        while(j >= 3 || j < 0) {
	    u[i] = -u[i];
	    if (++i >= problem->occ_size)
		return NULL;
	    j = g[i] + u[i];
        }
	update_vertex(problem, i, g, j);
    }
    return NULL;
}

struct bitvec *occ_shrink(const struct graph *g, const struct bitvec *occ,
			  bool enum_occ_twocolorings, bool use_graycode,
			  bool last_not_in_occ) {
    assert(occ_is_occ(g, occ));
    assert(graph_size(g) == bitvec_size(occ));
    size_t occ_size = bitvec_count(occ);
    if (occ_size == 0 || (last_not_in_occ && occ_size == 1))
        return NULL;

    // Ensure minimality first.
    struct bitvec *new_occ = bitvec_clone(occ);
    BITVEC_ITER(occ, v) {
	bitvec_unset(new_occ, v);
	if (occ_is_occ(g, new_occ)) {
	    if (verbose)
		fprintf(stderr, "omitting redundant %d\n", (int) v);
	} else {
	    bitvec_set(new_occ, v);
	}
	
    }
    if (bitvec_count(new_occ) < bitvec_count(occ))
	return new_occ;
    bitvec_free(new_occ);

    size_t h_size = g->size + occ_size;
    struct problem *problem = &(struct problem) {
	.g               = g,
	.occ             = occ,
	.sources	 = bitvec_make(h_size),
	.targets	 = bitvec_make(h_size),
	.num_sources     = 0,
	.use_graycode    = use_graycode,
	.last_not_in_occ = last_not_in_occ,
	.occ_size        = occ_size,
	.first_clone	 = graph_size(g),
    };
    occ_construct_h(problem);
    problem->flow = flow_make(problem->h);

    if (enum_occ_twocolorings)
	abort();//new_occ = occ_shrink_enum_2col(problem);
    else
        new_occ = occ_shrink_simple(problem);

    if (verbose)
	fprintf(stderr, "%llu flow augmentations\n",
                (unsigned long long) augmentations);

    graph_free(problem->h);
    bitvec_free(problem->sources);
    bitvec_free(problem->targets);
    flow_free(problem->flow);

    return new_occ;
}
