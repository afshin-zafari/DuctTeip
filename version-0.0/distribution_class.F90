# 1 "distribution_class.F90"

!!$------------------------------------------------------------------------------------------------------------------------------
!!$ External Dependencies
!!$      Approximation Class		Used for reconstructing the Workflow components after communications finished
!!$      Parameters Class		Determines the various Parametrs related to the Hybrid-Hetero parallelization
!!$      Partition Manager Class	Handles partitioning the Point Sets of a problem
!!$      Point Set Class		Point Set structure and methods
!!$      Resources Class		Determines the available computation resources in the network
!!$      Options Class			Determines by which options the program to be run
!!$      Constants			Constant identifiers like Length and others.
!!$      Workflow Manager Class		Handles workflow construction and execution
!!$      Operations Class		Handles Operations components of the workflow
!!$      MPI Library			MPI Data types and functions definitions
!!$      
!!$      
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ List of Types 
!!$     distribution
!!$     	structure for holding all the required information during the program execution
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ List of procedures
!!$
!!$        				Public 
!!$
!!$     Subroutine dist_chunk_read		Passed to the Point Set file reader to be called back after each chunk
!!$     Subroutine dist_read_pts_chunk		Starts reading the Popint Sets data chunk by chunk
!!$     Subroutine dist_do_all_large_probs	Loops through all Large Problems and solves them one by one
!!$     Function   dist_large_prob		Solves a single Large Problem using Distributed Task Scheduling
!!$     Function   dist_large_prob_old          Solves a single Large Problem using MPI explicit communications
!!$     Function   dist_new			Constructor of the Dsitribution class
!!$     Subroutine dist_extract_parameters	Extract specified Parameters for controlling the execution
!!$	Subroutine dist_get_next_par_set	Whenever called, returns the next Parameter set to be considered for a problem
!!$	Subroutine dist_extract_options		Extract various Options of the execution
!!$  	Subroutine dist_extract_resources	Extract or asked from nodes the available computing resources for them
!!$     Subroutine dist_init			Initializes the Distribution object
!!$     Subroutine dist_finish			Finializes the Distribtion activities
!!$     Subroutine dist_save_results		Saves the Result, if any
!!$     Subroutine dist_run			First solves all Large problems and then loops through all other problems
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ 					How Does It Work
!!$	Initialization:
!!$		- Its constructor is called externally by the main program.
!!$		- The workflow that is created in the main program by the end-user is passed in to be used anywhere needed.
!!$		- The various parameter settings are passed in externally as well.
!!$
!!$	Execution: ( All the processes run in the same way)
!!$		- According to the Parameter set passed in, extract list of different parameters for the problem.
!!$		- Using the parametrs list, Large problems are being solved first with group of process.
!!$		- For any small problem, every process solves a single problem independently.
!!$------------------------------------------------------------------------------------------------------------------------------

Module distribution_class 
    
    Use remote_access_class
    Use approximation_class
    Use parameters_class
    Use parts_mngr_class
    Use class_point_set
    Use resources_class
    Use options_class
    Use constants
    Use wfm_class
    Use op_class
    Use mpi

    Implicit None

    Integer,Parameter  :: ROOT = 0

    Type distribution
       Type(partition_mngr)  :: pmngr
       Type(remote_access)   :: rma
       Type(parameters)      :: par
       Type(resources)       :: res
       Type(options)         :: opt
       Type(wfm)	     :: wf
       Integer 		     :: rank , n_proc , cur_par_idx
    End Type distribution

 Contains 
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ This procedure is called by the Point Set class read_pts2 method. That method will read Point Sets chunk by chunk and call 
!!$ this procedure after each chunk is read. 
!!$ Input/Output arguments 'arg1' and 'arg2' of this procedure are pointers to Pts and
!!$ Distribution classes, respectively. These arguments have been passed already when the 'read_pts_part' subroutine from the  
!!$ Point Set class is called.
!!$ These arguments have to be casted to the correposnding objects(using 'Transfer' intrinsic Function) before passing to the other
!!$ procedures that need them. After returning from other procedures, like 'pmgr_send' , these arguments have to be casted back 
!!$ again to reflect any new changes made into them inside the other procedures.
!!$
!!$ 'flag' is an integer shows that how many Point types are included in this read chunk. If it is greater than 1, it indicates 
!!$ that the partition spans two or more Point Type domains. 
!!$------------------------------------------------------------------------------------------------------------------------------
    Subroutine dist_chunk_read(arg1,arg2,flag)

      Character(len=1)	,Dimension(:)	,Intent(inout)	:: arg1,arg2
      Real(kind=rfp)	,Dimension(:,:)	,Pointer 	:: ptr        
      Integer 		,Dimension(1:2)			:: rg,sp
      Type(distribution)				:: this
      Type(point_set)					:: pts
      Integer 						:: np ,nd,nt,id,f,t,Type,i,j,k,l,flag

      pts  = Transfer ( arg1, pts  )
      this = Transfer ( arg2, this )


