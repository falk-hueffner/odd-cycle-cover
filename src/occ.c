#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bitvec.h"
#include "flow.h"
#include "graph.h"
#include "occ.h"

struct graph *occ_gprime(const struct graph *g, const struct bitvec *occ,
			 vertex *origs) {
    size_t size = g->size;
    assert (bitvec_size(occ) == size);
    size_t num_clones = bitvec_count(occ);
    ALLOCA_BITVEC(coloring, size + num_clones);
    ALLOCA_BITVEC(not_occ, size);
    bitvec_copy(not_occ, occ);
    bitvec_invert(not_occ);
    struct graph *gprime = graph_subgraph(g, not_occ);
    graph_two_coloring(gprime, coloring);
    gprime = graph_grow(gprime, size + num_clones);
    size_t clone = 0;
    BITVEC_ITER(occ, v) {
	origs[clone] = v;
	
	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    if (bitvec_get(occ, w) && v > w)
		continue;
	    if (bitvec_get(coloring, w))
		graph_connect(gprime, v, w);
	    else
		graph_connect(gprime, size + clone, w);
	}
	clone++;
    }

    assert(graph_is_bipartite(gprime));
    return gprime;
}

struct bitvec *small_cut_partition(const struct graph *g,
				   const struct bitvec *yv1,
				   const struct bitvec *yv2) {
    size_t size = g->size;
    size_t ysize = bitvec_count(yv1);
    assert(ysize < 63);
    assert(bitvec_count(yv2) == ysize);
    if (ysize == 0)
	return NULL;

    ALLOCA_U_BITVEC(sources, size);
    ALLOCA_U_BITVEC(targets, size);
    bitvec_copy(sources, yv1);
    bitvec_copy(targets, yv2);
    vertex y1[ysize];
    vertex y2[ysize];
    size_t i = 0;
    BITVEC_ITER(yv1, v)
	y1[i++] = v;
    assert (i = ysize);
    i = 0;
    BITVEC_ITER(yv2, v)
	y2[i++] = v;
    assert (i = ysize);

    struct flow flow = flow_make(g->size);
    uint64_t code = 0, code_end = 1ULL << (ysize - 1);
    struct bitvec *cut = NULL;
    while (true) {
	flow_clear(&flow);
	flow_saturate(&flow, g, sources, targets, ysize);

	if (flow.flow < ysize) {
	    cut = bitvec_make(size);
	    flow_vertex_cut(&flow, g, sources, cut);
	    break;
	}

	if (++code >= code_end)
	    break;

	size_t x = gray_change(code);
	bitvec_toggle(sources, y1[x]);
	bitvec_toggle(targets, y1[x]);
	bitvec_toggle(sources, y2[x]);
	bitvec_toggle(targets, y2[x]);
    }

    flow_free(&flow);
    return cut;
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
			  bool last_not_in_occ) {
    assert(occ_is_occ(g, occ));
    size_t size = g->size;
    size_t ysize = bitvec_count(occ);
    if (ysize == 0 || (last_not_in_occ && ysize == 1))
	return NULL;
    vertex origs[ysize];
    struct bitvec *new_occ = NULL;
    struct graph *g_prime = occ_gprime(g, occ, origs);

    // If there are degree-0 clones, the code below will get confused;
    // fortunately, we can immediately derive a solution.
    for (size_t i = 0; i < ysize; ++i) {
	if (   !graph_vertex_exists(g_prime, origs[i])
	    || !graph_vertex_exists(g_prime, size + i)) {
	    new_occ = bitvec_make(size);
	    bitvec_copy(new_occ, occ);
	    bitvec_unset(new_occ, origs[i]);
	    goto done;
	}
    }

    size_t size2 = g_prime->size;

    ALLOCA_BITVEC(y_origs,  size2);
    ALLOCA_BITVEC(y_clones, size2);

    ALLOCA_BITVEC(sub, size2);
    for (size_t i = 0; i < size; i++)
	if (!bitvec_get(occ, i))
	    bitvec_set(sub, i);
    if (last_not_in_occ) {
	vertex orig = origs[ysize - 1], clone = size + ysize - 1;
	bitvec_toggle(sub, orig);
	bitvec_toggle(sub, clone);
	bitvec_toggle(y_origs, orig);
	bitvec_toggle(y_clones, clone);	
    }

    uint64_t code = 0, code_end = 1ULL << (last_not_in_occ ? ysize - 1 : ysize);
    while (true) {
#if 0
	struct graph *t = graph_subgraph(g, y_origs);
	if (!graph_is_bipartite(t)) {
	    graph_free(t);
	    goto next;
	}
	graph_free(t);
#endif
	struct graph *g2 = graph_subgraph(g_prime, sub);	
	struct bitvec *cut = small_cut_partition(g2, y_origs, y_clones);
	graph_free(g2);

	if (cut) {
	    new_occ = bitvec_make(size);
	    bitvec_copy(new_occ, occ);
	    bitvec_setminus(new_occ, y_origs);
	    BITVEC_ITER(cut, v) {
		if (v >= size)
		    v = origs[v - size];
		bitvec_set(new_occ, v);
	    }
	    assert(occ_is_occ(g, new_occ));
	    break;
	}
    next:;
	size_t x = gray_change(code);
	if (++code >= code_end)
	    break;
	assert(x < ysize);
	vertex orig = origs[x], clone = size + x;
	bitvec_toggle(sub, orig);
	bitvec_toggle(sub, clone);
	bitvec_toggle(y_origs, orig);
	bitvec_toggle(y_clones, clone);
    }
done:
    free(g_prime);
    return new_occ;
}
