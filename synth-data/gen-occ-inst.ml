let rec find_rand_int n pred =
  let x = Random.int n in
    if pred x then x
    else find_rand_int n pred
;;

let round x = int_of_float (x +. 0.5);;

let random_nearly_bipartite_graph n deg occ =
  let n1 = (n - occ) / 2 in
  let n2 = n - n1 - occ in
  let is_occ v = v < occ in
  let is_n1 v  = v < occ + n1 in
  let can_connect v1 v2 = is_occ v1 || is_occ v2 || is_n1 v1 <> is_n1 v2 in    
  let saturated g v =
    if is_occ v
    then Graph.deg g v >= n1 + n1 + occ - 1
    else if is_n1 v
    then Graph.deg g v >= n2      + occ - 1
    else Graph.deg g v >=      n1 + occ - 1 in    
  let rec loop g m =
    if m = 0 then g
    else
      let v1 = find_rand_int n (fun v -> not (saturated g v)) in
      let v2 = find_rand_int n (fun v2 -> v2 <> v1 && can_connect v1 v2
				  && not (Graph.is_connected g v1 v2)) in
	loop (Graph.connect g v1 v2) (pred m)
  in
    loop (Graph.grow Graph.empty n) (round (((float_of_int n) *. deg) /. 2.0))
;;
	
let () =
  if Array.length Sys.argv <> 1 + 3
  then Printf.fprintf stderr
    "Usage: %s vertices avg-deg occ-size\n" Sys.argv.(0)
  else begin
    Random.self_init();
    let n = int_of_string(Sys.argv.(1)) in
    let deg = float_of_string(Sys.argv.(2)) in
    let occ = int_of_string(Sys.argv.(3)) in
    let g = random_nearly_bipartite_graph n deg occ
    in
      Printf.printf "# |V| = %d, |E| = %d, |OCC| <= %d\n"
	(Graph.size g) (Graph.num_edges g) occ;
      Graph.iter_edges (fun i j -> Printf.printf "%d %d\n" i j) g;
  end    
;;
