#include <stdlib.h>

#include "bitvec.h"
#include "flow.h"
#include "graph.h"
#include "util.h"

typedef bool port_t;
#define IN false
#define OUT true
#define PORT(port) (port == IN ? "in" : "out")

void flow_clear(struct flow *flow) {
    for (size_t i = 0; i < flow->size; ++i)
	flow->come_from[i] = flow->go_to[i] = NULL_VERTEX;
    flow->flow = 0;
}

struct flow flow_make(size_t size) {
    struct flow flow = (struct flow) {
	.size      = size,
	.flow      = 0,
	.come_from = malloc(size * sizeof (*flow.come_from)),
	.go_to     = malloc(size * sizeof (*flow.go_to)),
    };

    flow_clear(&flow);

    return flow;
}

void flow_free(struct flow *flow) {
    free(flow->come_from);
    free(flow->go_to);
}

UNUSED static void verify_flow(const struct flow *flow,
			       const struct bitvec *sources,
			       const struct bitvec *targets) {
    for (size_t i = 0; i < flow->size; i++) {
	if (flow->go_to[i] != NULL_VERTEX) {
	    if (!(flow->come_from[flow->go_to[i]] == i))
		fprintf(stderr, "i = %zd", i);
	    assert(flow->come_from[flow->go_to[i]] == i);
	    if (!bitvec_get(sources, i)) {
		assert(flow->come_from[i] != NULL_VERTEX);
		assert(flow->go_to[flow->come_from[i]] == i);
	    } else {
		assert(flow->come_from[i] == NULL_VERTEX);
	    }
	}
	if (flow->come_from[i] != NULL_VERTEX) {
	    assert(flow->go_to[flow->come_from[i]] == i);
	    if (!bitvec_get(targets, i)) {
		assert(flow->go_to[i] != NULL_VERTEX);
		assert(flow->come_from[flow->go_to[i]] == i);
	    } else {
		assert(flow->go_to[i] == NULL_VERTEX);
	    }
	}
    }
}

bool flow_augment(struct flow *flow, const struct graph *g,
		  size_t num_sources,
                  const struct bitvec *sources, const vertex *source_vertices,
		  const struct bitvec *targets) {
    size_t size = graph_size(g);
    vertex predecessors[size * 2];
    bool seen[size * 2];
    memset(seen, 0, sizeof seen);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;
    for (size_t i = 0; i < num_sources; i++) {
	vertex v = source_vertices[i];
	if (flow->go_to[v] == NULL_VERTEX) {
	    vertex vcode = (v << 1) | OUT;
	    predecessors[vcode] = v;
	    *qtail++ = vcode;
	    seen[vcode] = true;
	}
    }

    vertex target = 0;		/* quench compiler warning */
    while (qhead != qtail) {
	vertex vcode = *qhead++;
	port_t port = vcode & 1;
	vertex v = vcode >>= 1, w;

	if (port == OUT) {
	    GRAPH_NEIGHBORS_ITER(g, v, w) {
		if (!graph_vertex_exists(g, w))
		    continue;
		vertex wcode = (w << 1) | IN;
		if (seen[wcode] || w == flow->go_to[v])
		    continue;

		predecessors[wcode] = v;
		*qtail++ = wcode;
		seen[wcode] = true;
		    
		vertex w2code = wcode ^ 1;
		if (!seen[w2code] && !flow_vertex_flow(flow, w)) {
		    predecessors[w2code] = w;
		    if (bitvec_get(targets, w)) {
			target = w;
			goto found;
		    }
		    *qtail++ = w2code;
		    seen[w2code] = true;
		}
	    }
	} else {
	    w = flow->come_from[v];
	    if (w != NULL_VERTEX) {
		vertex wcode = (w << 1) | OUT;
		if (!seen[wcode]) {
		    predecessors[wcode] = v;
		    *qtail++ = wcode;
		    seen[wcode] = true;
		    
		    vertex w2code = wcode ^ 1;
		    if (!seen[w2code] && flow_vertex_flow(flow, w)) {
			predecessors[w2code] = w;
			*qtail++ = w2code;
			seen[w2code] = true;
		    }
		}
	    }
	}
    }
    return false;

found:;    
    port_t t_port = OUT;
    vertex t = target;
    while (1) {
	vertex s = predecessors[(t << 1) | t_port];
	port_t s_port = t_port ^ 1;

	if (s == t) {
	    if (s_port == OUT) {
		flow->go_to[s] = NULL_VERTEX;
		flow->come_from[s] = NULL_VERTEX;
	    }
	} else {
	    if (s_port == OUT) {
		flow->come_from[t] = s;
		flow->go_to[s] = t;
	    }
	}
	if (bitvec_get(sources, s) && s_port == IN) {
	    flow->flow++;
	    break;
	}
	t = s;
	t_port = s_port;
    }

    return true;    
}

