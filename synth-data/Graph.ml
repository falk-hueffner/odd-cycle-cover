type t = {
  mutable edges: int array array;
};;

let create () = { edges = [||] };;

let size g = Array.length g.edges;;

let copy g =
  { edges = Array.init (size g) (fun i -> Array.copy (g.edges.(i))) }
;;

let array_append a x =
  let l = Array.length a in
    Array.init (l + 1) (fun i -> if i < l then a.(i) else x)
;;

let array_contains a x =
  let rec loop i =
    if i >= Array.length a
    then false
    else if a.(i) = x
    then true
    else loop (succ i)
  in
    loop 0
;;

let array_filter f a =
  let l = Array.fold_left (fun l x -> if f x then x :: l else l) [] a in
    Array.of_list (List.rev l)
;;

let is_connected g v u = (v < size g) && array_contains (g.edges.(v)) u;;

let connect g u v =
  assert (u <> v);
  if not (is_connected g u v)
  then
    let size' = (max u v) + 1 in
      if size' > size g then
	g.edges <-
	Array.init size'
	  (fun i ->
	     if i < size g
	     then Array.copy (g.edges.(i))
	     else [||]);
      g.edges.(u) <- array_append g.edges.(u) v;
      g.edges.(v) <- array_append g.edges.(v) u;
;;

let iter_neighbors f g v = Array.iter f (g.edges.(v));;
let iter_neighbors' f g v = f v; Array.iter f (g.edges.(v));;

let neighbors_array g v = g.edges.(v);;

let iter_edges f g =
  for v = 0 to (size g) - 1 do
    Array.iter (fun u -> if v < u then f v u) g.edges.(v)
  done
;;

let delete_vertex g v =  
  g.edges.(v) <- [||];
  for i = 0 to (Array.length g.edges) - 1 do
    g.edges.(i) <- array_filter ((<>) v) g.edges.(i);
  done
;;

let subgraph g s =
  let g' = create () in
    iter_edges
      (fun v w ->
	 if BitVec.is_set s v && BitVec.is_set s w
	 then connect g' v w)
      g;
    g'
;;

let output channel g =
  Printf.fprintf channel "{ %d\n" (size g);
  for u = 0 to (size g) - 1 do
    iter_neighbors
      (fun v -> if u <= v then Printf.fprintf channel "%d %d\n" u v) g u
  done;
  Printf.fprintf channel "}\n";
;;

let print g = output stdout g;;
let dump g = output stderr g;;