# 136


      
      Call pmgr_send(this%pmngr,pts,flag)

      arg1 = Transfer ( pts , arg1 )
      arg2 = Transfer ( this, arg2 )

      

    End Subroutine dist_chunk_read

    Subroutine  dist_read_pts_chunk(this,fname,ch,wt)

        Character(len=1) ,Dimension(:),Pointer  :: arg2
        Character(len=*) ,Intent(in)		:: fname
        Type(distribution)			:: this
        Type(point_set)				:: pts
        Integer ,Intent(in)			:: ch 
        Logical ,Intent(in)			:: wt 

        Allocate(arg2(1:sizeof(this)))
        arg2 = Transfer(this,arg2)

        

        Call read_pts_part(pts,fname,ch,wt,callback = dist_chunk_read,obj = arg2)

        this = Transfer(arg2,this)

    End Subroutine dist_read_pts_chunk

    Subroutine dist_do_all_large_probs(this) 

        Type(distribution),Intent(inout)  :: this
        Logical 			  :: answer

        answer = .False.

        Do
           this%cur_par_idx = par_get_next_large_prob(this%par)

            
            If (this%cur_par_idx <= 0 ) Exit 

            answer = dist_large_prob(this)
            Call par_set_done(this%par,this%cur_par_idx)

            Exit

        End Do 

        this%cur_par_idx = 0 

    End Subroutine dist_do_all_large_probs    
    
    Function dist_large_prob(this) Result(answer)

        Integer 		,Dimension(:,:) ,Pointer  :: FINISHED
        Type(distribution)	,Intent(inout)		  :: this
        Integer			,Dimension(:)	,Pointer  :: req_arr
        Integer 		,Dimension(:,:) ,Pointer  :: sts
        Character(len=FILE_NAME_LENGTH)			  :: fname,nd_cnt
        Integer 					  :: i ,np,nd,nt,buf_len,req,err,nc=4,pc
        Type(par_item) 					  :: pitem 
        Logical 					  :: answer



        answer = .False.
        
        nc = this%n_proc
        np = opt_get_option(this%opt,OPT_CHUNK_PTS)
        nt = opt_get_option(this%opt,OPT_BLOCK_CNT)
        pc = opt_get_option(this%opt,OPT_PART_CNT )
        write(*,*)  "Part  Count"," : ",pc
        this%rma =  rma_mat_assemble(this%opt,this%wf,this%rank,part_cnt=pc,part_size=10,node_cnt=nc,dim_cnt=2,np=np,nt =nt )
        answer = .True.



      End Function dist_large_prob

    Function dist_large_prob_old(this) Result(answer)

        Integer 		,Dimension(:,:) ,Pointer  :: FINISHED
        Type(distribution)	,Intent(inout)		  :: this
        Integer			,Dimension(:)	,Pointer  :: req_arr
        Integer 		,Dimension(:,:) ,Pointer  :: sts
        Character(len=FILE_NAME_LENGTH)			  :: fname,nd_cnt
        Integer 					  :: i ,np,nd,nt,buf_len,req,err,nc=4
        Type(par_item) 					  :: pitem 
        Logical 					  :: answer

        answer = .False.
!!$        
!!$        answer = .False.
!!$        pitem = par_get_params(this%par,this%cur_par_idx)
!!$        fname = pitem%pts
!!$
!!$        Call pts_get_info(fname ,np,nd,nt)
!!$
!!$        
!!$        
!!$        
!!$        this%pmngr=  pmgr_new( this%wf, this%rank, this%n_proc, this%opt%CHUNK_PTS, nd, nt) 
!!$
!!$        Allocate ( FINISHED( 1:nt              , 1:ID_COL_MAX    ))
!!$        Allocate ( req_arr ( 1:this%n_proc-1                     ))
!!$        Allocate ( sts     ( 1:MPI_STATUS_SIZE , 1:this%n_proc-1 ))
!!$
!!$        If ( this%rank == ROOT ) Then 
!!$
!!$             Call dist_read_pts_chunk(this,fname,this%opt%CHUNK_PTS,.False.)
!!$
!!$             buf_len = nt * Ubound(FINISHED,2) * sizeof(FINISHED(1,1))
!!$
!!$             Call pmgr_wait_for_recv(this%pmngr)
!!$             FINISHED = 0 
!!$
!!$             
!!$
!!$             Do i = 1,this%n_proc-1
!!$                Call MPI_ISEND( FINISHED, buf_len, MPI_BYTE, i, TAG_IDS_BLOCK, MPI_COMM_WORLD, req, err )
!!$                req_arr(i) = req
!!$             End Do
!!$
!!$             
!!$             
!!$
!!$             Call MPI_WAITALL(this%n_proc-1,req_arr,sts,err)
!!$             Call pmgr_compute(this%pmngr,.True.)
!!$
!!$             !for all results do 
!!$             !  RECV/read results
!!$             !end do
!!$
!!$        Else
!!$
!!$             
!!$             Call pmgr_recv(this%pmngr)
!!$             
!!$
!!$        End If
!!$
!!$        
!!$
!!$        Call MPI_BARRIER(MPI_COMM_WORLD,err)
!!$        Call pmgr_destruct(this%pmngr)
        
    End Function dist_large_prob_old

    Function dist_new(wf,par_set) Result(this) 	

      Type(parameters)  ,Optional   :: par_set
      Type(wfm)		,Intent(in) :: wf
      Type(distribution)	    :: this


      If ( Present(par_set) ) Then
         this%par = par_set
      End If

      this%wf   = wf
      this%rank = -1

      this%opt = opt_new("options.txt")
      
      


    End Function dist_new
 	



        Subroutine dist_extract_parameters(this)

            Type(distribution),Intent(inout)    :: this
            Type(epsilon) 			:: eps

        End Subroutine dist_extract_parameters

	Subroutine dist_get_next_par_set(this,phi,eps,filename)

            Character(len=FILE_NAME_LENGTH)	,Intent(out)   :: filename
            Type(distribution)			,Intent(inout) :: this
            Character(len=PHI_NAME_LENGTH)	,Intent(out)   :: phi
            Real(kind=rfp)			,Intent(out)   :: eps
            Type(par_item)				       :: pitem

            Do 
                pitem = par_get_params(this%par,this%cur_par_idx)

                

                phi      = pitem%phi
                eps      = pitem%eps
                filename = pitem%pts

                this%cur_par_idx = this%cur_par_idx + this%n_proc

                If ( .Not. pitem%done ) Exit 

            End Do 

        End Subroutine dist_get_next_par_set
    
	Subroutine dist_extract_options(this)

            Type(distribution),Intent(inout) :: this

	End Subroutine dist_extract_options
	
	Subroutine dist_extract_resources(this)

            Integer		,Dimension(MPI_STATUS_SIZE) 			:: sts
            Type(node)		,Dimension(1:1) 				:: pnode
            Type(distribution)					,Intent(inout)	:: this
            Character(len=20) 							:: pcode
            Integer 								:: req, err,temp,p

