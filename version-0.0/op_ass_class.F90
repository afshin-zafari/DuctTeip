# 1 "op_ass_class.F90"

# 1 "./debug.h" 1 



# 11










# 23




                          



# 35


# 41






# 51






# 61






# 71






# 81






# 91





# 3 "op_ass_class.F90" 2 

Module op_ass_class
  
  Use tl
  Use fp
  Use mpi
  Use Timer
  Use dm_class
  Use constants
  Use problem_class
  Use approximation_class
  Use radial_basis_function


  Implicit None

  Integer , Parameter,Private :: EVENT_TASK_STARTED 	= 300      ! task id, tot task cnt,0
  Integer , Parameter,Private :: EVENT_TASK_FINISHED 	= 500      ! task id, tot task cnt,0
  Integer , Parameter,Private :: EVENT_SUBTASK_STARTED 	    = 1017 ! task id, sub task id, thrno
  Integer , Parameter,Private :: EVENT_SUBTASK_FINISHED     = 1018 ! task id, sub task id, thrno

  Public :: op_asm_new_pts
  public op_ass
  Type op_ass
     Type(problem), Pointer			:: prob
     Type(approximation), Pointer	:: approx
     Character(len=NAME_LENGTH)		:: writes
     Integer :: proc_id
  End Type op_ass

  Type args
     Type(op_ass), Pointer	:: this
     Type(dm), Pointer		:: datamanager
     Real(kind=rfp),Dimension(:),Pointer::A,B
     Integer					:: i, j
  End Type args
	
Contains
!-----------------------------------------------------------------------------------------------------------------------------
  Function op_asm_new_pts(this,pts,pts2) Result(new_op_asm)

    Type(op_ass)     ,Intent(inout) :: this
    Type(point_set)  ,Intent(in)    :: pts,pts2
    Type(op_ass)     ,Pointer       :: new_op_asm
    Character(len=80)		    :: phi
    Type(epsilon)		    :: eps
    Type(point_set)                 :: pts_temp

    Allocate(new_op_asm)

    new_op_asm%prob   => this%prob
    new_op_asm%writes = this%writes
    Allocate ( new_op_asm%approx )
    phi = approximation_get_phi(this%approx)
    eps = approximation_get_eps(this%approx)
    Call approximation_new2(new_op_asm%approx , phi, eps,pts,pts2)
    pts_temp = approximation_get_point_set(new_op_asm%approx)

  End Function op_asm_new_pts
!-----------------------------------------------------------------------------------------------------------------------------
  ! Task
  ! This task calculates a distance matrix for block arg%i arg&j
  ! It is saved in the datamanager as <writes>_dist_i_j
  Subroutine op_ass_dist(arg)
    ! argument
    Type(args), Intent(inout)	:: arg
    ! lokala
    Real(kind=rfp), Dimension(:,:), Pointer	:: pts_i, pts_j
    Type(matrix), Pointer					:: dist
    Character(len=NAME_LENGTH)				:: nam
    Integer							:: i, j,taskid,thrdno,tlib_taskid
    
    
    taskid = dm_get_taskid(arg%datamanager)
!    write(*,*) event_name(EVENT_SUBTASK_STARTED),',',(MPI_Wtime()),',',EVENT_SUBTASK_STARTED,',',taskid,',',tlib_taskid,',',thrdNo
    nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    dist => dm_matrix_get(arg%datamanager, nam)
    If (.Not. Associated(dist) .Or. dm_get_mat_length(arg%datamanager) == 0 ) Then                             
       Return 
    End If
    pts_i => get_pts_ptr(arg%i, approximation_get_point_set(arg%this%approx))
    pts_j => get_pts_ptr(arg%j, approximation_get_point_set(arg%this%approx))
    Allocate(dist%grid(1:Size(pts_i, 1), 1:Size(pts_j, 1)))
    Do i=1,Size(dist%grid,1)
       Do j=1,Size(dist%grid,2)
          dist%grid(i,j) = Sqrt((pts_j(j,1)-pts_i(i,1))**2+(pts_j(j,2)-pts_i(i,2))**2)
       End Do
    End Do
!    write(*,*) event_name(EVENT_SUBTASK_FINISHED),',',(MPI_Wtime()),',',EVENT_SUBTASK_FINISHED,',',taskid,',',tlib_taskid,',',thrdNo

  End Subroutine op_ass_dist
