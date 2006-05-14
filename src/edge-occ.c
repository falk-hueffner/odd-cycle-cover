#include <stdint.h>

#include "bitvec.h"
#include "edge-flow.h"
#include "edge-occ.h"

extern bool verbose;
extern unsigned long long augmentations;

struct edge_occ *edge_occ_make(size_t capacity) {
    struct edge_occ *occ = malloc(sizeof (struct edge_occ)
				  + capacity * sizeof (*occ->edges));
    occ->size = 0;
    return occ;
}

bool edge_occ_is_occ(const struct graph *g, const struct edge_occ *occ) {
    struct graph *g2 = graph_copy(g);
    for (size_t i = 0; i < occ->size; i++)
	graph_disconnect(g2, occ->edges[i].v, occ->edges[i].w);
    bool is_occ = graph_is_bipartite(g2);
    graph_free(g2);
    return is_occ;
}

struct edge_occ *edge_occ_shrink(const struct graph *g,
				 const struct edge_occ *occ,
				 bool use_gray) {
    if (occ->size == 0)		// avoid illegal alloca
	return NULL;
    struct graph *g2 = graph_copy(g);
    g2 = graph_grow(g2, g->size + 2 * occ->size);
    ALLOCA_BITVEC(sources, g2->size);
    ALLOCA_BITVEC(targets, g2->size);
    for (size_t i = 0; i < occ->size; i++) {
	vertex v = g->size + 2 * i, w = v + 1;
	graph_disconnect(g2, occ->edges[i].v, occ->edges[i].w);
	graph_connect(g2, occ->edges[i].v, v);
	graph_connect(g2, occ->edges[i].w, w);
	bitvec_set(sources, v);
	bitvec_set(targets, w);
    }

    struct edge_flow *flow = edge_flow_make(g2->size);
    uint64_t code = 0, code_end = 1ULL << (occ->size - 1);   
    struct edge_occ *cut = NULL;
    while (true) {
	if (!use_gray)
	    edge_flow_clear(flow);
	while (flow->flow < occ->size
	       && edge_flow_augment(flow, g2, sources, targets))
	    augmentations++;

	if (flow->flow < occ->size) {
	    cut = edge_flow_cut(flow, g2, sources);
	    break;
	}

	if (++code >= code_end)
            break;

	size_t x = gray_change(code);
	vertex s = g->size + 2 * x, t = s + 1;
        if (use_gray) {
	    if (!bitvec_get(sources, s)) {
		vertex tmp = s;
		s = t;
		t = tmp;
	    }
	    vertex t2 = edge_flow_drain_source(flow, g2, s);
	    if (t2 != t)
		edge_flow_drain_target(flow, g2, t);
	}
	bitvec_toggle(sources, s);
	bitvec_toggle(sources, t);
	bitvec_toggle(targets, s);
	bitvec_toggle(targets, t);
    }

    edge_flow_free(flow);
    graph_free(g2);

    if (cut)
	for (size_t i = 0; i < cut->size; i++) {
	    if (cut->edges[i].v >= g->size || cut->edges[i].w >= g->size) {
		vertex v = cut->edges[i].v >= g->size ? cut->edges[i].v
		    : cut->edges[i].w;
		cut->edges[i] = occ->edges[(v - g->size) / 2];
	    }
	}
    return cut;
}

void edge_occ_dump(const struct edge_occ *occ) {
    fprintf(stderr, "{%zd: ", occ->size);
    for (size_t i = 0; i < occ->size; i++) {
	if (i)
	    fprintf(stderr, ", ");
	fprintf(stderr, "%zd %zd",
		(size_t) occ->edges[i].v, (size_t) occ->edges[i].w);
    }
    fprintf(stderr, " }");
}
