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

// Construct auxiliary graph à la Reed et al.
static struct graph *occ_construct_h(struct occ_problem *problem) {
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

struct bitvec *occ_shrink(const struct graph *g, const struct bitvec *occ,
			  bool enum2col, bool use_graycode,
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
    struct occ_problem *problem = &(struct occ_problem) {
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

    if (enum2col)
	abort();//new_occ = occ_shrink_enum_2col(problem);
    else
        new_occ = occ_shrink_gray(problem);

    if (verbose)
	fprintf(stderr, "%llu flow augmentations\n",
                (unsigned long long) augmentations);

    graph_free(problem->h);
    bitvec_free(problem->sources);
    bitvec_free(problem->targets);
    flow_free(problem->flow);

    return new_occ;
}
