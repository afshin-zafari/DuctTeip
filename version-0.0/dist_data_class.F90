# 1 "dist_data_class.F90"
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





# 2 "dist_data_class.F90" 2 

Module dist_data_class

  Use fp
  Use mpi
  Use dist_const
  Use dist_types
  Use dist_common
  Use class_point_set

  Implicit None
!!$---- Procedures ------------------------------------------------------------------------------------------------------------
!!$  data_check_sts_all           Loop for checking every Data status and change it if required
!!$  data_check_sts               Checking the status transition of every specific task according to event and current status
!!$  data_clean_all               Loop for destroying every data
!!$  data_create                  Creates a new Data object
!!$  data_find_by_nameGets a data Name and find it in the Data List , if any
!!$  data_find_by_sts             Returns the first Data that its status is as given
!!$  data_sample_pts              Craetes a sample Point Set strcuture based given input argments
!!$  data_populate                Populated the Data object and updates its status accordingly
!!$  data_read_partitions         Reads differnet partitions from correposnding files into memory,used for correctness testing.
!!$  data_save_partitions         Saves differnet partitions in the resulting matrix to their correposnding files,used for 
!!$                               testing correctness of the results
!!$  data_send                    Sending the Data content ( after packing all the Data ingredients into contigous memory)
!!$-----------------------------------------------------------------------------------------------------------------------------

Contains 

!!$-----------------------------------------------------------------------------------------------------------------------------
  Subroutine data_check_sts_all(this,notified_from,event)

    Type(remote_access) , Intent(inout) :: this
    Integer		, Intent(inout) :: event
    Integer		, Intent(in   ) :: notified_from
    Integer 				:: i 

   

    Do i = Lbound(this%data_list,1) , Ubound(this%data_list,1)
       If (this%data_list(i)%id /= DATA_INVALID_ID) Then 
          
          Call data_check_sts(this,this%data_list(i),event)
       End If
    End Do   

    

  End Subroutine data_check_sts_all
!!$---------------------------------------------------------------------------------------------------------------------
!!$   This routine, checks the current status of the input Data and the last event happened in the program. Then it 
!!$   decides on what will be the next status and whom has to be notified accordingly. The procedure can be 
!!$   summarized as following table:
!!$   
!!$   Event       Condition           From Status    To Status         Notify
!!$   -----       ----------          -----------    ---------         ---------
!!$    --          is remote           CLEANED        INIT             Listener
!!$   Data RCVD    --                  INIT           READ OK          Task ,Listener
!!$     --        Sync Terminate       INIT           CLEANED           --
!!$     --        No pending listener  READ OK        MODIFY OK        Task 
!!$     --        Not in Terminate     MODIFY OK      MODIFY OK         --
!!$     --        No pending listener  MODIFY OK      CLEANED           --
!!$               and in Terminate     
!!$   DATA ACK      --                 SEND INIT      ACK RCVD          --
!!$     --        Not in Terminate     ACK RCVD       ACK RCVD          --
!!$     --        In Terminate         ACK RCVD       CLEANED           --
!!$
!!$---------------------------------------------------------------------------------------------------------------------

  Subroutine data_check_sts(this,Data,event)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer 		, Intent(inout) :: event
    Integer 				:: from_sts,to_sts,notify,d,e
    Logical 				:: answer


    

    from_sts = data%status
    to_sts= from_sts
    notify = NOTIFY_DONT_CARE
    
