type t = string;;

let nul = '\000';;
let one = '\001';;

let create n = String.make n nul;;
let create_filled n = String.make n one;;
let copy     = String.copy;;

let length   = String.length;;
let compare  = String.compare;;

let is_set s n   = s.[n] <> nul;;
let set    s n   = s.[n] <- one;;
let unset  s n   = s.[n] <- nul;;
let put    s n b = s.[n] <- if b then one else nul;;
let toggle s n   = s.[n] <- if s.[n] <> nul then nul else one;;

let iter f s =
   for i = 0 to (length s) - 1 do
     if is_set s i then f i
   done
;;

let not_found = -1;;

let find_0 s p =
  let rec loop p =
    if p >= (length s)
    then not_found
    else if not (is_set s p)
    then p
    else loop (p + 1)
  in
    loop p
;;

let count s =
  let rec loop accu n =
    if n >= (length s)
    then accu
    else loop (accu + (if is_set s n then 1 else 0)) (n + 1)
  in
    loop 0 0
;;

let resize s l =
  let r = create l in
    iter (set r) s;
    r;
;;

let to_array s =
  let n = count s in
  let a = Array.make n 0 in
  let i = ref 0 in
    iter (fun x -> a.(!i) <- x; incr i) s;
    a;
;;

let inverse s =
  let r = copy s in
    for i = 0 to (length s) - 1 do
      toggle r i
    done;
    r
;;

let setminus s1 s2 =
  let r = create (length s1) in
    for i = 0 to (min (length s1) (length s2)) - 1 do
      put r i (is_set s1 i && not (is_set s2 i))
    done;
    r;
;;

let union s1 s2 =
  let r = create (max (length s1) (length s2)) in
    for i = 0 to (min (length s1) (length s2)) - 1 do
      put r i (is_set s1 i || is_set s2 i)
    done;
    r;
;;

let intersection s1 s2 =
  let r = create (max (length s1) (length s2)) in
    for i = 0 to (min (length s1) (length s2)) - 1 do
      put r i (is_set s1 i && is_set s2 i)
    done;
    r;
;;

let output channel s =
  Printf.fprintf channel "[%d:" (count s);
  iter (fun x -> Printf.fprintf channel " %d" x) s;
  output_char channel ']';
;;

let print g = output stdout g;;
let dump g = output stderr g;;
