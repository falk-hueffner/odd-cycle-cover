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

	vertex w;
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
				   const struct bitvec *yv2,
				   bool use_gray) {
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
    vertex source_vertices[ysize];
    vertex y1_xor_y2[ysize];
    size_t i = 0;
    BITVEC_ITER(yv1, v)
	y1[i++] = v;
    assert (i = ysize);
    i = 0;
    BITVEC_ITER(yv2, v)
	y2[i++] = v;
    assert (i = ysize);
    for (size_t i = 0; i < ysize; i++) {
	source_vertices[i] = y1[i];
	y1_xor_y2[i] = y1[i] ^ y2[i];
    }

    struct flow flow = flow_make(g->size);
    uint64_t code = 0, code_end = 1ULL << (ysize - 1);
    struct bitvec *cut = NULL;
    while (true) {
	if (!use_gray) 
	    flow_clear(&flow);
	while (flow.flow < ysize
	       && flow_augment(&flow, g, ysize, sources, source_vertices, targets))
	    continue;

	if (flow.flow < ysize) {
	    cut = bitvec_make(size);
	    flow_vertex_cut(&flow, g, sources, cut);
	    break;
	}

	if (++code >= code_end)
	    break;

	size_t x = gray_change(code);

	if (use_gray) {
	    if (bitvec_get(sources, y1[x])) {
		flow_drain_source(&flow, g, y1[x], targets);
	    } else {
		assert(bitvec_get(sources, y2[x]));
		flow_drain_source(&flow, g, y2[x], targets);
	    }
	    if (bitvec_get(targets, y1[x])) {
		// might already be drained by previous drain action
		if (flow_vertex_flow(&flow, y1[x]))
		    flow_drain_target(&flow, g, sources, y1[x]);
	    } else {
		assert(bitvec_get(targets, y2[x]));
		if (flow_vertex_flow(&flow, y2[x]))
		    flow_drain_target(&flow, g, sources, y2[x]);
	    }
	}
	
	bitvec_toggle(sources, y1[x]);
	bitvec_toggle(targets, y1[x]);
	bitvec_toggle(sources, y2[x]);
	bitvec_toggle(targets, y2[x]);
	source_vertices[x] ^= y1_xor_y2[x];
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

struct problem {
    const struct graph *g;	// The input graph
    struct graph *g_prime;	// G' as described by Reed et al.
    const struct bitvec *occ;	// The known OCC
    const vertex *occ_vertices;	// Array with the vertices in OCC
    size_t occ_size;		// Size of occ_vertices
    bool last_not_in_occ;	// Assume occ_vertices[occ_size-1] is not in a smaller OCC
    bool use_gray;		// Use gray code when enumerating valid partitions
    uint64_t subsets_examined;	// Statistics counter
};

// Build a new OCC given a cut CUT between two valid partitions of the
// subset REPLACED of the OCC.
// Returns malloced smaller OCC.
static struct bitvec *build_occ(const struct problem *problem,
				const struct bitvec *replaced,
				const struct bitvec *cut) {
    if (verbose)
	fprintf(stderr, "found small cut; ");
    struct bitvec *new_occ = bitvec_make(problem->g->size);
    bitvec_copy(new_occ, problem->occ);
    bitvec_setminus(new_occ, replaced);
    BITVEC_ITER(cut, v) {
	if (v >= problem->g->size)
	    v = problem->occ_vertices[v - problem->g->size];
	bitvec_set(new_occ, v);
    }
    assert(occ_is_occ(problem->g, new_occ));
    return new_occ;
}

// Enumerate all subsets Y of the OCC and find a smaller OCC by
// finding a small cut.
// Returns malloced smaller OCC or 0.
static struct bitvec *enum_occ_subsets(struct problem *problem) {
    size_t occ_size = problem->occ_size;
    size_t size = problem->g->size;
    ALLOCA_BITVEC(sub,      problem->g_prime->size);
    ALLOCA_BITVEC(y_origs,  problem->g_prime->size);
    ALLOCA_BITVEC(y_clones, problem->g_prime->size);

    // Start by considering replacing no vertices in the occ, i.e., Y
    // is empty, and no OCC vertex or its clone is in the subgraph of G'.
    for (size_t i = 0; i < size; i++)
	if (!bitvec_get(problem->occ, i))
	    bitvec_set(sub, i);
    if (problem->last_not_in_occ) {
	// We know the OCC vertex with highest index does not remain
	// in any smaller OCC. Enter it into Y for replacement.
	vertex last = problem->occ_vertices[occ_size - 1];
	vertex last_clone = size + occ_size - 1;
	bitvec_toggle(sub, last);
	bitvec_toggle(sub, last_clone);
	bitvec_toggle(y_origs, last);
	bitvec_toggle(y_clones, last_clone);
    }

    uint64_t code = 0;
    uint64_t code_end = 1ULL << (problem->last_not_in_occ ? occ_size - 1 : occ_size);
    while (true) {
#if 0
	struct graph *t = graph_subgraph(problem->g, y_origs);
	if (!graph_is_bipartite(t)) {
	    graph_free(t);
	    goto next;
	}
	graph_free(t);
#endif
	problem->subsets_examined++;
	struct graph *g2 = graph_subgraph(problem->g_prime, sub);
	struct bitvec *cut = small_cut_partition(g2, y_origs, y_clones, problem->use_gray);
	graph_free(g2);

	if (cut)
	    return build_occ(problem, y_origs, cut);

    next: UNUSED;
	size_t x = gray_change(code);
	if (++code >= code_end)
	    break;
	assert(x < occ_size);
	vertex orig = problem->occ_vertices[x], clone = size + x;
	bitvec_toggle(sub, orig);
	bitvec_toggle(sub, clone);
	bitvec_toggle(y_origs, orig);
	bitvec_toggle(y_clones, clone);	
    }
    return NULL;
}

enum color { GREY, BLACK, WHITE, RED };
static inline enum color invert_color(enum color c) {
    switch(c) {
    case BLACK: return WHITE;
    case WHITE: return BLACK;
    default: abort();
    }
}

static struct bitvec* bipsub_findcut(struct problem *problem,
				     struct bitvec *subgraph,
				     enum color *colors) {
    ALLOCA_BITVEC(clones, problem->g_prime->size);
    for (size_t i = 0; i < problem->occ_size; i++)
	bitvec_put(clones, problem->g->size + i,
		   bitvec_get(subgraph, problem->occ_vertices[i]));
    ALLOCA_BITVEC(g2sub, problem->g_prime->size);
    for (size_t i = 0; i < problem->g->size; i++) {
	if (bitvec_get(problem->occ, i))
	    bitvec_put(g2sub, i, bitvec_get(subgraph, i));
	else
	    bitvec_set(g2sub, i);
    }
    bitvec_join(g2sub, clones);
    problem->subsets_examined++;

    struct graph *g2 = graph_subgraph(problem->g_prime, g2sub);
    struct bitvec *cut = NULL;
#if 1
    struct flow flow = flow_make(g2->size);
    ALLOCA_BITVEC(sources, g2->size);
    ALLOCA_BITVEC(targets, g2->size);
    vertex source_vertices[problem->occ_size];
    size_t num_sources = 0;
    for (size_t i = 0; i < problem->occ_size; i++) {
	vertex v = problem->occ_vertices[i], v2 = problem->g->size + i;
	if (colors[v] == WHITE) {
	    bitvec_set(sources, v);
	    bitvec_set(targets, v2);
	    source_vertices[num_sources++] = v;
	} else if (colors[v] == BLACK) {
	    bitvec_set(sources, v2);
	    bitvec_set(targets, v);
	    source_vertices[num_sources++] = v2;
	} else {
	    assert(colors[v] == RED);
	}
    }
    while (flow.flow < num_sources
	   && flow_augment(&flow, g2, num_sources, sources,
			   source_vertices, targets))
	continue;
    
    if (flow.flow < num_sources) {
	cut = bitvec_make(g2->size);
	flow_vertex_cut(&flow, g2, sources, cut);
    }
#else
    cut = small_cut_partition(g2, subgraph, clones, problem->use_gray);
#endif
    graph_free(g2);

    if (cut)
	return build_occ(problem, subgraph, cut);
    else
	return NULL;
}

struct bitvec *bipsub_branch(struct problem *problem, const struct graph *g,
			     enum color *colors, struct bitvec *subgraph,
			     struct bitvec *in_queue,
			     vertex *qhead, vertex *qtail) {
    ALLOCA_U_BITVEC(in_queue_backup, g->size);
    bitvec_copy(in_queue_backup, in_queue);
    vertex *qtail_backup = qtail;
    bool did_enqueue = false;

    vertex v;
    if (qhead == qtail) {
	for (v = 0; v < g->size; v++)
	    if (graph_vertex_exists(g, v) && colors[v] == GREY)
		break;
	if (v == g->size) {
	    return bipsub_findcut(problem, subgraph, colors);
	} else {
	    bitvec_set(in_queue, v);
	    did_enqueue = true;
	}
    } else {
	v = *qhead++;
    }

    assert(colors[v] == GREY);
    enum color color = GREY;
    vertex w;
    GRAPH_NEIGHBORS_ITER(g, v, w) {
	enum color neighbor_color = colors[w];
	if (neighbor_color == BLACK || neighbor_color == WHITE) {
	    if (neighbor_color == color)
		goto try_red;
	    color = invert_color(neighbor_color);
	}
    }

    GRAPH_NEIGHBORS_ITER(g, v, w) {
	if (colors[w] == GREY && !bitvec_get(in_queue, w)) {
	    *qtail++ = w;
	    bitvec_set(in_queue, w);
	}
    }

    bitvec_set(subgraph, v);

    bool was_grey = color == GREY;
    if (was_grey)
	color = WHITE;
    colors[v] = color;

    struct bitvec *new_occ;
    new_occ = bipsub_branch(problem, g, colors, subgraph,
			    in_queue, qhead, qtail);
    if (new_occ)
	return new_occ;

#if 1
    if (was_grey) {
	colors[v] = BLACK;
	new_occ = bipsub_branch(problem, g, colors, subgraph,
				in_queue, qhead, qtail);
	if (new_occ)
	    return new_occ;
    }
#endif

    bitvec_unset(subgraph, v);
    bitvec_copy(in_queue, in_queue_backup);
    qtail = qtail_backup;

try_red:
    colors[v] = RED;
    new_occ = bipsub_branch(problem, g, colors, subgraph,
			    in_queue, qhead, qtail);
    if (new_occ)
	return new_occ;
    colors[v] = GREY;
    if (did_enqueue)
	bitvec_unset(in_queue, v);

    return NULL;
}

// Enumerate those subsets Y of OCC that induce a bipartite graph in G
// and find a smaller OCC by finding a small cut between valid
// partitions of Y.  Returns malloced smaller OCC or 0.
static struct bitvec *enum_occ_subsets_bipartite(struct problem *problem) {
    struct graph *occ_sub = graph_subgraph(problem->g, problem->occ);
    enum color colors[occ_sub->size];
    memset(colors, 0, sizeof colors);
    ALLOCA_BITVEC(subgraph, problem->g_prime->size);
    ALLOCA_BITVEC(in_queue, occ_sub->size);
    vertex queue[occ_sub->size];
    vertex *qtail = queue;

    if (problem->last_not_in_occ) {
	vertex last = problem->occ_vertices[problem->occ_size - 1], w;
	colors[last] = WHITE;
	GRAPH_NEIGHBORS_ITER(occ_sub, last, w) {
	    *qtail++ = w;
	    bitvec_set(in_queue, w);
	}
	bitvec_set(subgraph, last);
    }

    struct bitvec *new_occ = bipsub_branch(problem, occ_sub, colors, subgraph,
					   in_queue, queue, qtail);

    graph_free(occ_sub);
    return new_occ;
}

struct bitvec *occ_shrink(const struct graph *g, const struct bitvec *occ,
			  bool enum_bipartite, bool use_gray,
			  bool last_not_in_occ) {
    assert(occ_is_occ(g, occ));
    struct bitvec *new_occ = NULL;

    struct problem problem = {
	.g = g,
	.occ = occ,
	.occ_size = bitvec_count(occ),
	.last_not_in_occ = last_not_in_occ,
	.use_gray = use_gray,
	.subsets_examined = 0,
    };

    if (problem.occ_size == 0 || (last_not_in_occ && problem.occ_size == 1))
	return NULL;

    vertex occ_vertices[problem.occ_size];
    problem.g_prime = occ_gprime(g, occ, occ_vertices);
    problem.occ_vertices = occ_vertices;

    // If there are degree-0 clones, the functions below will get confused;
    // fortunately, we can immediately derive a solution.
    for (size_t i = 0; i < problem.occ_size; ++i) {
	if (   !graph_vertex_exists(problem.g_prime, problem.occ_vertices[i])
	    || !graph_vertex_exists(problem.g_prime, problem.g->size + i)) {
	    if (verbose)
		fprintf(stderr, "omitting redundant %d\n", (int) problem.occ_vertices[i]);
	    new_occ = bitvec_make(problem.g->size);
	    bitvec_copy(new_occ, occ);
	    bitvec_unset(new_occ, problem.occ_vertices[i]);
	    goto done;
	}
    }

    if (enum_bipartite) {
	new_occ = enum_occ_subsets_bipartite(&problem);
    } else {
	new_occ = enum_occ_subsets(&problem);
    }
    if (verbose)
	fprintf(stderr, "%llu subsets examined\n",
		(unsigned long long) problem.subsets_examined);

done:
    graph_free(problem.g_prime);
    return new_occ;
}
