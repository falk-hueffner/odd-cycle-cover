(** Module for handling undirected (imperative) graphs. Vertices are
    identified with integers. Modifying is very expensive, the data
    structure is targeted towards mainly being queried.  *)
      
type t
(** The type of a graph.  *)

val create : unit -> t
(** Returns a new graph.  *)

val size : t -> int
(** Returns the highest number of a vertex that was connected, plus one.  *)

val copy : t -> t
(** Returns a copy of a graph.  *)

val connect : t -> int -> int -> unit
(** [connect g v w] connects vertices [v] and [w] in [g]. O(n^2)  *)

val is_connected : t -> int -> int -> bool
(** [is_connected g v w] returns true if [v] and [w] are connected in [g].
    O(n)  *)

val iter_neighbors : (int -> unit) -> t -> int -> unit
(** [iter_neighbors f g v] calls [f w] for each neighbor [w] of [v] in [g].  *)

val neighbors_array : t -> int -> int array
(** [neighbors_array g v] returns an array with the neighbors of [v]
    in [g].  *)

val iter_edges : (int -> int -> unit) -> t -> unit
(** [iter_edges f g] calls [f u v] for each edge [(u, v)] in [g].  *)

val delete_vertex : t -> int -> unit
(** [delete_vertex g v] removes all edges involving [v] in [g]. [size] is
    unchanged.  *)

val subgraph : t -> BitVec.t -> t
(** [subgraph g s] returns a copy of [g] where all edges where not both end
    points are in [s]  are removed.  *)
