#ifndef GRAPH_H
#define GRAPH_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct bitvec;

/* uint16_t would do, but seems to be slower in benchmarks.  */
typedef unsigned vertex;

struct graph {
    size_t capacity;
    size_t size;
    struct vertex {
	vertex capacity;
	vertex deg;
	vertex neighbors[];
    } *vertices[];
};

#define GRAPH_NEIGHBORS_ITER(g, v, w)			\
    for (vertex __n = 0, w;				\
	 __n < g->vertices[v]->deg			\
	 && (w = g->vertices[v]->neighbors[__n++], 1);)

struct graph *graph_make(void);
struct graph *graph_grow(struct graph * g, size_t size);
struct graph *graph_subgraph(const struct graph *g, const struct bitvec *s);
void graph_free(struct graph *g);

static inline size_t graph_size(const struct graph *g) { return g->size; }
size_t graph_num_vertices(const struct graph * g);
size_t graph_num_edges(const struct graph * g);
static inline bool graph_vertex_exists(const struct graph *g, vertex v) {
    assert(v < g->size);
    return g->vertices[v] != NULL;
}
bool graph_is_bipartite(const struct graph *g);
bool graph_two_coloring(const struct graph *g, struct bitvec *colors);

void graph_connect(struct graph *g, vertex v, vertex w);

void graph_output(const struct graph *g, FILE* stream, const char **vertices);
static inline void graph_print(const struct graph *g, const char **vertices) {
    graph_output(g, stdout, vertices);
}
static inline void graph_dump(const struct graph *g, const char **vertices) {
    graph_output(g, stderr, vertices);
}
struct graph *graph_read(FILE* stream, const char ***vertices_out);

#endif // GRAPH_H
