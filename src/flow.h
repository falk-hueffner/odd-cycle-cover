#ifndef FLOW_H
#define FLOW_H

#include <stdbool.h>

#include "graph.h"

#define NULL_VERTEX ((vertex) -1)
struct flow {
    size_t size, flow;
    vertex *come_from;
    vertex *go_to;
};

struct flow flow_make(size_t size);
void flow_clear(struct flow *flow);
void flow_free(struct flow *flow);

static inline bool flow_vertex_flow(const struct flow *flow, vertex v) {
    return flow->go_to[v] != NULL_VERTEX || flow->come_from[v] != NULL_VERTEX;
}
bool flow_augment(struct flow *flow, const struct graph *g,
		  size_t num_sources,
                  const struct bitvec *sources, const vertex *source_vertices,
		  const struct bitvec *targets);
void flow_drain_source(struct flow *flow, const struct graph *g,
		       vertex source, const struct bitvec *targets);
void flow_drain_target(struct flow *flow, const struct graph *g,
		       const struct bitvec *sources, vertex target);
void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut);
void flow_dump(const struct flow *flow);

#endif
