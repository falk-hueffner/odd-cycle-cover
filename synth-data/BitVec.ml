type t = {
  mutable size: int;
  mutable data: string;
}

exception Negative_index of string

(* Unsafe versions  *)
(*
external bool_of_01 : int -> bool = "%identity"
external int_of_bool : bool -> int = "%identity"
external char_of_byte : int -> char = "%identity"
let get_byte s pos = int_of_char (String.unsafe_get s pos);;
let set_byte s pos b = String.unsafe_set s pos (char_of_int b);;
*)

(* Safe versions.  *)
let bool_of_01 = function 0 -> false | 1 -> true | _ -> failwith "bool_of_01";;
let int_of_bool = function false -> 0 | true -> 1;;
let char_of_byte = char_of_int;;
let get_byte s pos = int_of_char (String.get s pos);;
let set_byte s pos b = String.set s pos (char_of_int b);;

let byte_len n = (n + 7) lsr 3;;

let empty () =
  { size = 0;
    data = ""; }
;;

let create n =
  let n = if n < 0 then 0 else n in
    { size = n;
      data = String.make (byte_len n) (char_of_int 0) }
;;

let create_filled n =
  let n = if n < 0 then 0 else n in
  let s =
    { size = n;
      data = String.make (byte_len n) (char_of_int 0xff) } in
 let last = (byte_len n) - 1 in
 let byte = get_byte s.data last in
 let byte = byte land ((1 lsl (n land 0b111)) - 1) in
   set_byte s.data last byte;
   s;
;;

let copy s =
  { size = s.size;
    data = String.copy s.data; }
;;

let clone = copy;;

let grow s n =
  let bytes = byte_len n in
  let s_bytes = byte_len s.size in
    s.size <- n;
    if bytes > s_bytes
    then
      s.data <-
        let d =
	  String.create bytes
	in
	  String.blit s.data 0 d 0 s_bytes;
	  String.fill d s_bytes (bytes - s_bytes) (char_of_int 0);
	  d;
;;

let set s n =
  if n < 0 || n >= s.size
  then raise (Invalid_argument "BitSet.set: index out of bounds");
  let mask = 1 lsl (n land 0b111) in
  let bytepos = n lsr 3 in
  let byte = get_byte s.data bytepos in
    set_byte s.data bytepos (byte lor mask)
;;

let unset s n =
  if n < 0 || n >= s.size
  then raise (Invalid_argument "BitSet.unset: index out of bounds");
  let mask = 1 lsl (n land 0b111) in
  let bytepos = n lsr 3 in
  let byte = get_byte s.data bytepos in
    set_byte s.data bytepos (byte land (lnot mask))
;;

let toggle s n =
  if n < 0 || n >= s.size
  then raise (Invalid_argument "BitSet.toggle: index out of bounds");
  let mask = 1 lsl (n land 0b111) in
  let bytepos = n lsr 3 in
  let byte = get_byte s.data bytepos in
    set_byte s.data bytepos (byte lxor mask)
;;

let put s b n =
  if n < 0 || n >= s.size
  then raise (Invalid_argument "BitSet.put: index out of bounds");
  let shift = n land 0b111 in
  let mask = 1 lsl shift in
  let mask2 = (int_of_bool b) lsl shift in
  let bytepos = n lsr 3 in
  let byte = get_byte s.data bytepos in
    set_byte s.data bytepos ((byte land (lnot mask)) lor mask2)
;;

let is_set s n =
  if n < 0 || n >= s.size
  then raise (Invalid_argument "BitSet.put: index out of bounds");
  let shift = n land 0b111 in
  let bytepos = n lsr 3 in
    bool_of_01 (((get_byte s.data bytepos) lsr shift) land 1)
;;