# 87

    Select Case(from_sts)
       Case (DATA_STS_CLEANED) 
          If ( data%proc_id /= this%proc_id ) Then ! data is remote 
             
             Call data_add_to_list(this,Data)
             notify = NOTIFY_OBJ_LISTENER
          End If
          to_sts = DATA_STS_INITIALIZED
       Case(DATA_STS_INITIALIZED )
          If ( event == EVENT_DATA_RECEIVED) Then 
             to_sts = DATA_STS_READ_READY
             notify = NOTIFY_OBJ_TASK + NOTIFY_OBJ_LISTENER
          End If
          If ( this%sync_stage == SYNC_TYPE_TERM_OK ) Then 
             to_sts = DATA_STS_CLEANED
             
             write(*,*)  "data Clean Called"," : ","data init in termination"
             Call data_clean(this,Data,event)
          End If
       Case(DATA_STS_READ_READY)
          Call is_any_pending_lsnr(this,Data,event,answer)
          If ( .Not. answer ) Then 
             to_sts = DATA_STS_MODIFY_READY
             notify = NOTIFY_OBJ_TASK
          End If
       Case(DATA_STS_MODIFY_READY)
          If (this%sync_stage < SYNC_TYPE_TERM_OK) Then              
             
          Else
             Call is_any_pending_task(this,Data,event,answer)
             If ( .Not. answer ) Then 
                to_sts = DATA_STS_CLEANED
                write(*,*)  "data Clean Called"," : ","no pending task"
                Call data_clean(this,Data,event)
             End If
          End If 
       Case (COMM_STS_SEND_PROGRESS)
          If ( event == EVENT_DATA_ACK) Then 
             to_sts = COMM_STS_SEND_COMPLETE
          End If
       Case (COMM_STS_SEND_COMPLETE)
          If (this%sync_stage < SYNC_TYPE_TERM_OK) Then              
             
          Else
             to_sts = DATA_STS_CLEANED
             
             write(*,*)  "data Clean Called"," : ","send complete in termination"
             Call data_clean(this,Data,event) 
          End If
       Case default
    End Select

    data%status = to_sts
    this%notify = notify

    If (to_sts /= from_sts .Or. notify /= NOTIFY_DONT_CARE ) Then 
       If (data%id /= DATA_INVALID_ID) Then 
          e=EVENT_DATA_STS_CHANGED
          d=data%id
          call instrument(e,d,0,0)
       End If
    End If
    


    

  End Subroutine data_check_sts

!!$---------------------------------------------------------------------------------------------------------------------
  Subroutine data_clean_all(this)

    Type(remote_access) ,Intent(inout) 	:: this
    Integer 				:: i ,event = EVENT_DONT_CARE

    
    
    Do i = Lbound(this%data_list,1) , Ubound(this%data_list,1)
       If (this%data_list(i)%id /= DATA_INVALID_ID) Then 
          Call data_clean(this,this%data_list(i),event)
          write(*,*)  "Data Cleaned"," : ",this%data_list(i)%name
          !write(*,*) event_name(EVENT_DATA_CLEANED),',',(MPI_Wtime()),',',EVENT_DATA_CLEANED,',',i,',',0,',',0
       End If
    End Do

    
    
  End Subroutine data_clean_all

!!$---------------------------------------------------------------------------------------------------------------------
  Function data_create(this,name,nr,nc,version,pid) Result(dh)

    Type(remote_access) ,Intent(inout) 					:: this
    Integer		,Dimension(AXS_TYPE_LBOUND:AXS_TYPE_UBOUND)     :: vers
    Type(data_handle)   ,Pointer 					:: dh
    Integer 		,Intent(in)					:: pid,nr,nc,version
    Character(len=*) 							:: name
    Type(point_set) ::pts

    

    Allocate(dh)
    dh%name    = name
    dh%proc_id = pid
    dh%nrows   = nr
    dh%ncols   = nc
    dh%np      = this%np
    dh%nd      = this%nd
    dh%nt      = 2
    dh%sv_r    = 0
    dh%sv_w    = 0
    dh%sv_m    = 0
    dh%version = 0 
    dh%has_lsnr= 0
    If (opt_get_option(this%opt,OPT_CHOLESKY) /= 0 ) dh%ncols = dh%nrows

       
    Call data_add_to_list(this,dh)
    
    call instrument(EVENT_DATA_ADDED,dh%id,0,0)

    If (0==1 .and. opt_get_option(this%opt,OPT_NODE_AUTONOMY) /= 0 .And. this%proc_id == pid .And. dh%name(1:1)/= "D") Then 
       pts = data_sample_pts(this,this%np,2,this%nd)
       Call data_populate(this,dh%name ,pts)
       write(*,*)  "Populated "," : ",(this%proc_id,pid,dh%name ) 
    End If

    

  End Function data_create