!-----------------------------------------------------------------------------------------------------------------------------

  ! Task
  ! This task calculates dphi for block arg%i arg%j
  ! It uses the distance matrix <writes>_dist_i_j from the datamanager if arg%j >= arg%i
  ! Othervise it uses the transponated <writes>_dist_j_i
  Subroutine op_ass_rbf(arg)
    ! argument
    Type(args), Intent(inout)		    :: arg
    ! lokala
    Real(kind=rfp), Dimension(:,:), Pointer :: pts_i, pts_j
    Character(len=NAME_LENGTH)	            :: nam		! name
    Type(matrix), Pointer		    :: mat
    Integer, Dimension(1:2)		    :: rg		! range
    Integer				    :: i, j, offset_i, offset_j
    ! dphi anrop
    Character(len=80)			    :: phi
    Character				    :: nprime
    Integer				    :: nd
    Real(kind=rfp)			    :: eps
    Real(kind=rfp), Dimension(:,:), Pointer :: dist, distT, Block
    Integer                                 :: taskid,thrdno,tlib_taskid

    
    taskid = dm_get_taskid(arg%datamanager)
!    write(*,*) event_name(EVENT_SUBTASK_STARTED),',',(MPI_Wtime()),',',EVENT_SUBTASK_STARTED,',',taskid,',',tlib_taskid,',',thrdNo

    ! allocate block
    pts_i => get_pts_ptr(arg%i, approximation_get_point_set(arg%this%approx))
    pts_j => get_pts_ptr(arg%j, approximation_get_point_set(arg%this%approx))
    Allocate(Block(1:Size(pts_i, 1), 1:Size(pts_j, 1)))

    If(arg%j >= arg%i) Then
       nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    Else
       nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%j+48)//"_"//to_str2(arg%i+48)
    End If

    mat => dm_matrix_get(arg%datamanager, nam)
    If (.Not. Associated(mat).Or.  dm_get_mat_length(arg%datamanager) ==0 ) Then 
       Return
    End If

    phi = approximation_get_phi(arg%this%approx)
    nprime = expression_get_op(problem_get_expression(arg%this%prob), geometry_eq(problem_get_geometry(arg%this%prob), arg%i))
    nd = dims_pts(approximation_get_point_set(arg%this%approx))
    eps = Real(get_eps(approximation_get_eps(arg%this%approx), 1))
    dist => mat%grid

    If(arg%j >= arg%i) Then
       Call dphi(phi, nprime, nd, eps, dist, Block)
    Else
       Allocate(distT(1:Size(dist, 2), 1:Size(dist, 1)))
       distT = Transpose(dist)
       Call dphi(phi, nprime, nd, eps, distT, Block)
       Deallocate(distT)
    End If
     

    rg = range_pts(arg%i, approximation_get_point_set(arg%this%approx))
    offset_i = rg(1)-1
    rg = range_pts(arg%j, approximation_get_point_set(arg%this%approx))
    offset_j = rg(1)-1
 
    mat => dm_matrix_get(arg%datamanager, arg%this%writes)
    If (.Not. Associated(mat).Or.  dm_get_mat_length(arg%datamanager) ==0 ) Then 
       Return
    End If

    Do i=1,Size(Block,1)
       Do j=1,Size(Block,2)
          mat%grid(offset_i+i, offset_j+j) = Block(i,j)
       End Do
    End Do
    
    ! deallocate block
    Deallocate(Block)
!    write(*,*) event_name(EVENT_SUBTASK_FINISHED),',',(MPI_Wtime()),',',EVENT_SUBTASK_FINISHED,',',taskid,',',tlib_taskid,',',thrdNo
    

  End Subroutine op_ass_rbf
!-----------------------------------------------------------------------------------------------------------------------------
	
  ! Task
  ! This task currently outputs the result to a file
  ! Its main responsibility is however making sure <writes> is
  Subroutine op_ass_synch(arg)
    ! argument
    Type(args), Intent(inout)	:: arg
    ! lokala
    Type(matrix), Pointer	:: mat
    Integer			:: i, j

    mat => dm_matrix_get(arg%datamanager, arg%this%writes)

    Call dm_set_sync(arg%datamanager,1)
!    write(*,*) event_name(EVENT_TASK_FINISHED),',',(MPI_Wtime()),',',EVENT_TASK_FINISHED,',',dm_get_taskid(arg%datamanager),',',0,',',0
    

  End Subroutine op_ass_synch
