let round x = int_of_float (x +. 0.5);;

type snp = A | B | Dunno;;

type haplotype = {
  name: string;
  data: snp array;
  mutable mutated: bool;
};;

let output_haplotype channel s =
  Printf.fprintf channel "%s%s = " s.name (if s.mutated then "*" else "");
  Array.iter (fun x ->
                output_char channel
		  (match x with A -> 'A' | B -> 'B' | Dunno -> '-')) s.data;
;;

let random_snp () = if Random.bool () then A else B;;

let gen_fragments h k =
  let n = Array.length h.data in
  let bps = Array.make (n + 1) false in
  bps.(n) <- true;
  let nbps = ref (k - 1) in
    while !nbps > 0 do
      let p = (Random.int n) + 1 in
	if not bps.(p)
	then begin
	  bps.(p) <- true;
	  decr nbps;
	end;
    done;
    let result = ref [] in
    let p0 = ref 0 in
      for p1 = 1 to n do
	if bps.(p1)
	then begin
	  let h' = Array.init n (fun p -> if p >= !p0 && p < p1 then h.data.(p) else Dunno) in
	    result := {
	      name = h.name ^ "_" ^ (string_of_int !p0);
	      data = h';
	      mutated = false;
	    } :: !result;
	    p0 := p1;
	end
      done;
      assert (List.length !result = k);
      !result;
;;

let mutate_fragment p s =
  let did_mutate = ref false in
    for i = 0 to (Array.length s.data) - 1 do
      if s.data.(i) <> Dunno && Random.float 1.0 < p
      then begin
	s.data.(i) <- if s.data.(i) = A then B else A;
	s.mutated <- true;
      end
    done;
;;

let snps_conflict s1 s2 =
  if s1 = Dunno || s2 = Dunno
  then false
  else s1 <> s2
;;

let fragments_compatible f1 f2 =
  assert (Array.length f1.data = Array.length f2.data);
  let compatible = ref true in
    for i = 0 to (Array.length f1.data) - 1 do
      if snps_conflict f1.data.(i) f2.data.(i) then compatible := false
    done;
    !compatible;
;;    

let main n d c k p =
  let h1 = {
    name = "h1";
    data = Array.init n (fun _ -> random_snp ());
    mutated = false; } in
  let h2 = {
    name = "h2";
    data = Array.copy h1.data;
    mutated = false; } in
  let flips = ref (round ((float_of_int n) *. d)) in
    while !flips > 0 do
      let p = Random.int n in
	if h1.data.(p) = h2.data.(p)
	then begin
          h2.data.(p) <- if h2.data.(p) = A then B else A;
          decr flips;
	end
    done;
    Printf.printf "# n = %d d = %f c = %d k = %d p = %f\n" n d c k p;
    Printf.printf "# %a\n" output_haplotype h1;
    Printf.printf "# %a\n" output_haplotype h2;    
    let fragments = ref [] in
      for i = 0 to c - 1 do
	let h1' = { name = "h1_" ^ string_of_int i; data = h1.data; mutated = false } in
	let h2' = { name = "h2_" ^ string_of_int i; data = h1.data; mutated = false } in
	fragments := !fragments @ (gen_fragments h1' k);
	fragments := !fragments @ (gen_fragments h2' k);
      done;
      List.iter (mutate_fragment p) !fragments;
(*       List.iter (fun h -> output_haplotype stdout h; print_newline ()) !fragments; *)
      let mutfrags = List.fold_left
	(fun mutfrags f -> if f.mutated then f.name::mutfrags else mutfrags) [] !fragments in
      Printf.printf "# %d fragments\n" (List.length !fragments);
      Printf.printf "# %d mutations:\n#" (List.length mutfrags);
      List.iter (fun s -> Printf.printf " %s" s) (List.rev mutfrags);
      print_newline ();
      let fragments = Array.of_list !fragments in
	for i = 0 to (Array.length fragments) - 1 do
	  for j = i + 1 to (Array.length fragments) - 1 do
	    if not (fragments_compatible fragments.(i) fragments.(j))
	    then Printf.printf "%s %s\n" fragments.(i).name fragments.(j).name
	  done
	done
;;

let () =
  (match Array.length Sys.argv with
       6 -> Random.self_init ()
     | 7 -> (let seed = int_of_string Sys.argv.(6) in
	       Random.init seed;
	       Printf.printf "# random seed = %d\n" seed)
     | _ -> (Printf.fprintf stderr "usage: %s n d c k p [random-seed]\n"
	       Sys.argv.(0); exit 1));
  let n = int_of_string Sys.argv.(1) in
  let d = float_of_string Sys.argv.(2) in
  let c = int_of_string Sys.argv.(3) in
  let k = int_of_string Sys.argv.(4) in
  let p = float_of_string Sys.argv.(5) in
    main n d c k p
;;

