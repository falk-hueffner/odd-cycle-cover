#include <stdlib.h>
#include <string.h>

char *strdup(const char *s);	/* string.h broken on alpha-linux */

#include "bitvec.h"
#include "graph.h"

size_t graph_num_vertices(const struct graph *g) {
    size_t n = 0;
    for (size_t v = 0; v < g->size; v++)
	if (graph_vertex_exists(g, v))
	    n++;
    return n;
}

size_t graph_num_edges(const struct graph *g) {
    size_t n = 0, _;
    for (size_t v = 0; v < g->size; v++)
	if (graph_vertex_exists(g, v))
	    GRAPH_NEIGHBORS_ITER(g, v, _)
		n++;
    assert ((n % 2) == 0);
    return n / 2;
}

struct graph *graph_copy(const struct graph *g) {
    struct graph *g2 = malloc(sizeof (struct graph)
			      + g->size * sizeof (struct vertex *));
    g2->capacity = g2->size = g->size;
    for (size_t i = 0; i < g->size; i++) {
	if (!graph_vertex_exists(g, i)) {
	    g2->vertices[i] = NULL_NEIGHBORS;
	} else {
	    size_t bytes = (sizeof (struct vertex)
			    + g->vertices[i]->deg * sizeof (vertex));
	    g2->vertices[i] = malloc(bytes);
	    memcpy(g2->vertices[i], g->vertices[i], bytes);
	}
    }
    return g2;
}

struct graph *graph_grow(struct graph *g, size_t size) {
    if (size > g->capacity) {
	g = realloc(g, sizeof (struct graph) + size * sizeof (struct vertex *));
	g->capacity = size;
    }
    for (size_t i = g->size; i < size; ++i)
	g->vertices[i] = NULL_NEIGHBORS;
    g->size = size;
    return g;
}

void graph_free(struct graph *g) {
    for (size_t v = 0; v < g->size; v++)
	if (graph_vertex_exists(g, v))
	    free(g->vertices[v]);
    free(g);
}

static void grow_neighbors(struct graph *g, vertex v, size_t new_capacity) {
    bool is_new = !graph_vertex_exists(g, v);
    if (is_new || new_capacity > g->vertices[v]->capacity) {
	g->vertices[v] = realloc(graph_vertex_exists(g, v) ? g->vertices[v] : NULL,
				 sizeof (struct vertex)
				 + new_capacity * sizeof (vertex));
	g->vertices[v]->capacity = new_capacity;
	if (is_new)
	    g->vertices[v]->deg = 0;
    }    
}

static struct vertex *malloc_vertices(size_t n) {
    struct vertex *result = malloc(sizeof (vertex) * (2 + n));
    result->deg = 0;
    result->capacity = n;
    return result;
}

struct graph *graph_make(size_t size) {
    struct graph *g = calloc(sizeof (struct graph)
			     + size * sizeof *g->vertices, 1);
    g->capacity = g->size = size;
    for (size_t i = 0; i < size; ++i)
	g->vertices[i] = NULL_NEIGHBORS;
    return g;
}


void graph_connect(struct graph *g, vertex v, vertex w) {
    assert(v < g->size);
    assert(w < g->size);

    grow_neighbors(g, v, graph_vertex_exists(g, v) ? g->vertices[v]->deg + 1 : 1);
    grow_neighbors(g, w, graph_vertex_exists(g, w) ? g->vertices[w]->deg + 1 : 1);
    
    g->vertices[v]->neighbors[g->vertices[v]->deg++] = w;
    g->vertices[w]->neighbors[g->vertices[w]->deg++] = v;
}

void graph_disconnect(struct graph *g, vertex v, vertex w) {
    assert(v < g->size);
    assert(w < g->size);
    assert(graph_vertex_exists(g, v));
    assert(graph_vertex_exists(g, w));

    for (size_t i = 0; i < g->vertices[v]->deg; i++) {
	if (g->vertices[v]->neighbors[i] == w) {
	    g->vertices[v]->neighbors[i] =
		g->vertices[v]->neighbors[--g->vertices[v]->deg];
	}
    }
    for (size_t i = 0; i < g->vertices[w]->deg; i++) {
	if (g->vertices[w]->neighbors[i] == v) {
	    g->vertices[w]->neighbors[i] =
		g->vertices[w]->neighbors[--g->vertices[w]->deg];
	}
    }
}

void graph_vertex_disable(struct graph *g, vertex v) {
    ((size_t *) g->vertices)[v] |= (size_t) 1;
}
void graph_vertex_enable(struct graph *g, vertex v) {
    ((size_t *) g->vertices)[v] &= ~(size_t) 1;
}

struct graph *graph_subgraph(const struct graph *g, const struct bitvec *s) {
    size_t size = g->size;
    struct graph *sub = malloc(sizeof (struct graph) + sizeof (void *) * size);
    sub->capacity = sub->size = size;