!-----------------------------------------------------------------------------------------------------------------------------

  ! Translate this operation into tasks
  Subroutine op_ass_task(this, datamanager,taskstr)
    ! argument
    Type(op_ass)      , Intent(inout), Target    :: this
    Type(dm)          , Intent(inout), Target    :: datamanager
    Character(len=*)  , Intent(in)   , Optional  :: taskstr
    ! lokala
    Integer			            :: i, j				! loop index
    Integer				    :: Block, blocks	        ! block, #blocks
    Integer, Dimension(1:2)		    :: rg, sp			! range, span
    Type(matrix), Pointer		    :: mat				! matrix, pointer in %grid
    Character(len=NAME_LENGTH)		    :: nam				! name
    Type(tl_handle)			    :: reads, writes, adds
    Type(tl_handle), Dimension(:), Pointer  :: handles
    Type(args)				    :: arg				! arguments to task
    Integer                                 :: task_cnt=0

    Call dm_matrix_add(datamanager, this%writes)
     
    mat => dm_matrix_get(datamanager, this%writes)
    rg = range_pts(approximation_get_point_set(this%approx))
    Allocate(mat%grid(1:rg(2),1:rg(2)))	
    sp = span_pts_id(approximation_get_point_set(this%approx))
    Do i=sp(1),sp(2)
       Do j=sp(1),sp(2)
          nam = Trim(this%writes)//"_block_"//to_str2(i+48)//"_"//to_str2(j+48)
          Call dm_matrix_add(datamanager, nam)
       End Do
    End Do

    Do i=sp(1),sp(2)
       Do j=i,sp(2)
          nam = Trim(this%writes)//"_dist_"//to_str2(i+48)//"_"//to_str2(j+48)
          Call dm_matrix_add(datamanager, nam)
       End Do
    End Do

    Do i=sp(1),sp(2)
       Do j=i,sp(2)
          ! argument
          arg%this => this
          arg%datamanager => datamanager
          arg%i = i
          arg%j = j
          ! handle
          nam = Trim(this%writes)//"_dist_"//to_str2(i+48)//"_"//to_str2(j+48)
          writes = dm_handle_get(datamanager, nam)
          task_cnt = task_cnt +1
          Call tl_add_task_named(taskstr//"dist"//to_str2(i+48)//","//to_str2(j+48)//CHAR(0),&
               op_ass_dist, arg, sizeof(arg), 0, 0, writes, 1, 0, 0)

       End Do
    End Do
    		
    Do i=sp(1),sp(2)
       Do j=sp(1),sp(2)
          arg%this => this
          arg%datamanager => datamanager
          arg%i = i
          arg%j = j

          ! handle
          If(j >= i) Then
             nam = Trim(this%writes)//"_dist_"//to_str2(i+48)//"_"//to_str2(j+48)
          Else
             nam = Trim(this%writes)//"_dist_"//to_str2(j+48)//"_"//to_str2(i+48)
          End If
          reads = dm_handle_get(datamanager, nam)
          
          nam = Trim(this%writes)//"_block_"//to_str2(j+48)//"_"//to_str2(i+48)

          ! adds = dm_handle_get(datamanager, this%writes)
          adds = dm_handle_get(datamanager, nam)
          
          task_cnt = task_cnt +1
          !Call tl_add_task_unsafe(op_ass_rbf, arg, sizeof(arg), reads, 1, 0, 0, adds, 1)
          Call tl_add_task_named(taskstr//"rbf"//to_str2(i+48)//","//to_str2(j+48)//CHAR(0),&
                                 op_ass_rbf, arg, sizeof(arg), reads, 1, 0, 0, adds, 1)
       End Do
    End Do

    arg%this => this
    arg%datamanager => datamanager
    arg%i = -1
    arg%j = -1

    ! handle
    ! reads = dm_handle_get(datamanager, this%writes)
    ! calculate number of blocks + 1 (for result matrix)
    blocks = (sp(2)-sp(1)+1)*(sp(2)-sp(1)+1)+1
    Allocate(handles(1:blocks))
    ! block handles
    Do i=sp(1),sp(2)
       Do j=sp(1),sp(2)
          nam = Trim(this%writes)//"_block_"//to_str2(i+48)//"_"//to_str2(j+48)
          Block = (j-sp(1)) + (i-sp(1))*(sp(2)-sp(1)+1) + 1
          handles(Block) = dm_handle_get(datamanager, nam)
       End Do
    End Do
    ! result handles
    handles(blocks) = dm_handle_get(datamanager, this%writes)
    task_cnt = task_cnt +1
    Call tl_add_task_named("sync"//CHAR(0),op_ass_synch, arg, sizeof(arg), handles, blocks, 0, 0, 0, 0)
    
    


  End Subroutine op_ass_task
!-----------------------------------------------------------------------------------------------------------------------------

  Subroutine op_ass_task_dist(this, datamanager,taskstr,taskid,totnp)

    Type(tl_handle)   , Dimension(:) , Pointer   :: handles
    Type(op_ass)      , Intent(inout), Target    :: this
    Type(dm)          , Intent(inout), Target    :: datamanager
    Character(len=*)  , Intent(in)   , Optional  :: taskstr  
    Integer           , Dimension(1:2)           :: rg1,rg2, sp1,sp2 	
    Integer           , Intent(in)               :: taskid,totnp
    Type(matrix)      , Pointer		         :: mat			
    Character(len=NAME_LENGTH)		         :: nam			
    Type(tl_handle)			         :: reads, writes, adds
    Type(args)				         :: arg			
    Integer			                 :: i, j,k,i2,j2
    Integer				         :: Block, blocks,np,nd,nt
    Type(matrix)      , Pointer			 :: dist
    Real(kind=rfp)    , Dimension(:,:), Pointer	:: pts_i, pts_j

# 325



    this%writes(8:10) = to_str2(taskid+48)
    Call dm_matrix_add(datamanager, this%writes)
    mat => dm_matrix_get(datamanager, this%writes)

    rg1 = range_pts(approximation_get_point_set (this%approx))
    rg2 = range_pts(approximation_get_point_set2(this%approx))

    Allocate( mat%grid(1:totnp,1:totnp) )	

    sp1 = span_pts_id(approximation_get_point_set (this%approx))
    sp2 = span_pts_id(approximation_get_point_set2(this%approx))
    

    Do i=sp1(1),sp1(2)
       Do j=sp2(1),sp2(2)
          nam = Trim(this%writes)//"_block_"//to_str2(i+48)//"_"//to_str2(j+48)
!          write(*,*)  "asm task "," : ",("+>",nam,"<") 
          Call dm_matrix_add(datamanager, nam)
          nam = Trim(this%writes)//"_dist_"//to_str2(i+48)//"_"//to_str2(j+48)
          Call dm_matrix_add(datamanager, nam)
       End Do
    End Do

# 454



# 459


! SINGLE TASK:

    Do i=sp1(1),sp1(2)
       Do j=sp2(1),sp2(2)
          arg%this => this
          arg%datamanager => datamanager
          arg%i = i
          arg%j = j
          nam = Trim(this%writes)//"_block_"//to_str2(j+48)//"_"//to_str2(i+48)
!          write(*,*)  "asm task "," : ",("r>",nam,"<") 
          writes = dm_handle_get(datamanager, nam)
          Call tl_add_task_unsafe(op_ass_single_task, arg, sizeof(arg), 0, 0, writes, 1, 0, 0) !!! ### MT
       End Do
    End Do


    arg%this => this
    arg%datamanager => datamanager
    arg%i = -1
    arg%j = -1
    blocks = (sp1(2)-sp1(1)+1)*(sp2(2)-sp2(1)+1) +1
    Allocate(handles(1:blocks))
    Do i=sp1(1),sp1(2)
       Do j=sp2(1),sp2(2)
          nam = Trim(this%writes)//"_block_"//to_str2(i+48)//"_"//to_str2(j+48)
          Block = (j-sp2(1)) + (i-sp1(1))*(sp2(2)-sp2(1)+1) + 1
          handles(Block) = dm_handle_get(datamanager, nam)
       End Do
    End Do
    handles(blocks) = dm_handle_get(datamanager, this%writes)
    Call tl_add_task_named("sync"//CHAR(0),op_ass_synch, arg, sizeof(arg), handles, blocks, 0, 0, 0, 0)

# 496



  End Subroutine op_ass_task_dist
!-----------------------------------------------------------------------------------------------------------------------------

  Subroutine op_ass_distance_dist(arg)
    Real(kind=rfp)            , Dimension(:,:), Pointer	:: pts_i, pts_j
    Real(kind=rfp)            , Dimension(1:32,1:2)	:: LocA,LocB
    Real(kind=rfp)            , Dimension(1:32*4)	:: LocC
    Type(args)                , Intent(inout)		:: arg
    Type(matrix)              , Pointer			:: dist
    Character(len=NAME_LENGTH)				:: nam
    Integer						:: i, j,taskid,thrdno,tlib_taskid,np,nd,nt,k
    Real(kind=rfp)::r
    
    taskid = dm_get_taskid(arg%datamanager)
    nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    dist => dm_matrix_get(arg%datamanager, nam)
    pts_i => get_pts_ptr(arg%i, approximation_get_point_set (arg%this%approx))
    Call pts_get_info(approximation_get_point_set (arg%this%approx),np,nd,nt)
    pts_j => get_pts_ptr(arg%j, approximation_get_point_set2(arg%this%approx))
    Do j=1,Size(dist%grid,2)
       Do i=1,Size(dist%grid,1)
          r = Sqrt((pts_j(j,1)-pts_i(i,1))**2+(pts_j(j,2)-pts_i(i,2))**2)
          dist%grid(i,j) = r 
       End Do
    End Do

  End Subroutine op_ass_distance_dist
!-----------------------------------------------------------------------------------------------------------------------------

  Subroutine op_ass_rbf_dist(arg)
    Real(kind=rfp) , Dimension(:,:), Pointer :: pts_i, pts_j
    Real(kind=rfp) , Dimension(:,:), Pointer :: dist,  Block
    Type(args)     , Intent(inout)	     :: arg
    Type(matrix)   , Pointer		     :: mat
    Integer        , Dimension(1:2)	     :: rg		
    Character(len=NAME_LENGTH)	             :: nam		
    Integer				     :: i, j, offset_i, offset_j
    Character(len=80)			     :: phi
    Character				     :: nprime
    Integer				     :: nd
    Real(kind=rfp)			     :: eps
    Integer                                  :: taskid,thrdno,tlib_taskid

    nam = TRIM(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    mat => dm_matrix_get(arg%datamanager, nam)
    phi = approximation_get_phi(arg%this%approx)
    nprime = expression_get_op(problem_get_expression(arg%this%prob), geometry_eq(problem_get_geometry(arg%this%prob), arg%i))
    nd = dims_pts(approximation_get_point_set(arg%this%approx))
    eps = Real(get_eps(approximation_get_eps(arg%this%approx), 1))
    !TODO : remove these hard-coded entries
        eps = 2.0
        phi="gauss"
        nprime='0'

    dist => mat%grid
    nam = Trim(arg%this%writes)//"_block_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
     
    mat => dm_matrix_get(arg%datamanager, nam)

    rg = range_pts(arg%i, approximation_get_point_set (arg%this%approx))
    offset_i = rg(1)-1
    rg = range_pts(arg%j, approximation_get_point_set2(arg%this%approx))
    offset_j = rg(1)-1
    
 
    mat => dm_matrix_get(arg%datamanager, arg%this%writes)
    Block =>mat%grid(offset_i+1:offset_i+Size(dist,1),offset_j+1:offset_j+Size(dist,2))
    Call dphi(phi, nprime, nd, eps, dist, Block)
     

  End Subroutine op_ass_rbf_dist
!-----------------------------------------------------------------------------------------------------------------------------
  Subroutine op_ass_single_task_warmup(arg)
    Real(kind=rfp)            , Dimension(:,:), Pointer	:: pts_i, pts_j
    Type(args)                , Intent(inout)		:: arg
    Type(matrix)              , Pointer			:: dist
    Character(len=NAME_LENGTH)				:: nam
    Integer						:: i,j,taskid,thrdno,tlib_taskid,np,nd,nt
    Real(kind=rfp)::r
    Real(kind=rfp)::phi_r, epsilon
    Type(matrix)   , Pointer             :: mat
    Integer                  :: offset_i, offset_j
    Integer        , Dimension(1:2)      :: rg
    Integer :: ii
    Integer :: blocksize

# 588

    taskid = dm_get_taskid(arg%datamanager)
!    write(*,*) event_name(EVENT_SUBTASK_STARTED),',',(MPI_Wtime()),',',EVENT_SUBTASK_STARTED,',',taskid,',',tlib_taskid,',',thrdNo
    
    nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    dist => dm_matrix_get(arg%datamanager, nam) ! linear search
    If (.Not. Associated(dist) .Or. dm_get_mat_length(arg%datamanager) == 0 ) Then
       Return
    End If
    mat => dm_matrix_get(arg%datamanager, arg%this%writes) ! linear search
    call pts_get_info(approximation_get_point_set(arg%this%approx),np,nd,nt)
    blocksize = np/nt/nt
    offset_i = (arg%i-1)*blocksize*nt + (arg%j-1)*blocksize
    rg(1) = 1+offset_i
    rg(2) = offset_i + blocksize

    pts_i => get_pts_ptr(rg, approximation_get_point_set (arg%this%approx))
    pts_j => get_pts_ptr(approximation_get_point_set2(arg%this%approx))

    Do j=1,Size(pts_j, 1)
      Do i=1,blocksize+1
        mat%grid(offset_i+i, j) = 0;
      end do
    end do
  End Subroutine op_ass_single_task_warmup
!-------------------------------------------------------------------------------------------------------

  Subroutine op_ass_single_task(arg)
    Real(kind=rfp)            , Dimension(:,:), Pointer	:: pts_i, pts_j
    Type(args)                , Intent(inout)		:: arg
    Type(matrix)              , Pointer			:: dist
    Character(len=NAME_LENGTH)				:: nam
    Integer						:: i,j,taskid,thrdno,tlib_taskid,np,nd,nt
    Real(kind=rfp)::r
    Real(kind=rfp)::phi_r, epsilon
    Type(matrix)   , Pointer             :: mat
    Integer                  :: offset_i, offset_j
    Integer        , Dimension(1:2)      :: rg
    Integer :: ii
    Integer :: blocksize

# 632


# 636


    taskid = dm_get_taskid(arg%datamanager)
    
    nam = Trim(arg%this%writes)//"_dist_"//to_str2(arg%i+48)//"_"//to_str2(arg%j+48)
    dist => dm_matrix_get(arg%datamanager, nam) ! linear search
    If (.Not. Associated(dist) .Or. dm_get_mat_length(arg%datamanager) == 0 ) Then
       Return
    End If

    epsilon = 2.0 ! ### hardcoded

    mat => dm_matrix_get(arg%datamanager, arg%this%writes) ! linear search

    call pts_get_info(approximation_get_point_set(arg%this%approx),np,nd,nt)
    blocksize = np/nt/nt
    offset_i = (arg%i-1)*blocksize*nt + (arg%j-1)*blocksize
    rg(1) = 1+offset_i
    rg(2) = offset_i + blocksize

    pts_i => get_pts_ptr(rg, approximation_get_point_set (arg%this%approx))
    pts_j => get_pts_ptr(approximation_get_point_set2(arg%this%approx))

    Do j=1,Size(pts_j, 1)
      Do i=1,blocksize+1
        r = Sqrt((pts_j(j,1)-pts_i(i,1))**2+(pts_j(j,2)-pts_i(i,2))**2)
        phi_r = exp(-(epsilon*r)**2.0_rfp)  ! ### hardcoded phi='gauss', nprime='0'
        mat%grid(offset_i+i, j) = phi_r;
      end do
    end do


# 671


  End Subroutine op_ass_single_task
!------------------------------------------------------------------------------------------------

 Subroutine op_ass_new(this, prob, approx, res)
    Type(op_ass), Intent(inout)				:: this
    Type(problem), Intent(in), Target 		:: prob
    Type(approximation), Intent(in), Target	:: approx
    Character(len=*), Intent(in)			:: res

    this%prob => prob
    this%approx => approx
    this%writes = res

  End Subroutine op_ass_new
!-----------------------------------------------------------------------------------------------------------------------------

  Subroutine op_ass_print(this)
    Type(op_ass), Intent(in) :: this
    Write(*,*) "op_ass ", "writes ", this%writes
  End Subroutine op_ass_print
!-----------------------------------------------------------------------------------------------------------------------------

  Function to_str2(no) Result(str)
    Integer :: no
    Character(len=2)::str
    Write (str,"(I2.2)") no-48
  End Function to_str2
!-----------------------------------------------------------------------------------------------------------------------------
End Module op_ass_class
