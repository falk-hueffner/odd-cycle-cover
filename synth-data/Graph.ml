module Array = FArray;;

type t = int Array.t Array.t;;

let empty = Array.empty;;
let size = Array.length;;
let grow g l = Array.grow g l empty;;

let deg g v = Array.length g.(v);;

let is_connected g v u = v < size g && Array.mem g.(v) u;;

let connect g u v =
  assert (u <> v);
  if is_connected g u v
  then g
  else
    let g = grow g ((max u v) + 1) in
    let g = g.(u) <- Array.append g.(u) v in
      g.(v) <- Array.append g.(v) u
;;

let iter_neighbors f g v = Array.iter f g.(v);;
let fold_neighbors f x g v = Array.fold_left f x g.(v);;
let iter_neighbors' f g v = f v; Array.iter f g.(v);;

let neighbors_array g v = g.(v);;

let iter_edges f g =
  for v = 0 to (size g) - 1 do
    Array.iter (fun u -> if v < u then f v u) g.(v)
  done
;;

let fold_edges f accu g =
  let rec loop u accu =
    if u >= size g
    then accu
    else
      let accu = 
	Array.fold_left
	  (fun accu v -> if u < v then f accu u v else accu)
	  accu
	  g.(u)
      in
	loop (succ u) accu
  in
    loop 0 accu
;;

let delete_vertex g v =  
  let g = g.(v) <- Array.empty in
    Array.map (Array.filter ((<>) v)) g
;;

let subgraph g s =
  Array.init
    (size g)
    (fun u ->
       if not (BitVec.is_set s u)
       then Array.empty
       else Array.filter (BitVec.is_set s) g.(u))
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
