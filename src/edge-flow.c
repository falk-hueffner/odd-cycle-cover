#include <stdlib.h>
#include <string.h>

#include "bitvec.h"
#include "edge-flow.h"
#include "edge-occ.h"
#include "hash-set.h"

struct edge_flow *edge_flow_make(size_t size) {
    struct edge_flow *flow = malloc(sizeof (struct edge_flow)
				    + size * sizeof *flow->edge_flow);
    flow->size = size;
    flow->flow = 0;
    for (size_t i = 0; i < size; ++i)
        flow->edge_flow[i] = calloc(size, sizeof *flow->edge_flow[i]);
    return flow;
}

void edge_flow_clear(struct edge_flow *flow) {
    for (size_t i = 0; i < flow->size; ++i)
	memset(flow->edge_flow[i], 0, flow->size * sizeof *flow->edge_flow[i]);
    flow->flow = 0;
}

void edge_flow_free(struct edge_flow *flow) {
    for (size_t i = 0; i < flow->size; ++i)
        free(flow->edge_flow[i]);
    free(flow);
}

bool edge_flow_augment(struct edge_flow *flow, const struct graph *g,
		       const struct bitvec *sources,
		       const struct bitvec *targets) {
    bool seen[g->size];
    memset(seen, 0, sizeof seen);
    vertex predecessors[g->size];
    vertex queue[g->size];
    vertex *qhead = queue, *qtail = queue;

    BITVEC_ITER(sources, s) {
	*qtail++ = s;
	seen[s] = true;
    }

    vertex target = 0;
    while (qhead != qtail) {
	vertex v = *qhead++, w;
	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    if (edge_flow_residual_capacity(flow, v, w)
		&& !seen[w]) {
		predecessors[w] = v;
		if (bitvec_get(targets, w)) {
		    target = w;
		    goto found;
		}
		*qtail++ = w;
		seen[w] = true;
	    }
	}
    }
    return false;

found:;
    vertex t = target, s;
    do {
	s = predecessors[t];
	edge_flow_push(flow, s, t);
	t = s;
    } while (!bitvec_get(sources, s));
    flow->flow++;
    return true;
}

vertex edge_flow_drain_source(struct edge_flow *flow, const struct graph *g,
			      vertex s) {
    vertex t;
    next:
    GRAPH_NEIGHBORS_ITER(g, s, t) {
	if (flow->edge_flow[s][t]) {
	    flow->edge_flow[s][t] = false;
	    s = t;
	    goto next;
	}
    }
    flow->flow--;
    return s;
}

vertex edge_flow_drain_target(struct edge_flow *flow, const struct graph *g,
			      vertex t) {
    vertex s;
    next:
    GRAPH_NEIGHBORS_ITER(g, t, s) {
	if (flow->edge_flow[s][t]) {
	    flow->edge_flow[s][t] = false;
	    t = s;
	    goto next;
	}
    }
    flow->flow--;
    return t;
}

struct edge_occ *edge_flow_cut(struct edge_flow *flow, const struct graph *g,
			       const struct bitvec *sources) {
    struct hash_table *edge_seen = hash_set_make(struct edge, NULL, NULL);
    struct hash_table *edge_traversed = hash_set_make(struct edge, NULL, NULL);
    bool seen[g->size];
    memset(seen, 0, sizeof seen);
    vertex queue[g->size];
    vertex *qhead = queue, *qtail = queue;

    BITVEC_ITER(sources, s) {
	*qtail++ = s;
	seen[s] = true;
    }

    while (qhead != qtail) {
	vertex v = *qhead++, w;
	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    struct edge e = v < w ? (struct edge) { v, w }
				  : (struct edge) { w, v };
	    hash_set_insert(edge_seen, &e);
	    if (edge_flow_residual_capacity(flow, v, w)) {
		hash_set_insert(edge_traversed, &e);
		if (!seen[w]) {
		    *qtail++ = w;
		    seen[w] = true;
		}
	    } 
	}
    }

    struct edge *pe;
    size_t n = 0;
    HASH_SET_ITER(edge_seen, pe)
	if (!hash_set_contains(edge_traversed, pe))
	    n++;
    struct edge_occ *occ = edge_occ_make(n);
    HASH_SET_ITER(edge_seen, pe)
	if (!hash_set_contains(edge_traversed, pe))
	    occ->edges[occ->size++] = *pe;

    return occ;    
}

void edge_flow_dump(const struct edge_flow *flow) {
    fprintf(stderr, "{ edge_flow = %zd\n", flow->flow);
    for (size_t v = 0; v < flow->size; v++)
	for (size_t w = 0; w < flow->size; w++)
	    if (flow->edge_flow[v][w])
		fprintf(stderr, "%zd -> %zd\n", v, w);
    fprintf(stderr, "}\n");
}
