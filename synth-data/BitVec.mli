(** Module for handling (imperative) bit vectors.  *)

type t
(** The type of a bit vector.  *)

val create : int -> t
(** [create n] returns a new bit vector of length [n] with all bits
    set to 0.  *)

val create_filled : int -> t
(** [create_filled n] returns a new bit vector of length [n] with all bits
    set to 1.  *)

val copy : t -> t
(** Returns a copy of a bit vector.  *)

val length : t -> int
(** Returns the length of a bit vector.  *)

val compare : t -> t -> int
(** Behaves like [Pervasives.compare], for an unspecified total ordering
    on bit vectors.  *)

val is_set : t -> int -> bool
(** [is_set s n] returns [true] iff bit [n] is set in [s].  *)

val set : t -> int -> unit
(** [set s n] sets bit [n] in [s] to 1.  *)

val unset : t -> int -> unit
(** [set s n] sets bit [n] in [s] to 0.  *)

val put : t -> int -> bool -> unit
(** [set s n v] sets bit [n] in [s] to [v].  *)

val toggle : t -> int -> unit
(** [toggle s n v] behaves like [put s n (not (get s n))].  *)

val not_found : int
val iter : (int -> unit) -> t -> unit
(** [iter f s] calls [f n] for each [n] where [s] has a 1-bit .  *)  

val find_0 : t -> int -> int
(** [find_0 s n] returns the first position in [s] larger or equal
    than [n] that contains a 0-bit, or [not_found] if no such position
    exists.  *)

val count : t -> int
(** Returns the number of 1-bits in the set.  *)

val resize : t -> int -> t
(** [resize s n] returns a copy of [s] of size [n], with bits cropped
    or padded with zero.  *)

val to_array : t -> int array
(** Returns an array containing the posisions of the 1-bits.  *)

val inverse : t -> t
(** Returns a copy of the input bit vector with 1-bits replaced by
    0-bits and vice versa.  *)

val setminus : t -> t -> t
(** [setminus s1 s2] returns a copy of [s1] with positions that have a
    1-bit in [s2] cleared.  *)

val union : t -> t -> t
(** [union s1 s2] returns a copy of the larger of [s1] and [s2] with
    positions that have a 1-bit in the other set to 1.  *)

val intersection : t -> t -> t
(** [intersection s1 s2] returns a set the size of the larger of [s1]
    and [s2] with 1-bits in positions where both sets have a 1-bit.  *)

val output : out_channel -> t -> unit
(** [output c s] prints a debug representation of [s] to channel [c].  *)

val print : t -> unit
(** [print s] is the same as [output stdout s].  *)

val dump : t -> unit
(** [dump s] is the same as [output stderr s].  *)
