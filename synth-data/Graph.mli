(** Module for handling undirected (imperative) graphs. Vertices are
    identified with integers. Modifying is very expensive, the data
    structure is targeted towards mainly being queried.  *)
      
type t
(** The type of a graph.  *)

val empty : t
(** Returns the empty graph.  *)

val size : t -> int
(** Returns the highest number of a vertex that was connected, plus one.  *)

val num_edges : t -> int
(** Returns the number of edges in the graph.  *)

val grow : t -> int -> t
(** [grow g t] makes sure [size g] will return at least [t].  *)

val connect : t -> int -> int -> t
(** [connect g v w] connects vertices [v] and [w] in [g]. Takes O(n) time.  *)

val disconnect : t -> int -> int -> t
(** [disconnect g v w] disconnects vertices [v] and [w] in [g].
    Takes O(n) time.  *) 

val deg : t -> int -> int
(** [deg g v] returns the number of neighbors of [v] in [g].  *)

val is_connected : t -> int -> int -> bool
(** [is_connected g v w] returns true if [v] and [w] are connected in [g].
    O(n)  *)

val iter_neighbors : (int -> unit) -> t -> int -> unit
(** [iter_neighbors f g v] calls [f w] for each neighbor [w] of [v] in [g].  *)

val fold_neighbors : ('a -> int -> 'a) -> 'a -> t -> int -> 'a
(** [iter_neighbors f x g v] is [f (...(f x v1) ... vn)] for each
    neighbor [vi] of [v] in [g].  *)

val neighbor : t -> int -> int -> int
(** [neighbor g v i] returns the [i]-th neighbor of [v] in [g].  *)

val iter_edges : (int -> int -> unit) -> t -> unit
(** [iter_edges f g] calls [f u v] for each edge [(u, v)] in [g].  *)

val fold_edges : ('a -> int -> int -> 'a) -> 'a -> t -> 'a

val delete_vertex : t -> int -> t
(** [delete_vertex g v] removes all edges involving [v] in [g]. [size] is
    unchanged.  *)

val subgraph : t -> BitVec.t -> t
(** [subgraph g s] returns a copy of [g] where all edges where not both end
    points are in [s]  are removed.  *)

val output : out_channel -> t -> unit
(** [output c g] prints a debug representation of [g] to channel [c].  *)

val print : t -> unit
(** [print g] is the same as [output stdout g].  *)

val dump : t -> unit
(** [dump g] is the same as [output stderr g].  *)