!!$---------------------------------------------------------------------------------------------------------------------

  Function data_find_by_name_dbg(this,name,f,l) Result(Data)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)             	:: Data
    Integer          , Intent(in)       :: l
    Character(len=*) , Intent(in)	:: f
    Character(len=*) 			:: name
    Integer 				:: i 

       
    data%id = DATA_INVALID_ID
    data%name = ""

    Do i = Lbound(this%data_list,1),Ubound(this%data_list,1)
       If (this%data_list(i)%id == DATA_INVALID_ID ) continue
       If (this%data_list(i)%name == name ) Then 
          Data = this%data_list(i)
          Return
       Else
          
       End If
    End Do

       

  End Function data_find_by_name_dbg
!!$---------------------------------------------------------------------------------------------------------------------

  Function data_find_by_sts(this,sts) Result (Data)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in   ) :: sts
    Type(data_handle)                   :: Data
    Integer 				:: i 

    data%id = DATA_INVALID_ID
    Do i= Lbound(this%data_list,1),Ubound(this%data_list,1)
       If (this%data_list(i)%status == sts ) Then 
          Data = this%data_list(i)
          Return
       End If
    End Do

  End Function data_find_by_sts
!!$---------------------------------------------------------------------------------------------------------------------
  Function data_sample_pts(this,np,nt,dim_cnt) Result(pts)

    Type(remote_access) , Intent(inout)        :: this
    Integer             , Intent(in)           :: np,nt,dim_cnt
    Type ( point_set)                          :: pts
    Real(kind=rfp)     ,Dimension(:,:),Pointer :: x
    Integer            ,Dimension(:,:),Pointer :: id
    Integer                                    :: i ,j,l

    
    Allocate ( x(1:np,1:dim_cnt) , id (1:nt,1:ID_COL_MAX)  ) 
    id = 0 
    x = 0.0
    Do i = 1, Ubound(x,1)
       Do j = 1, Ubound(x,2)
          x(i,j) = i*10 +j
       End Do
    End Do

    j = Ubound(x,1) / Ubound(id,1)
    Do i = 1, Ubound(id,1)
       id(i,1) = i 
       id(i,ID_COL_FROM) = (i-1)*j+1
       id(i,ID_COL_TO  ) = (i  )*j
    End Do
    
    
    pts = new_pts(id,x)
    
  End Function data_sample_pts
