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
	    "  -v  Print progress to stderr\n"
	    "  -g  Enumerate valid partitions by gray code\n"
	    "  -s  Print only statistics\n"
	    "  -h  Display this list of options\n"
	);
}

bool verbose  = false;
int main(int argc, char *argv[]) {
    const char **vertices;
    struct graph *g = graph_read(stdin, &vertices);

    bool use_gray = false;
    bool stats    = false;
    int c;
    while ((c = getopt(argc, argv, "vgsh")) != -1) {
	switch (c) {
	case 'v': verbose  = true; break;
	case 'g': use_gray = true; break;
	case 's': stats    = true; break;
	case 'h': usage(stdout); exit(0); break;
	default:  usage(stderr); exit(1); break;
	}
    }

    struct bitvec *occ = bitvec_make(g->size);
    ALLOCA_BITVEC(sub, g->size);

    for (size_t i = 0; i < g->size; i++) {
	if (verbose) {
	    fprintf(stderr, "size = %zd ", i+1);
	    fprintf(stderr, "occ = "); bitvec_dump(occ); putc('\n', stderr);
	}
	bitvec_set(sub, i);
	struct graph *g2 = graph_subgraph(g, sub);
	if (occ_is_occ(g2, occ)) {
	    graph_free(g2);
	    continue;
	}
	bitvec_set(occ, i);
	struct bitvec *occ_new = occ_shrink(g2, occ, true, use_gray);
	graph_free(g2);
	if (occ_new) {
	    free(occ);
	    occ = occ_new;
	}
    }

    if (stats) {
	printf("%5d %6d %5d %10.2f\n", g->size, graph_num_edges(g), bitvec_count(occ),
	       user_time());
    } else {
	BITVEC_ITER(occ, v)
	    puts(vertices[v]);
    }

    return 0;
}
