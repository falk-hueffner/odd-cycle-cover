/* Eleazar Eskin, Eran Halperin, Richard Karp.
   Efficient Reconstruction of Haplotype Structure via Perfect Phylogeny.
   Journal of Bioinformatics and Computational Biology. 1:1,1-20, 2003.  */
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <unistd.h>
#include <sys/times.h>

#include "bitvec.h"
#include "edge-occ.h"
#include "graph.h"
#include "occ.h"

double user_time(void) {
    struct tms buf;
    times(&buf);
    return (double) buf.tms_utime / sysconf(_SC_CLK_TCK);
}

void usage(FILE *stream) {
    fprintf(stream,
	    "occ: Calculate minimum odd cycle cover\n"
	    "  -d  Start with a random heuristic OCC and shrink it succesively\n"
	    "  -b  Enumerate valid partitions only for bipartite subgraphs\n"
	    "  -g  Enumerate valid partitions by gray code\n"
	    "  -v  Print progress to stderr\n"
	    "  -s  Print only statistics\n"
	    "  -h  Display this list of options\n"
	);
}

bool verbose    = false;
bool edge_occ	= false;
bool downwards	= false;
bool enum_2col  = false;
bool use_gray   = false;
bool stats_only = false;
unsigned long long augmentations = 0;

struct bitvec *find_occ(const struct graph *g) {
    struct bitvec *occ = NULL;
    
    if (downwards) {
	occ = occ_heuristic(g);
	for (size_t i = 0; i < 100; i++) {
	    struct bitvec *occ2 = occ_heuristic(g);
	    if (bitvec_count(occ2) < bitvec_count(occ)) {
		free(occ);
		occ = occ2;
	    } else {
		free(occ2);
	    }
	}
	struct bitvec *occ_new;
	while (true) {
	    if (verbose) {
		fprintf(stderr, "occ = ");
		bitvec_dump(occ);
		putc('\n', stderr);
	    }
	    occ_new = occ_shrink(g, occ, enum_2col, use_gray, false);
	    if (!occ_new || bitvec_count(occ_new) == bitvec_count(occ))
		break;
	    free(occ);
	    occ = occ_new;
	}
    } else {
	occ = bitvec_make(g->size);
	ALLOCA_BITVEC(sub, g->size);

	for (size_t i = 0; i < g->size; i++) {
	    bitvec_set(sub, i);
	    struct graph *g2 = graph_subgraph(g, sub);
	    if (occ_is_occ(g2, occ)) {
		graph_free(g2);
		continue;
	    }
	    bitvec_set(occ, i);
	    if (verbose) {
		fprintf(stderr, "size = %3zd ", graph_num_vertices(g2));
		fprintf(stderr, "occ = ");
		bitvec_dump(occ);
		putc('\n', stderr);
	    }
	    struct bitvec *occ_new = occ_shrink(g2, occ, enum_2col,
						use_gray, true);
	    if (occ_new) {
		free(occ);
		occ = occ_new;
		if (!occ_is_occ(g2, occ)) {
		    fprintf(stderr, "Internal error!\n");
		    abort();
		}
	    }
	    graph_free(g2);
	}
    }
    return occ;
}

struct edge_occ *find_edge_occ(const struct graph *g) {
    struct edge_occ *occ = edge_occ_make(0);
    vertex v, w;
    struct graph *g2 = graph_make(g->size);
    GRAPH_ITER_EDGES(g, v, w) {
	graph_connect(g2, v, w);
	if (edge_occ_is_occ(g2, occ))
	    continue;
	occ = realloc(occ, sizeof (struct edge_occ)
		      + (occ->size + 1) * sizeof *occ->edges);
	occ->edges[occ->size++] = (struct edge) { v, w };
	assert(edge_occ_is_occ(g2, occ));
	if (verbose) {
	    fprintf(stderr, "size = %3zd ", graph_num_edges(g2));
	    fprintf(stderr, "occ = "); edge_occ_dump(occ);
	    fprintf(stderr, "\n");
	}
	struct edge_occ *new_occ = edge_occ_shrink(g2, occ, use_gray);
	if (new_occ) {
	    free(occ);
	    occ = new_occ;
	}
    }

    return occ;
}

int main(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "edbgvsh")) != -1) {
	switch (c) {
	case 'e': edge_occ   = true; break;
	case 'd': downwards  = true; break;
	case 'b': enum_2col  = true; break;
	case 'g': use_gray   = true; break;
	case 'v': verbose    = true; break;
	case 's': stats_only = true; break;
	case 'h': usage(stdout); exit(0); break;
	default:  usage(stderr); exit(1); break;
	}
    }

    const char **vertices;
    struct graph *g = graph_read(stdin, &vertices);
    size_t occ_size;

    
    if (!edge_occ) {
	struct bitvec *occ = find_occ(g);
	occ_size = bitvec_count(occ);
	if (!stats_only)
	    BITVEC_ITER(occ, v)
		puts(vertices[v]);
    }  else {
	struct edge_occ *occ = find_edge_occ(g);
	occ_size = occ->size;
	if (!stats_only)
	    for (size_t i = 0; i < occ->size; i++)
		printf("%s %s\n",
		       vertices[occ->edges[i].v], vertices[occ->edges[i].w]);
    }
    
    if (stats_only)
	printf("%5zd %6zd %5zd %10.2f %16llu\n",
	       g->size, graph_num_edges(g), occ_size, user_time(), augmentations);

    return 0;
}