let bitcounts = [|
  0; 1; 1; 2; 1; 2; 2; 3; 1; 2; 2; 3; 2; 3; 3; 4;
  1; 2; 2; 3; 2; 3; 3; 4; 2; 3; 3; 4; 3; 4; 4; 5;
  1; 2; 2; 3; 2; 3; 3; 4; 2; 3; 3; 4; 3; 4; 4; 5;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  1; 2; 2; 3; 2; 3; 3; 4; 2; 3; 3; 4; 3; 4; 4; 5;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  3; 4; 4; 5; 4; 5; 5; 6; 4; 5; 5; 6; 5; 6; 6; 7;
  1; 2; 2; 3; 2; 3; 3; 4; 2; 3; 3; 4; 3; 4; 4; 5;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  3; 4; 4; 5; 4; 5; 5; 6; 4; 5; 5; 6; 5; 6; 6; 7;
  2; 3; 3; 4; 3; 4; 4; 5; 3; 4; 4; 5; 4; 5; 5; 6;
  3; 4; 4; 5; 4; 5; 5; 6; 4; 5; 5; 6; 5; 6; 6; 7;
  3; 4; 4; 5; 4; 5; 5; 6; 4; 5; 5; 6; 5; 6; 6; 7;
  4; 5; 5; 6; 5; 6; 6; 7; 5; 6; 6; 7; 6; 7; 7; 8;
|];;

let count s =
  let sum = ref 0 in
    for i = 0 to pred (byte_len s.size) do
      sum := !sum + bitcounts.(get_byte s.data i)
    done;
    !sum;
;;

let trailing_zeros = [|
  8; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  5; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  6; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  5; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  7; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  5; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  6; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  5; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
  4; 0; 1; 0; 2; 0; 1; 0; 3; 0; 1; 0; 2; 0; 1; 0;
|];;

let not_found = -1;;

let find s n =
  if n < 0 then raise (Invalid_argument "BitSet.find: negative index");
  if n >= s.size
  then not_found
  else
    let len = byte_len s.size in
    let rec loop bytepos byte =
      if byte <> 0
      then 8 * bytepos + trailing_zeros.(byte)
      else if bytepos >= len - 1
      then not_found
      else loop (succ bytepos) (get_byte s.data (succ bytepos)) in
    let bytepos = n lsr 3 in
    let byte = get_byte s.data bytepos in
    let byte = byte land (-1 lsl (n land 0b111)) in
      loop bytepos byte
;;

let find_0 s n =
  if n < 0 then raise (Invalid_argument "BitSet.find_0: negative index");
  if n >= s.size
  then not_found
  else
    let len = byte_len s.size in
    let rec loop bytepos byte =
      if byte <> 0
      then
	let p =	8 * bytepos + trailing_zeros.(byte)
	in if p >= s.size then not_found else p
      else if bytepos >= len - 1
      then not_found
      else loop (succ bytepos) ((get_byte s.data (succ bytepos)) lxor 0xff) in
    let bytepos = n lsr 3 in
    let byte = get_byte s.data bytepos in
    let byte = byte lxor 0xff in
    let byte = byte land (-1 lsl (n land 0b111)) in
      loop bytepos byte
;;

let iter f s =
  let rec loop p =
    let p = find s p in
      if p <> not_found then begin
	f p;
	loop (succ p);
      end
  in
    loop 0
;;

(* Unoptimized  *)
let resize s l =
  let r = create l in
    iter (set r) s;
    r;
;;

(* Unoptimized  *)
let inverse s =
  let r = copy s in
    for i = 0 to s.size - 1 do
      toggle r i
    done;
    r
;;

(* Unoptimized  *)
let to_array s =
  let n = count s in
  let a = Array.make n 0 in
  let i = ref 0 in
    iter (fun x -> a.(!i) <- x; incr i) s;
    a;
;;

(* Unoptimized  *)
let setminus s1 s2 =
  let r = create s1.size in
    for i = 0 to (min s1.size s2.size) - 1 do
      put r (is_set s1 i && not (is_set s2 i)) i
    done;
    r;
;;
      
(* Unoptimized  *)
let union s1 s2 =
  let r = create (max s1.size s2.size) in
    for i = 0 to (min s1.size s2.size) - 1 do
      put r (is_set s1 i || is_set s2 i) i
    done;
    r;
;;

let output channel s =
  output_char channel '[';
  let print_sep = ref false in
    iter (fun x ->
	    if !print_sep then output_char channel ' ';
	    Printf.fprintf channel "%d"x;
	    print_sep := true) s;
  output_char channel ']';
;;

let print g = output stdout g;;
let dump g = output stderr g;;

