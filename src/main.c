/* Eleazar Eskin, Eran Halperin, Richard Karp.
   Efficient Reconstruction of Haplotype Structure via Perfect Phylogeny.
   Journal of Bioinformatics and Computational Biology. 1:1,1-20, 2003.  */
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <unistd.h>
#include <sys/times.h>

#include "bitvec.h"
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

bool verbose  = false;
int main(int argc, char *argv[]) {
    bool downwards	= false;
    bool enum_bipartite = false;
    bool use_gray       = false;
    bool stats_only     = false;
    int c;
    while ((c = getopt(argc, argv, "dbgvsh")) != -1) {
	switch (c) {
	case 'd': downwards      = true; break;
	case 'b': enum_bipartite = true; break;
	case 'g': use_gray       = true; break;
	case 'v': verbose        = true; break;
	case 's': stats_only     = true; break;
	case 'h': usage(stdout); exit(0); break;
	default:  usage(stderr); exit(1); break;
	}
    }

    const char **vertices;
    struct graph *g = graph_read(stdin, &vertices);
    struct bitvec *occ = NULL;
    if (downwards) {
	occ = occ_heuristic(g);
	for (size_t i = 0; i < 1000; i++) {
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
	    if (verbose)
		fprintf(stderr, "occ = "); bitvec_dump(occ); putc('\n', stderr);
	    occ_new = occ_shrink(g, occ, enum_bipartite, use_gray, false);
	    if (!occ_new)
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
		fprintf(stderr, "size = %zd ", graph_num_vertices(g2));
		fprintf(stderr, "occ = "); bitvec_dump(occ); putc('\n', stderr);
	    }
	    struct bitvec *occ_new = occ_shrink(g2, occ, enum_bipartite, use_gray, true);
	    if (occ_new) {
		free(occ);
		occ = occ_new;
		assert(occ_is_occ(g2, occ));
	    }
	    graph_free(g2);
	}
    }
    
    if (stats_only) {
	printf("%5zd %6zd %5zd %10.2f\n", g->size, graph_num_edges(g), bitvec_count(occ),
	       user_time());
    } else {
	BITVEC_ITER(occ, v)
	    puts(vertices[v]);
    }

    return 0;
}
