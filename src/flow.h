#ifndef FLOW_H
#define FLOW_H

#include <stdbool.h>

#include "graph.h"

struct flow {
    bool **edge_flow;		/* Flows Out v -> In  u  (u <> v) */
    bool *vertex_flow;		/* Flows In  v -> Out v */
    unsigned flow, size, ysize;
};

struct flow flow_make(size_t size, size_t ysize);
void flow_clear(struct flow *flow);
void flow_free(struct flow *flow);

bool flow_augment(struct flow *flow, const struct graph *g,
                  const struct bitvec *sources, const vertex *source_vertices,
		  const struct bitvec *targets);
void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut);

#endif
