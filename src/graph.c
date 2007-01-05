#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
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

bool graph_is_connected(const struct graph *g, vertex v, vertex w) {
    assert(v < g->size);
    assert(w < g->size);
    for (size_t i = 0; i < g->vertices[v]->deg; i++)
	if (g->vertices[v]->neighbors[i] == w)
	    return true;
    return false;
}

void graph_connect(struct graph *g, vertex v, vertex w) {
    assert(v < g->size);
    assert(w < g->size);
    assert(v != w);

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

static int pstrcmp(const void *p1, const void *p2) {
    const char *s1 = *(const char **) p1;
    const char *s2 = *(const char **) p2;

    return strcmp(s1, s2);
}

struct graph *graph_read(FILE *stream, const char ***vertex_names) {
    size_t line_capacity = 0, line_num = 0;
    char *line = NULL;
    size_t num_names = 0, names_capacity = 16;
    const char **names = malloc(names_capacity * sizeof *names);
    size_t num_edges = 0, edges_capacity = 64;
    struct edge { const char *v[2]; } *edges
	= malloc(edges_capacity * sizeof *edges);

    while (get_line(&line, &line_capacity, stream)) {
	line_num++;
	const char *name[2];
	name[0] = strtok(line, WHITESPACE);
        if (!name[0] || name[0][0] == '#')
            continue;
        name[1] = strtok(NULL, WHITESPACE);
        if (!name[1] || name[1][0] == '#') {
            fprintf(stderr, "Syntax error on line %zu\n", line_num);
            exit(1);
        }
        const char *rest = strtok(NULL, WHITESPACE);
        if (rest && rest[0] != '#')
            fprintf(stderr, "warning: ignoring trailing garbage on line %zu\n",
		    line_num);

	// This is O(n^2) or so, but anything better seems like overkill.
	for (size_t i = 0; i < 2; i++) {
	    const char **p;
	    //fprintf(stderr, "contents:\n");
	    //for (size_t j = 0; j < num_names; j++)
	    //	fprintf(stderr, " %s [%zd]\n", names[j], j);

	    if ((p = bsearch(&name[i], names, num_names, sizeof *names,
			     pstrcmp))) {
		name[i] = *p;
		//fprintf(stderr, "found %s [%p]\n", name[i], name[i]);
	    } else {
		if (num_names >= names_capacity) {
		    names_capacity *= 2;
		    names = realloc(names, names_capacity * sizeof *names);
		}
		name[i] = dup_str(name[i]);
		names[num_names++] = name[i];
		qsort(names, num_names, sizeof *names, pstrcmp);
		//fprintf(stderr, "created %s [%p]\n", name[i], name[i]);
	    }
	}
	if (num_edges >= edges_capacity) {
	    edges_capacity *= 2;
	    edges = realloc(edges, edges_capacity * sizeof *edges);
	}
	edges[num_edges++] = (struct edge) { {name[0], name[1]} };
    }

    struct graph *g = graph_make(num_names);
    
    for (size_t i = 0; i < num_edges; i++) {
	vertex v[2];
	for (size_t j = 0; j < 2; j++) {
	    const char **p = bsearch(&edges[i].v[j], names, num_names,
				     sizeof *names, pstrcmp);
	    v[j] = p - names;
	}
	if (graph_vertex_exists(g, v[0]) && graph_vertex_exists(g, v[1])
	    && graph_is_connected(g, v[0], v[1]))
	    fprintf(stderr, "warning: duplicate edge\n");// {%s, %s}\n", names[v[0]], names[v[1]]);
	else
	    graph_connect(g, v[0], v[1]);
    }
    free(edges);
    *vertex_names = names;

    return g;
}
