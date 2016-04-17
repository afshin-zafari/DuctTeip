program main

	use wfm_class
	
	implicit none
	
	type(wfm) :: w
	type(op), dimension(4) :: ops
	
	call tl_init(4)
	
	call op_create_matrix(ops(1), "A")
	call op_create_matrix(ops(2), "B")	
	call op_mult_matrix(ops(3), "A", "B", "C")
	call op_mult_matrix(ops(4), "A", "C", "Result")
	
	call wfm_new(w, 4)
	call wfm_add(w, ops(1))
	call wfm_add(w, ops(2))
	call wfm_add(w, ops(3))
	call wfm_add(w, ops(4))

	call wfm_execute(w, (/ "Result" /))
	call wfm_print_dm(w)
	
end program main