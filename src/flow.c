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
	.edge_flow   = malloc(size * sizeof (bool *)),
	.vertex_flow = calloc(size, sizeof (bool)),
	.flow        = 0,
	.size        = size,
    };

    for (size_t i = 0; i < size; ++i)
	flow.edge_flow[i] = calloc(size, sizeof (bool));

    return flow;
}

void flow_clear(struct flow *flow) {
    for (size_t i = 0; i < flow->size; ++i)
	memset(flow->edge_flow[i], 0, flow->size * sizeof (bool));
    memset(flow->vertex_flow, 0, flow->size * sizeof (bool));
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
    bool **edge_flow = flow->edge_flow;
    bool *vertex_flow = flow->vertex_flow;
    vertex predecessors[size * 2];
    bool seen[size * 2];
    memset(seen, 0, sizeof seen);
    vertex queue[size * 2];
    vertex *qhead = queue, *qtail = queue;
    for (size_t v = bitvec_find(sources, 0); v != BITVEC_NOT_FOUND;
	 v = bitvec_find(sources, v + 1)) {
	if (!vertex_flow[v]) {
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
	port_t wport = port ^ 1;
	bool *edge_flow_v = edge_flow[v];

	GRAPH_NEIGHBORS_ITER(g, v, w) {
	    vertex wcode = (w << 1) | wport;
	    if ((port == OUT ? !edge_flow_v[w] : edge_flow[w][v])
		&& !seen[wcode]) {
		predecessors[wcode] = v;

		*qtail++ = wcode;
		seen[wcode] = true;

		vertex w2code = wcode ^ 1;
		if (!seen[w2code]
		    && vertex_flow[w] == (port == IN)) {
		    predecessors[w2code] = w;
		    if (bitvec_get(targets, w)) {
			assert (port == OUT);
			target = w;
			goto found;
		    }
		    *qtail++ = w2code;
		    seen[w2code] = true;
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
	    vertex_flow[p] ^= 1;
	} else {
	    if (p_port == OUT)
		edge_flow[p][s] ^= 1;
	    else
		edge_flow[s][p] ^= 1;
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
	if (!flow->vertex_flow[v]) {
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
	    if (port == OUT ? !flow->edge_flow[v][w]
			    :  flow->edge_flow[w][v]) {
		*qtail++ = wcode;
		bitvec_set(enqueued, wcode);
		if (wport == OUT)
		    bitvec_set(reached, w);

		vertex w2code = wcode ^ 1;
		if (!bitvec_get(enqueued, w2code)
		    && flow->vertex_flow[w] == (port == IN)) {
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
	    if (flow->edge_flow[i][j])
		fprintf(stderr, "%u -> %u\n", i, j);	    
	}	
    }
    for (unsigned i = 0; i < flow->size; ++i)
	if (flow->vertex_flow[i])
	    fprintf(stderr, "%u -> %u\n", i, i);
    fprintf(stderr, "}\n");
}
