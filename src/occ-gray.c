#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "bitvec.h"
#include "flow.h"
#include "graph.h"
#include "occ.h"
#include "util.h"

extern bool verbose;
extern unsigned long long augmentations;

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
struct bitvec *occ_shrink_gray(struct problem *problem) {
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
