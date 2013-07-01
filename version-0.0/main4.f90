program main

	use wfm_class

	use problem_class
	use approximation_class
	use class_point_set
	use fp
	use timer
	use distribution_class
        use parameters_class
        use mpi

	implicit none
	
    type(distribution)      :: dist
	type(wfm)               :: w
	type(op), dimension(1)	:: ops
	
	type(problem)			:: prob
	type(geometry)			:: geom
	type(expression)		:: pde
	
	type(approximation)		:: approx
	type(epsilon)			:: eps1,eps2,eps3
	
	integer(kind=8)			:: time
        type(parameters)                :: par
        type(eps_parts)                 :: eps_prts
        integer :: node_id,err
        real(kind=rfp) :: r
        

        Write ( *,* ) "sizeof one element ",sizeof(r)
        

	! 1 - Use interior eq, 0 - Use boundary eq
	
	! boxgeom256.dat
	call geometry_new(geom, (/0,0,0,0,1,1,1,1,1,1,1/))
	
	
	call expression_new(pde)
	call problem_new(prob, geom, pde)
	

        !pts = read_pts2("input/boxgeom256.dat",callme)
 
        eps1 = new_eps(.false., (/1.0_rfp, 2.0_rfp/), 3)
        par = par_new((/ "mq   " , "gauss", "iq   " ,"imq  "/),eps1, &
                     (/ "input/boxgeom256.dat          ","input/boxgeom256copy.dat      " /))
        eps2 = new_eps(.false., (/1.0_rfp, 1.0_rfp/), 1)
!!$        call par_add(par,(/ "tps2" , "tps4"/),eps2, &
!!$                     (/ "input/boxgeom256.dat    ","input/boxgeom256copy.dat" /))
!!$
!!$        eps3 = new_eps(.false., (/0.5_rfp, 0.5_rfp/), 1)
!!$        eps_prts  = par_new_eps_parts(eps3,(/ 2,3,4,5 /))
!!$        call par_add(par,(/ "r3" , "r5"/), eps_prts, &
!!$                     (/ "input/boxgeom256.dat    ","input/boxgeom256copy.dat" /))


	call approximation_new(approx, "gauss", eps1, "input/boxgeom256.dat")
	
	call op_assemble(ops(1), prob, approx, "Result")

	call wfm_new(w, 1)
	call wfm_add(w, ops(1))
        dist = dist_new(w,par)        
	time = getime()
        call dist_run(dist)
	time = (getime()-time)

	call tl_destroy()
	
	write(*,*) "Done. ", time/1.e6
end program main
