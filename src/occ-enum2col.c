#include "bitvec.h"
#include "flow.h"
#include "graph.h"
#include "occ.h"

extern bool verbose;
extern unsigned long long augmentations;

enum color { GREY, BLACK, WHITE, RED };

static struct bitvec* assemble_occ(struct occ_problem *problem,
				   const enum color *colors) {
    if (verbose)
	fprintf(stderr, "found small cut; ");

    struct bitvec *occ = bitvec_make(problem->g->size);
    ALLOCA_BITVEC(cut, problem->h->size);
    ALLOCA_BITVEC(sources, problem->h->size);
    for (size_t i = 0; i < problem->occ_size; i++) {
	vertex v = problem->occ_vertices[i];
	if (colors[i] == WHITE)
	    bitvec_set(sources, v);
	else if (colors[i] == BLACK)
	    bitvec_set(sources, problem->clones[v]);
	else
	    bitvec_set(occ, v);
    }
    
    cut = flow_vertex_cut(problem->flow, sources);
    BITVEC_ITER(cut, v) {
	if (v >= problem->first_clone)
	    v = problem->occ_vertices[v - problem->first_clone];
	bitvec_set(occ, v);
    }
    bitvec_free(cut);
    
    return occ;
}

static void remove_pair(struct occ_problem *problem, vertex v) {
    vertex s, t;
    if (flow_is_source(problem->flow, v)) {
	assert (flow_is_target(problem->flow, problem->clones[v]));
	s = v;
	t = problem->clones[v];
    } else {
	assert (flow_is_source(problem->flow, problem->clones[v]));
	s = problem->clones[v];
	t = v;
    }
    vertex t2 = flow_drain_source(problem->flow, s);
    if (t2 != t) {
	vertex s2 = flow_drain_target(problem->flow, t);
	graph_vertex_disable(problem->h, s);
	graph_vertex_disable(problem->h, t);
	flow_augment_pair(problem->flow, s2, t2);
	augmentations++;
	graph_vertex_enable(problem->h, s);
	graph_vertex_enable(problem->h, t);
    }
}

static struct bitvec *branch(struct occ_problem *problem, struct graph *occ_g,
			     enum color *colors, struct bitvec *in_queue,
			     vertex *qhead, vertex *qtail) {
    ALLOCA_U_BITVEC(in_queue_backup, occ_g->size);
    bitvec_copy(in_queue_backup, in_queue);
    vertex *qtail_backup = qtail;
    bool did_enqueue = false;

    // Find a vertex to branch on.
    size_t i;
    if (qhead == qtail) {
	// might optimize: don't start at 0
	for (i = 0; i < problem->occ_size; i++) {
	    if (colors[i] == GREY)
		break;
	}
	if (i == problem->occ_size) {
            return NULL;
	} else {
            bitvec_set(in_queue, i);
            did_enqueue = true;	    
	}	
    } else {
	i = *qhead++;
    }
    assert(colors[i] == GREY);
    vertex v = problem->occ_vertices[i], v2 = problem->first_clone + i;

    enum color color = GREY;
    // Check whether the color of i is already determined by its
    // neighbors, or whether it has to be omitted anyway since it has
    // both black and white neighbors.
    if (graph_vertex_exists(occ_g, i)) {
	vertex j;
	GRAPH_NEIGHBORS_ITER(occ_g, i, j) {
	    enum color neighbor_color = colors[j];
	    if (neighbor_color == BLACK || neighbor_color == WHITE) {
		if (neighbor_color == color)
		    goto try_red;
		color = neighbor_color == BLACK ? WHITE : BLACK;
	    }
	}
	GRAPH_NEIGHBORS_ITER(occ_g, i, j) {
	    if (colors[j] == GREY && !bitvec_get(in_queue, j)) {
		*qtail++ = j;
		bitvec_set(in_queue, j);
	    }
	}
    }

    bool was_grey = color == GREY;
    if (was_grey)
        color = WHITE;

    colors[i] = color;
    graph_vertex_enable(problem->h, v);
    graph_vertex_enable(problem->h, v2);

    vertex s, t;
    if (color == WHITE)
	s = v, t = v2;
    else
	s = v2, t = v;
    augmentations++;
    if (!flow_augment_pair(problem->flow, s, t))
	return assemble_occ(problem, colors);

    struct bitvec *new_occ;
    if ((new_occ = branch(problem, occ_g, colors, in_queue, qhead, qtail)))
	return new_occ;

    if (was_grey) {
	// 2nd branch.
	remove_pair(problem, v);
	colors[i] = BLACK;
	augmentations++;
	if (!flow_augment_pair(problem->flow, v2, v))
	    return assemble_occ(problem, colors);
	if ((new_occ = branch(problem, occ_g, colors, in_queue, qhead, qtail)))
	    return new_occ;
    }

    bitvec_copy(in_queue, in_queue_backup);
    qtail = qtail_backup;
    remove_pair(problem, v);
    graph_vertex_disable(problem->h, v);
    graph_vertex_disable(problem->h, v2);

try_red:
    colors[i] = RED;
    assert(!graph_vertex_exists(problem->h, v));
    if ((new_occ = branch(problem, occ_g, colors, in_queue, qhead, qtail)))
	return new_occ;
    colors[i] = GREY;
    if (did_enqueue)
	bitvec_unset(in_queue, i);

    return NULL;
}

struct bitvec *occ_shrink_enum2col(struct occ_problem *problem) {
    // Construct the induced subgrapg G[occ].
    struct graph *occ_g = graph_make(problem->occ_size);
    for (size_t i = 0; i < problem->occ_size; i++) {
	vertex v = problem->occ_vertices[i], w;
	GRAPH_NEIGHBORS_ITER(problem->g, v, w) {
	    if (v < w && bitvec_get(problem->occ, w)) {
		size_t j = problem->clones[w] - problem->first_clone;
		graph_connect(occ_g, i, j);		
	    }
	}
    }

    for (size_t i = 0; i < problem->occ_size - (problem->last_not_in_occ ? 1 : 0); i++) {
	vertex v = problem->occ_vertices[i];
	graph_vertex_disable(problem->h, v);
	graph_vertex_disable(problem->h, problem->first_clone + i);
    }

    enum color colors[problem->occ_size];
    memset(colors, 0, sizeof colors);
    
    vertex queue[problem->occ_size];
    ALLOCA_BITVEC(in_queue, problem->occ_size);
    vertex *qtail = queue;

    if (problem->last_not_in_occ) {
	size_t last = problem->occ_size - 1;
	vertex last_v = problem->occ_vertices[last], j;
	colors[last] = WHITE;
	flow_augment_pair(problem->flow, last_v, problem->clones[last_v]);
	augmentations++;
	if (graph_vertex_exists(occ_g, last)) {
	    GRAPH_NEIGHBORS_ITER(occ_g, last, j) {
		*qtail++ = j;
		bitvec_set(in_queue, j);
	    }
	}
    }

    struct bitvec *new_occ = branch(problem, occ_g, colors,
				    in_queue, queue, qtail);

    graph_free(occ_g);
    return new_occ;
}

