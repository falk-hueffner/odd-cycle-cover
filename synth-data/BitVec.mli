(** Module for handling (imperative) bit vectors.  *)

(** The type of a bit vector.  *)
type t
(** [create n] returns a new bit vector of length [n] with all bits
    set to 0.  *)
val create : int -> t
(** [create_filled n] returns a new bit vector of length [n] with all bits
    set to 1.  *)
val create_filled : int -> t
(** Returns a copy of a bit vector.  *)
val copy : t -> t
(** Returns the length of a bit vector.  *)
val length : t -> int
(** Behaves like [Pervasives.compare], for an unspecified total ordering
    on bit vectors.  *)
val compare : t -> t -> int
(** [is_set s n] returns [true] iff bit [n] is set in [s].  *)
val is_set : t -> int -> bool
(** [set s n] sets bit [n] in [s] to 1.  *)
val set : t -> int -> unit
(** [set s n] sets bit [n] in [s] to 0.  *)
val unset : t -> int -> unit
(** [set s n v] sets bit [n] in [s] to [v].  *)
val put : t -> int -> bool -> unit
(** [toggle s n v] behaves like [put s n (not (get s n))].  *)
val toggle : t -> int -> unit
(** [iter f s] calls [f n] for each [n] where [s] has a 1-bit .  *)  
val iter : (int -> unit) -> t -> unit
val not_found : int
(** [find_0 s n] returns the first position in [s] larger or equal
    than [n] that contains a 0-bit, or [not_found] if no such position
    exists.  *)
val find_0 : t -> int -> int
(** Returns the number of 1-bits in the set.  *)
val count : t -> int
(** [resize s n] returns a copy of [s] of size [n], with bits cropped
    or padded with zero.  *)
val resize : t -> int -> t
(** Returns an array containing the posisions of the 1-bits.  *)
val to_array : t -> int array
(** Returns a copy of the input bit vector with 1-bits replaced by
    0-bits and vice versa.  *)
val inverse : t -> t
(** [setminus s1 s2] returns a copy of [s1] with positions that have a
    1-bit in [s2] cleared.  *)
val setminus : t -> t -> t
(** [union s1 s2] returns a copy of the larger of [s1] and [s2] with
    positions that have a 1-bit in the other set to 1.  *)
val union : t -> t -> t
(** [output c s] prints a debug representation of [s] to channel [c].  *)
val output : out_channel -> t -> unit
(** [print s] is the same as [output stdout s].  *)
val print : t -> unit
(** [dump s] is the same as [output stderr s].  *)
val dump : t -> unit
