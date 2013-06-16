#define mat(a,b,c) #a,b,c
program main 

  use mpi 
  use cholesky_taskgen
  

  type(matrix_block),pointer  :: M
  type(remote_access) :: rma
  type(task_gen),pointer::tg
  integer :: c,n,s,err,i,j,max_i,max_j,p,q
  character(len=5):: ps,qs,ns

  call getarg(1,ns)
  call getarg(2,ps)
  call getarg(3,qs)
  read (ps,"(I3)")  p
  read (ns,"(I3)")  n
  read (qs,"(I3)")  q

  call MPI_INIT(err)
  write (*,*) n,p,q
  rma%node_cnt = n 
  call chol_task_gen(n,p,q)
  call MPI_FINALIZE(err)
  stop
  i = 0 
  j = 0 
  max_i = 1000000
  max_j = 1000000
  err = 1
  do while ( err /= 0 .and. j < max_j) 
     tg =>taskgen_init(n)
     call set_stages(tg,data_stage=i,task_stage=j)
     err = task_gen_loop(rma,tg)
     call dt_schedule(tg,err,max_i,max_j)
     write (*,*) "max_i,j:",max_i,max_j
     call taskgen_finish(tg)
     i= i + 1
  end do
  err =1 
  max_j = min(max_j,5)
  max_i = min(max_i,5)
  do j= 0,max_j 
     write (*,*) "max_i,j:",max_i,max_j
     do i=1,max_i
        write (*,*) "i,j:",i,j
        tg =>taskgen_init(n)
        call set_stages(tg,data_stage=i,task_stage=j)
        err = task_gen_loop(rma,tg)
        call dt_schedule(tg,1,max_i,max_j)
        call taskgen_finish(tg)
     end do
  end do

  call MPI_FINALIZE(err)
end program

