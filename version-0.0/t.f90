
program main
    use mpi
        Integer :: err,r,p

        Call MPI_INIT(err)
        Call MPI_COMM_RANK( MPI_COMM_WORLD, r  , err )
        Call MPI_COMM_SIZE( MPI_COMM_WORLD, p, err )
        write (*,*) " I am " , r , " from ", p
        Call MPI_FINALIZE(err)
        
end program