!!$---------------------------------------------------------------------------------------------------------------------
  Subroutine data_populate(this,name,pts)

    Type(remote_access) , Intent(inout) :: this
    Type(point_set)     , Intent(inout) :: pts
    Character(len=*)    , Intent(in)    :: name
    Character(len=2)                    :: prefix,sep
    Type(data_handle)   		:: my_data
    Integer                             :: test,choltrf,i,j,k,r,c,ro,co
    Type(point_set)     , Pointer       :: part_pts

    

    my_data = data_find_by_name_dbg(this,name,"dist_data_class.F90",325)    
    test = opt_get_option(this%opt,OPT_TEST_CORRECTNESS)
    choltrf = opt_get_option(this%opt,OPT_CHOLESKY)
    If ( choltrf /= 0 ) Then 
       write(*,*)  "Chol Init Data name,rows"," : ",(name,my_data%nrows)
       my_data%ncols = my_data%nrows
       Allocate ( this%data_list(my_data%id)%mat (1:my_data%nrows,1:my_data%ncols) ) 
       
       Read ( name , "(A2 I2 A1 I2)") prefix,ro,sep,co
       
       call instrument(EVENT_DATA_POPULATED,my_data%id,0,0)
       Do i = 1,Ubound(this%data_list(my_data%id)%mat,1)
          Do j = 1,Ubound(this%data_list(my_data%id)%mat,2)
             r=ro*this%np+i
             c=co*this%np+j
             If ( r== c) Then 
                this%data_list(my_data%id)%mat(i,j)= r
             Else
                this%data_list(my_data%id)%mat(i,j)= Min (r,c)-2
             End If
          End Do
       End Do
       this%data_list(my_data%id)%status = DATA_STS_READ_READY
       Call print_chol_data(this)
       call instrument(EVENT_DATA_POPULATED,my_data%id,0,0)
       Return 
    End If
    If ( test /= 0 ) Then 
       part_pts =>  data_read_partitions(this)
    End If
    If (my_data%id /= DATA_INVALID_ID) Then 
       this%data_list(my_data%id)%status = DATA_STS_READ_READY
       if ( test /= 0 ) Then 
          this%data_list(my_data%id)%pts  = part_pts
          
          call write_pts(this%data_list(my_data%id)%pts )
       else
          this%data_list(my_data%id)%pts  = pts
       end if
       this%event = EVENT_TASK_FINISHED
       
       call instrument(EVENT_DATA_POPULATED,my_data%id,0,0)
    End If

    
  End Subroutine data_populate
!!$---------------------------------------------------------------------------------------------------------------------
  Function data_read_partitions(this) Result (pts)
    Type(remote_access) , Intent(inout) :: this
    Type(point_set)     , Pointer       :: pts
    Type(point_set)     , Dimension(1:4):: p
    Integer                             :: i,n, sz,np,nd,nt
    Character(len=22)                   :: fname
    Logical                             :: flag
    
    n = this%proc_id + 1
    sz = 0 
    Do i = 1,4
       fname = "./input/boxgeom" // Achar(n+48)//".dat"
       p(i) = new_pts(fname)
       Call pts_get_info(p(i),np,nd,nt)
       sz = sz + np
       
       Call write_pts(p(i))
       n = n+ this%node_cnt
       If (n > 8) Exit
    End Do
    Allocate ( pts ) 
    pts = new_pts(sz,nd,i)
     
    Call pts_get_info(pts,np,nd,nt)
     
    Do n=1,i
       flag = add_pts(pts,p(n))
       Call destruct_pts(p(n))
    End Do
    Call write_pts(pts)
  End Function  data_read_partitions
!!$---------------------------------------------------------------------------------------------------------------------
  Subroutine data_save_partitions(this,mat)

    Type(remote_access) , Intent(inout)                :: this
    Real(kind=rfp)      , Dimension(1:,:) , Intent(in) :: mat
    Integer                                            :: n, sz,f,i
    Character(len=22)                                  :: fname

    n = this%proc_id + 1
    sz = 9
    f  = 1
    Do While  (n <= 8) 
       If ( n > 4 ) sz = 16 
       fname = "./output/boxgeom" // Achar(n+48)//".dat"
       

       Open ( 38,file=fname)
       Do i = f,f+sz-1
          Write(38,*) mat(i,1:Ubound(mat,2 ) )
          Write(* ,*) mat(i,f:f+sz-1 )
       End Do
       Close (38)
       f = f+sz
       n = n+ this%node_cnt
    End Do

  End Subroutine data_save_partitions
