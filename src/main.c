/* Eleazar Eskin, Eran Halperin, Richard Karp.
   Efficient Reconstruction of Haplotype Structure via Perfect Phylogeny.
   Journal of Bioinformatics and Computational Biology. 1:1,1-20, 2003.  */
#include <stdio.h>
#include <stdlib.h>

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

int main() {
    const char **vertices;
    struct graph *g = graph_read(stdin, &vertices);

    //graph_dump(g, vertices);

    struct bitvec *occ = bitvec_make(g->size);
    ALLOCA_BITVEC(sub, g->size);

    for (size_t i = 0; i < g->size; i++) {
	//fprintf(stderr, "size = %zd ", i+1);
	//fprintf(stderr, "occ = "); bitvec_dump(occ);
	bitvec_set(sub, i);
	struct graph *g2 = graph_subgraph(g, sub);
	if (occ_is_occ(g2, occ)) {
	    graph_free(g2);
	    continue;
	}
	bitvec_set(occ, i);
	struct bitvec *occ_new = occ_shrink(g2, occ, true);
	graph_free(g2);
	if (occ_new) {
	    free(occ);
	    occ = occ_new;
	}
    }
    //fprintf(stderr, "final occ = "); bitvec_dump(occ);

    printf("%5d %6d %5d %10.2f\n", g->size, graph_num_edges(g), bitvec_count(occ),
	   user_time());

    return 0;
}
