#ifndef OCC_H
#define OCC_H

#include <stdbool.h>

#include "graph.h"

struct bitvec;

bool occ_is_occ(const struct graph *g, const struct bitvec *occ);
struct graph *occ_gprime(const struct graph *g, const struct bitvec *occ,
			 vertex *origs);
struct bitvec *occ_shrink(const struct graph *g, const struct bitvec *occ,
			  bool enum_bipartite, bool use_gray,
			  bool last_not_in_occ);
struct bitvec *occ_heuristic(const struct graph *g);

#endif // OCC_H