!!$---------------------------------------------------------------------------------------------------------------------
!!$  Steps of this routine are
!!$    1. The Data object in the list is found.
!!$    2. All the contents of the Data is packed into a single contigous memory. Contents are:
!!$       - Data object information e.g. Name, Version, ProcessorId ...
!!$       - Points Set information which is : 
!!$           -- ID/Types of Points
!!$           -- The coordinates of Points
!!$    3. Send the buffer in non-blocking mode to the given destination
!!$    4. Change the status of the Data to SEND PROGRESS ( = NOT_ACKED)
!!$---------------------------------------------------------------------------------------------------------------------
  Subroutine data_send(this, Data,dest)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer             , Intent(inout) :: dest
    integer 				:: err,idx

    do idx = 1, ubound(this%data_req,1)
       if (this%data_req(idx) == MPI_REQUEST_NULL) then 
          exit
       end if
    end do
    If (idx <= Ubound(this%data_req,1)) Then 
          
          Call pack_data_pts (this,this%data_list(data%id),this%data_list(data%id)%pts,&
                              this%data_list(data%id)%sendbuf,this%data_buf_size) 
           

          Call MPI_ISEND (this%data_list(data%id)%sendbuf,this%data_buf_size,MPI_BYTE,& 
                          dest,MPI_TAG_DATA,this%mpi_comm,this%data_req(idx),err) 


          data%status = COMM_STS_SEND_PROGRESS
          write(*,*) event_name(EVENT_DATA_SEND_REQUESTED),',',(MPI_Wtime()),',',EVENT_DATA_SEND_REQUESTED,',',idx,',',dest,',',thi&
&s&
# 447
%data_buf_size
    End If

  End Subroutine data_send
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine data_partition_matrix(this,nb)
    type(data_handle) ,intent(inout) :: this
    integer , intent(in) :: nb
    integer :: rb,cb,brows,bcols,k,l
    type(matrix),pointer:: mat
    integer , dimension(1:4) :: rg
    brows = ubound(this%mat,1)/nb
    bcols = ubound(this%mat,2)/nb
    call chol_print_data( this%mat,brows,bcols)
    do cb=1,nb
       do rb=1,nb
          call dm_matrix_add(this%dmngr,mat_name('M',rb,cb))
          mat=>dm_matrix_get(this%dmngr,mat_name('M',rb,cb))
          allocate( mat%grid(1:brows,1:bcols) )
          rg = block_get_range(this%mat,rb,cb,nb)
          mat%grid(:,:) = this%mat( rg(1):rg(2),rg(3):rg(4) ) 
          call chol_print_data( mat%grid,brows,bcols)
       end do
    end do
  end subroutine data_partition_matrix
!----------------------------------------------------------------------------------------------------------------
  subroutine data_combine_parts(this,nr,nc,nb)
    type(data_handle) ,intent(inout) :: this
    integer , intent(in) :: nr,nc,nb
    integer :: rb,cb,brows,bcols
    type(matrix),pointer:: mat
    integer , dimension(1:4) :: rg
    if ( .not. associated( this%mat ) ) then 
       allocate ( this%mat (1:nr,1:nc) ) 
    end if
    do cb=1,nb
       do rb=1,nb
          mat=>dm_matrix_get(this%dmngr,mat_name('M',rb,cb))
          rg = block_get_range(this%mat,rb,cb,nb)
          brows= ubound(mat%grid,1)
          bcols= ubound(mat%grid,2)
          call chol_print_data( mat%grid,brows,bcols)
          this%mat( rg(1):rg(2),rg(3):rg(4) ) = mat%grid(:,:)
       end do
    end do
    call chol_print_data( this%mat,nr,nc)
  end subroutine data_combine_parts
!----------------------------------------------------------------------------------------------------------------

  function data_get_mat_part(this,rb,cb) result(mat)

    type(data_handle) , intent(inout) :: this
    integer           , intent(in)            :: rb,cb
    real(kind=rfp), dimension(:,:),pointer    :: mat
    type(matrix) , pointer :: dMat
  
    dMat=>dm_matrix_get(this%dmngr,mat_name('M',rb,cb))
    mat=>dMat%grid
    
     
  end function data_get_mat_part


End Module dist_data_class
