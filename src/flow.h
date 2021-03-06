#ifndef FLOW_H
#define FLOW_H

#include <stdbool.h>

#include "graph.h"

struct flow;

struct flow* flow_make(const struct graph *g);
void flow_clear(struct flow *flow);
void flow_free(struct flow *flow);

size_t flow_flow(const struct flow *flow);
bool flow_vertex_flow(const struct flow *flow, vertex v);
bool flow_is_source(const struct flow *flow, vertex v);
bool flow_is_target(const struct flow *flow, vertex v);

bool flow_augment(struct flow *flow, const struct bitvec *sources,
		  const struct bitvec *targets);
bool flow_augment_pair(struct flow *flow, vertex source, vertex target);
vertex flow_drain_source(struct flow *flow, vertex source);
vertex flow_drain_target(struct flow *flow, vertex target);
struct bitvec *flow_vertex_cut(const struct flow *flow,
			       const struct bitvec *sources);
void flow_dump(const struct flow *flow);

#endif
