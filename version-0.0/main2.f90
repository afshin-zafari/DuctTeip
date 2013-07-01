program main

	use wfm_class

	use problem_class
	use approximation_class
	use class_point_set
	use fp
	use timer
	
	implicit none
	
	type(wfm) :: w
	type(op), dimension(1)	:: ops
	
	type(problem)			:: prob
	type(geometry)			:: geom
	type(expression)		:: pde
	
	type(approximation)		:: approx
	type(epsilon)			:: eps
	
	integer(kind=8)			:: time
	
	call tl_init(1)
	
	! 1 - Use interior eq, 0 - Use boundary eq
	
	! boxgeom256.dat
	call geometry_new(geom, (/0,0,0,0,1,1,1,1/))
	
	! boxgeom.dat
	! call geometry_new(geom, (/0,0,0,0,1,1,1,1/))
	
	! new_geom.dat
	! call geometry_new(geom, (/0,0,0,0,0,0,1,1,1,1/))
	
	call expression_new(pde)
	call problem_new(prob, geom, pde)
	
	eps = new_eps(.false., (/2.0_rfp, 2.0_rfp/), 1)
	call approximation_new(approx, "gauss", eps, "input/boxgeom256.dat")
	! call approximation_new(approx, "gauss", eps, "input/boxgeom.dat")
	! call approximation_new(approx, "gauss", eps, "input/new_geom.dat")
	
	call op_assemble(ops(1), prob, approx, "Result")

	call wfm_new(w, 1)
	call wfm_add(w, ops(1))
	time = getime()
	call wfm_execute(w, (/ "Result" /))
	time = getime()-time
	! call wfm_execute(w, (/ "Result_dist_1_3" /))

	! call wfm_print_dm(w)

	call tl_destroy()
	
	write(*,*) "Done. ", time
	
end program main
