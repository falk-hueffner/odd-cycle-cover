#ifndef FLOW_H
#define FLOW_H

#include <stdbool.h>

struct bitvec;
struct graph;

struct flow {
    struct bitvec **edge_flow;  /* Flows Out v -> In  u  (u <> v) */
    struct bitvec *vertex_flow; /* Flows In  v -> Out v */
    unsigned flow, size;
};

struct flow flow_make(size_t size);
void flow_clear(struct flow *flow);
void flow_free(struct flow *flow);

bool flow_augment(struct flow *flow, const struct graph *g,
                  const struct bitvec *sources, const struct bitvec *targets);
void flow_saturate(struct flow *flow, const struct graph *g,
		   const struct bitvec *sources, const struct bitvec *targets,
		   size_t maxflow);
void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut);

#endif