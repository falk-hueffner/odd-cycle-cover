#include <stdlib.h>

#include "bitvec.h"
#include "flow.h"
#include "graph.h"

typedef bool port_t;
#define IN false
#define OUT true
#define PORT(port) (port == IN ? "in" : "out")

struct flow flow_make(size_t size) {
    struct flow flow = (struct flow) {
	.edge_flow   = malloc(size * sizeof (struct bitvec *)),
	.vertex_flow = bitvec_make(size),
	.flow        = 0,
	.size        = size,
    };

    for (unsigned i = 0; i < size; ++i)
	flow.edge_flow[i] = bitvec_make(size);

    return flow;
}

void flow_clear(struct flow *flow) {
    for (unsigned i = 0; i < flow->size; ++i)
	bitvec_clear(flow->edge_flow[i]);
    bitvec_clear(flow->vertex_flow);
    flow->flow = 0;
}

void flow_free(struct flow *flow) {
    for (unsigned i = 0; i < flow->size; ++i)
	free(flow->edge_flow[i]);
    free(flow->edge_flow);
    free(flow->vertex_flow);
}

bool flow_augment(struct flow *flow, const struct graph *g,
                  const struct bitvec *sources, const struct bitvec *targets) {
    size_t size = graph_size(g);
    vertex predecessors[size * 2];
    ALLOCA_BITVEC(seen, size * 2);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;
    for (size_t v = bitvec_find(sources, 0); v != BITVEC_NOT_FOUND;
	 v = bitvec_find(sources, v + 1)) {
	if (!bitvec_get(flow->vertex_flow, v)) {
	    vertex vcode = (v << 1) | OUT;
	    predecessors[vcode] = v;
	    *qtail++ = vcode;
	    bitvec_set(seen, vcode);
	}
    }

    vertex target = 0;		/* quench compiler warning */
    while (qhead != qtail) {
	vertex vcode = *qhead++;
	port_t port = vcode & 1;
	vertex v = vcode >>= 1, w;
	port_t wport = port ^ 1;

	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    vertex wcode = (w << 1) | wport;
	    if (!bitvec_get(seen, wcode)
		&& (port == OUT ? !bitvec_get(flow->edge_flow[v], w)
				:  bitvec_get(flow->edge_flow[w], v))) {
		predecessors[wcode] = v;
		
		*qtail++ = wcode;
		bitvec_set(seen, wcode);

		vertex w2code = wcode ^ 1;
		if (!bitvec_get(seen, w2code)
		    && bitvec_get(flow->vertex_flow, w) == (port == IN)) {
		    predecessors[w2code] = w;
		    if (bitvec_get(targets, w)) {
			assert (port == OUT);
			target = w;
			goto found;
		    }
		    *qtail++ = w2code;
		    bitvec_set(seen, w2code);
		}
	    }
	}
    }
    return false;

found:;    
    port_t s_port = OUT;
    vertex s = target;
    while (1) {
	vertex p = predecessors[(s << 1) | s_port];
	port_t p_port = s_port ^ 1;

	if (p == s) {
	    bitvec_toggle(flow->vertex_flow, p);
	} else {
	    if (p_port == OUT)
		bitvec_toggle(flow->edge_flow[p], s);
	    else
		bitvec_toggle(flow->edge_flow[s], p);
	}
	if (bitvec_get(sources, p) && p_port == IN) {
	    flow->flow++;
	    break;
	}
	s = p;
	s_port = p_port;
    }

    return true;    
}

void flow_saturate(struct flow *flow, const struct graph *g,
		   const struct bitvec *sources, const struct bitvec *targets,
		   size_t maxflow) {
    while (flow->flow < maxflow && flow_augment(flow, g, sources, targets))
	continue;
}

void flow_vertex_cut(const struct flow *flow, const struct graph *g,
                     const struct bitvec *sources, struct bitvec *cut) {
    size_t size = graph_size(g);
    ALLOCA_BITVEC(enqueued, size * 2);
    ALLOCA_BITVEC(seen, size);
    ALLOCA_BITVEC(reached, size);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;

    for (size_t v = bitvec_find(sources, 0); v != BITVEC_NOT_FOUND;
	 v = bitvec_find(sources, v + 1)) {
	bitvec_set(seen, v);
	if (!bitvec_get(flow->vertex_flow, v)) {
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
	    port_t wport = port ^ 1;
	    vertex wcode = (w << 1) | wport;
	    if (bitvec_get(enqueued, wcode))
		continue;
	    if (port == OUT)
		bitvec_set(seen, w);
	    if (port == OUT ? !bitvec_get(flow->edge_flow[v], w)
			    :  bitvec_get(flow->edge_flow[w], v)) {
		*qtail++ = wcode;
		bitvec_set(enqueued, wcode);
		if (wport == OUT)
		    bitvec_set(reached, w);

		vertex w2code = wcode ^ 1;
		if (!bitvec_get(enqueued, w2code)
		    && bitvec_get(flow->vertex_flow, w) == (port == IN)) {
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
    fprintf(stderr, "{ flow %u:\n", flow->flow);
    for (unsigned i = 0; i < flow->size; ++i) {
	for (unsigned j = 0; j < flow->size; ++j) {
	    if (bitvec_get(flow->edge_flow[i], j))
		fprintf(stderr, "%u -> %u\n", i, j);	    
	}	
    }
    for (unsigned i = 0; i < flow->size; ++i)
	if (bitvec_get(flow->vertex_flow, i))
	    fprintf(stderr, "%u -> %u\n", i, i);
    fprintf(stderr, "}\n");
}
