#ifndef EDGE_FLOW_H
#define EDGE_FLOW_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "graph.h"

struct bitvec;
struct edge_occ;

struct edge_flow {
    size_t size, flow;
    bool *edge_flow[];		// Invariant: not both uv and vu set
};

struct edge_flow *edge_flow_make(size_t size);
void edge_flow_clear(struct edge_flow *flow);
void edge_flow_free(struct edge_flow *flow);

static inline bool edge_flow_residual_capacity(const struct edge_flow *flow,
					       vertex v, vertex w) {
    return !flow->edge_flow[v][w];
}

static inline void edge_flow_push(const struct edge_flow *flow,
				  vertex v, vertex w) {
    assert(edge_flow_residual_capacity(flow, v, w));
    if (flow->edge_flow[w][v])
	flow->edge_flow[w][v] = false;
    else
	flow->edge_flow[v][w] = true;
}

bool edge_flow_augment(struct edge_flow *flow, const struct graph *g,
		       const struct bitvec *sources,
		       const struct bitvec *targets);
vertex edge_flow_drain_source(struct edge_flow *flow, const struct graph *g,
			      vertex s);
vertex edge_flow_drain_target(struct edge_flow *flow, const struct graph *g,
			      vertex t);
struct edge_occ *edge_flow_cut(struct edge_flow *flow, const struct graph *g,
			       const struct bitvec *sources);

void edge_flow_dump(const struct edge_flow *flow);

#endif
