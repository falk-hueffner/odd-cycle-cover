type 'a t = 'a array;;

let length = Array.length;;
let get = Array.get;;
let unsafe_get = Array.unsafe_get;;
let iter = Array.iter;;
let fold_left = Array.fold_left;;
let empty = [| |];;
let init = Array.init;;
let of_list = Array.of_list;;

let mem a x =
  let rec loop i =
    if i >= length a
    then false
    else a.(i) = x || loop (succ i)
  in
    loop 0
;;

let set a i x = let a = Array.copy a in Array.get a i x; a;;
let unsafe_set a i x = let a = Array.copy a in Array.unsafe_set a i x; a;;

let grow a l x =
  if length a >= l
  then a
  else Array.init l (fun i -> if i < length a then a.(i) else x)
;;

let append a x =
  let l = length a in
    Array.init (succ l) (fun i -> if i < l then a.(i) else x)
;;

let map f a = Array.init (length a) (fun i -> f a.(i));;

let filter f a =
  let l = fold_left (fun l x -> if f x then x :: l else l) [] a in
    of_list (List.rev l)
;;

let remove a x = filter ((<>) x) a;;