void flow_augment_pair(struct flow *flow, const struct graph *g,
		       vertex source, vertex target) {
    size_t size = graph_size(g);
    vertex predecessors[size * 2];
    bool seen[size * 2];
    memset(seen, 0, sizeof seen);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;
    assert(flow->go_to[source] == NULL_VERTEX);
    vertex sourcecode = (source << 1) | OUT;
    predecessors[sourcecode] = source;
    *qtail++ = sourcecode;
    seen[sourcecode] = true;

    while (qhead != qtail) {
	vertex vcode = *qhead++;
	port_t port = vcode & 1;
	vertex v = vcode >>= 1, w;

	if (port == OUT) {
	    GRAPH_NEIGHBORS_ITER(g, v, w) {
		if (!graph_vertex_exists(g, w))
		    continue;
		vertex wcode = (w << 1) | IN;
		if (seen[wcode] || w == flow->go_to[v])
		    continue;

		predecessors[wcode] = v;
		*qtail++ = wcode;
		seen[wcode] = true;
		    
		vertex w2code = wcode ^ 1;
		if (!seen[w2code] && !flow_vertex_flow(flow, w)) {
		    predecessors[w2code] = w;
		    if (w == target)
			goto found;
		    *qtail++ = w2code;
		    seen[w2code] = true;
		}
	    }
	} else {
	    w = flow->come_from[v];
	    if (w != NULL_VERTEX) {
		vertex wcode = (w << 1) | OUT;
		if (!seen[wcode]) {
		    predecessors[wcode] = v;
		    *qtail++ = wcode;
		    seen[wcode] = true;
		    
		    vertex w2code = wcode ^ 1;
		    if (!seen[w2code] && flow_vertex_flow(flow, w)) {
			predecessors[w2code] = w;
			*qtail++ = w2code;
			seen[w2code] = true;
		    }
		}
	    }
	}
    }
    return;

found:;    
    port_t t_port = OUT;
    vertex t = target;
    while (1) {
	vertex s = predecessors[(t << 1) | t_port];
	port_t s_port = t_port ^ 1;

	if (s == t) {
	    if (s_port == OUT) {
		flow->go_to[s] = NULL_VERTEX;
		flow->come_from[s] = NULL_VERTEX;
	    }
	} else {
	    if (s_port == OUT) {
		flow->come_from[t] = s;
		flow->go_to[s] = t;
	    }
	}
	if (s == source && s_port == IN) {
	    flow->flow++;
	    break;
	}
	t = s;
	t_port = s_port;
    }
}

vertex flow_drain_source(struct flow *flow, UNUSED const struct graph *g,
			 vertex source) {
    vertex v = source;
    while (flow->go_to[v] != NULL_VERTEX) {
	vertex succ = flow->go_to[v];
	flow->go_to[v] = NULL_VERTEX;
	flow->come_from[succ] = NULL_VERTEX;
	v = succ;
    }
    flow->flow--;
    return v;
}

vertex flow_drain_target(struct flow *flow, UNUSED const struct graph *g,
			 vertex target) {
    vertex v = target;
    while (flow->come_from[v] != NULL_VERTEX) {
	vertex pred = flow->come_from[v];
	flow->come_from[v] = NULL_VERTEX;
	flow->go_to[pred] = NULL_VERTEX;
	v = pred;
    }
    flow->flow--;
    return v;
}

void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut) {
    size_t size = graph_size(g);
    assert(bitvec_size(sources) >= size);
    assert(bitvec_size(cut) >= size);
    ALLOCA_BITVEC(enqueued, size * 2);
    ALLOCA_BITVEC(seen, size);
    ALLOCA_BITVEC(reached, size);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;

    for (size_t v = bitvec_find(sources, 0); v != BITVEC_NOT_FOUND;
	 v = bitvec_find(sources, v + 1)) {
	bitvec_set(seen, v);
	if (!flow_vertex_flow(flow, v)) {
	    vertex vcode = (v << 1) | OUT;
	    *qtail++ = vcode;
	    bitvec_set(enqueued, vcode);
	    bitvec_set(reached, v);
	}
    }

    while (qhead != qtail) {
	vertex vcode = *qhead++;
	port_t port = vcode & 1;
	vertex v = vcode >>= 1, w;

	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    if (!graph_vertex_exists(g, w))
		continue;
	    port_t wport = port ^ 1;
	    vertex wcode = (w << 1) | wport;
	    if (bitvec_get(enqueued, wcode))
		continue;
	    if (port == OUT)
		bitvec_set(seen, w);
	    if (port == OUT ? w != flow->go_to[v]
			    : w == flow->come_from[v]) {
		*qtail++ = wcode;
		bitvec_set(enqueued, wcode);
		if (wport == OUT)
		    bitvec_set(reached, w);

		vertex w2code = wcode ^ 1;
		if (!bitvec_get(enqueued, w2code)
		    && flow_vertex_flow(flow, w) == (port == IN)) {
		    *qtail++ = w2code;
		    bitvec_set(enqueued, w2code);
		    if (port == OUT)
			bitvec_set(reached, w);
		}
	    }
	}
    }
    
    bitvec_copy(cut, seen);
    bitvec_setminus(cut, reached);
}

void flow_dump(const struct flow *flow) {
    fprintf(stderr, "{ flow %zu:\n", flow->flow);
    for (size_t i = 0; i < flow->size; i++) {
	if (flow->go_to[i] != NULL_VERTEX)
	    fprintf(stderr, "%4zu -> %4zu\n", i, flow->go_to[i]);
	if (flow->come_from[i] != NULL_VERTEX)
	    fprintf(stderr, "%4zu <- %4zu\n", i, flow->come_from[i]);
    }
    fprintf(stderr, "}\n");
}