!!$		If ( this%opt%READ_INFO_FROM_FILE .Or. .True. )  Then 
!!$                    ! read file into this%res%clusters(1)%nodes(1:n_proc)
!!$		Else		
!!$                    this%res = res_new(this%rank)
!!$                    pnode(1) = res_get_node_info(this%res)
!!$
!!$                    If ( this%rank == ROOT )  Then
!!$
!!$                        Do p = 1,this%n_proc-1
!!$                            
!!$
!!$                            Call MPI_PROBE(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,sts,err)
!!$                            Call MPI_RECV(pnode,sizeof(pnode),MPI_BYTE,sts(MPI_SOURCE),0,MPI_COMM_WORLD,sts,err)
!!$
!!$                            this%res%clusters(1)%nodes(1) = pnode(1)
!!$
!!$                            
!!$                            
!!$                            
!!$ 
!!$                       End Do
!!$
!!$                    Else
!!$                        ! send res%clusters(1)%nodes(1) to ROOT
!!$                        pnode(1) = this%res%clusters(1)%nodes(1)
!!$                        Call MPI_SEND(pnode,sizeof(pnode), MPI_BYTE,ROOT,0,MPI_COMM_WORLD,req,err)
!!$
!!$                        
!!$                        
!!$                        
!!$
!!$                    End If
!!$
!!$                    Call MPI_BARRIER(MPI_COMM_WORLD,err)
!!$
!!$		End If 

	End Subroutine dist_extract_resources
 
    Subroutine dist_init(this)

	Type(distribution),Intent(inout) :: this
        Integer 			 :: err,nt


        Call MPI_INIT(err)
        Call MPI_COMM_RANK( MPI_COMM_WORLD, this%rank  , err )
        Call MPI_COMM_SIZE( MPI_COMM_WORLD, this%n_proc, err )
# 418

        nt = opt_get_option(this%opt,OPT_TLIB_THREAD_COUNT)
        If ( opt_get_option(this%opt,OPT_PURE_MPI) == 0 ) Then 
# 423

          call tl_init(nt,this%rank)

        End If
        this%cur_par_idx = this%rank


    End Subroutine dist_init

    Subroutine dist_finish(this)

	Type(distribution),Intent(inout) :: this
        Integer 			 :: err


        Call MPI_FINALIZE(err)


        

    End Subroutine dist_finish

    Subroutine dist_save_results(this)

	Type(distribution),Intent(inout)::this

    End Subroutine dist_save_results
    
    Subroutine dist_run(this)

	Type(distribution)		,Intent(inout)  :: this
        Character(len=PHI_NAME_LENGTH) 			:: phi
        Character(len=FILE_NAME_LENGTH) 		:: filename
        Real(kind=rfp)			,Dimension(1:2) :: eps_rg
        Type(approximation) 				:: approx
        Type(epsilon) 					:: eps
        Type(problem) 					:: prob
        Type(op) 					:: ops
        Real(kind=rfp) 					:: eps_r
        Integer 					:: i


        Call dist_init		    (this)
        Call dist_extract_options   (this)
        Call dist_extract_parameters(this)
        Call dist_extract_resources  (this)
        Call dist_do_all_large_probs (this)
        Call dist_finish(this)
    End Subroutine dist_run
	
End Module distribution_class
