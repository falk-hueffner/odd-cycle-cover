#ifndef FLOW_H
#define FLOW_H

#include <stdbool.h>

#include "graph.h"

struct flow;

struct flow* flow_make(size_t size);
void flow_clear(struct flow *flow);
void flow_free(struct flow *flow);

size_t flow_flow(const struct flow *flow);
bool flow_vertex_flow(const struct flow *flow, vertex v);
bool flow_is_source(const struct flow *flow, vertex v);
bool flow_is_target(const struct flow *flow, vertex v);

bool flow_augment(struct flow *flow, const struct graph *g,
                  const struct bitvec *sources,
		  const struct bitvec *targets);
void flow_augment_pair(struct flow *flow, const struct graph *g,
		       vertex source, vertex target);
vertex flow_drain_source(struct flow *flow, const struct graph *g,
			 vertex source);
vertex flow_drain_target(struct flow *flow, const struct graph *g,
			 vertex target);
void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut);
void flow_dump(const struct flow *flow);

#endif