    for (size_t v = 0; v < g->size; v++) {
	size_t new_deg = 0;
	if (!bitvec_get(s, v) || !graph_vertex_exists(g, v)) {
	    sub->vertices[v] = NULL_NEIGHBORS;
	} else {
	    for (size_t n = 0; n < g->vertices[v]->deg; n++) {
		if (bitvec_get(s, g->vertices[v]->neighbors[n]))
		    new_deg++;
	    }
	    sub->vertices[v] = malloc_vertices(new_deg);
	    for (size_t n = 0; n < g->vertices[v]->deg; n++) {
		vertex w =g->vertices[v]->neighbors[n];
		if (bitvec_get(s, w))
		    sub->vertices[v]->neighbors[sub->vertices[v]->deg++] = w;
	    }
	}
    }

    return sub;    
}

bool graph_two_coloring(const struct graph *g, struct bitvec *colors) {
    size_t size = graph_size(g);
    assert(colors->num_bits >= size);
    ALLOCA_BITVEC(seen, size);
    vertex queue[size];
    vertex *qhead = queue, *qtail = queue;

    for (size_t v0 = 0; v0 < size; v0++) {
	if (!graph_vertex_exists(g, v0) || bitvec_get(seen, v0))
	    continue;
	assert(qtail <= queue + size);
	*qtail++ = v0;
	bitvec_set(seen, v0);
	do {
	    vertex v = *qhead++, w;
	    bool c = bitvec_get(colors, v);
	    GRAPH_NEIGHBORS_ITER(g, v, w) {
		if (!bitvec_get(seen, w)) {
		    bitvec_put(colors, w, !c);
		    assert(qtail < queue + size);
		    *qtail++ = w;
		    bitvec_set(seen, w);
		} else {
		    if (bitvec_get(colors, w) == c)
			return false;
		}
	    }
	    
	} while (qhead != qtail);
    }
    assert (bitvec_count(seen) == graph_num_vertices(g));
    return true;
}

bool graph_is_bipartite(const struct graph *g) {
    ALLOCA_BITVEC(colors, g->size);
    return graph_two_coloring(g, colors);    
}

void graph_output(const struct graph *g, FILE *stream, const char **vertices) {
    fprintf(stream, "{ %zd\n", g->size);
    for (unsigned v = 0; v < g->size; v++) {
	if (graph_vertex_exists(g, v)) {
	    if (g->vertices[v]->deg == 0) {
		if (vertices)
		    fprintf(stream, "%s\n", vertices[v]);
		else
		    fprintf(stream, "%d\n", v);
	    }
	    for (unsigned n = 0; n < g->vertices[v]->deg; n++) {
		unsigned w = g->vertices[v]->neighbors[n];
		if (v < w) {
		    if (vertices)
			fprintf(stream, "%s %s\n", vertices[v], vertices[w]);
		    else
			fprintf(stream, "%d %d\n", v, w);
		}
	    }
	}
    }
    fprintf(stream, "}\n");
}

struct graph *graph_read(FILE* stream, const char ***vertices_out) {
    // We ignore buffer overruns for simplicity.
    char buf[4096];
    struct edge { size_t v, w; } edges[16384];
    size_t num_edges = 0;
    const char **vertices = malloc(8192 * sizeof *vertices);
    size_t num_vertices = 0;

    while (fgets(buf, sizeof buf, stream)) {
	if (strncmp("# Graph Name", buf, strlen("# Graph Name")) == 0) {
	    // Special hack for Sebastian's graph format.
	    do {
		fgets(buf, sizeof buf, stream);
	    } while (strncmp("# Edges", buf, strlen("# Edges")) != 0);
	    continue;
	}
	const char *v_name = strtok(buf, " ,\t\n\r");
	if (!v_name || v_name[0] == '#')
	    continue;

	const char *w_name = strtok(NULL, " ,\t\n\r");
	if (!w_name) {
	    fprintf(stderr, "syntax error\n");
	    exit(1);
	}

	size_t v, w;
	for (v = 0; v < num_vertices; v++) {
	    if (strcmp(vertices[v], v_name) == 0)
		break;
	}
	if (v == num_vertices)
	    vertices[num_vertices++] = strdup(v_name);

	for (w = 0; w < num_vertices; w++)
	    if (strcmp(vertices[w], w_name) == 0)
		break;
	if (w == num_vertices)
	    vertices[num_vertices++] = strdup(w_name);

	edges[num_edges++] = (struct edge) { v, w };
    }

    size_t degs[num_vertices];
    memset(degs, 0, sizeof degs);

    for (size_t e = 0; e < num_edges; e++) {
	degs[edges[e].v]++;
	degs[edges[e].w]++;
    }

    struct graph *g = malloc(sizeof (struct graph)
			     + sizeof (struct vertex *) * num_vertices);
    g->size = num_vertices;
    g->capacity = num_vertices;
    for (size_t v = 0; v < g->size; v++)
	g->vertices[v] = malloc_vertices(degs[v]);

    for (size_t e = 0; e < num_edges; e++)
	graph_connect(g, edges[e].v, edges[e].w);

    *vertices_out = vertices;
    return g;
}
