# 1 "remote_access_class.F90"

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





# 3 "remote_access_class.F90" 2 

Module remote_access_class 

  Use fp
  Use mpi 
  Use wfm_class 
  Use dist_const
  Use dist_types
  Use dist_common
  Use dist_data_class
  Use dist_task_class
  Use dist_lsnr_class
  Use class_point_set 
  Use radial_basis_function

  Implicit None
!!$-------------------------------------------------------------------------------------------------------------------------------
!!$  Logic of the Program
!!$
!!$   A) ROOT or COORDINATOR node
!!$
!!$    1. The list of tasks is generated ( based on Broadcasting Method or Pipelinig)
!!$    2. The dependencies of the Tasks to Data are determined.
!!$    3. The task list are sent to their corresponding destination nodes.
!!$    4. If all tasks are sent, a SYNC message is sent to the other nodes to have them finish their Dependency Checking routines.
!!$    5. If a Data gets READY , 
!!$      5-1. Checks Tasks for execution. If any, runs it.
!!$      5-2. Checks Listeners for any listening node on that Data. If any, send Data to it.
!!$    6. If all Tasks are executed and FINISHED, send SYNC message for TERMINATION to all other nodes.
!!$    7. When all listeners are cleaned and Tasks are finished, Terminate the Program.
!!$
!!$   B) Non ROOT nodes
!!$     Loop until nothing remains to do:
!!$     1. if any Task received, adds it to list and computes the dependencies.
!!$     2. if any Listener received, adds it to list and waits for its Data to be READY.
!!$     3. if any Data received(READY), 
!!$       3-1. runs any Task that has no more dependency. 
!!$       3-2. sends the Data to the Listeners waiting for it.
!!$     4. if SYNC for LAST TASK received, does not respond ACK until all isteners are sent and ACKed.
!!$     5. if SYNC for TERMINATION OK received, does not respond until all tasks are FINISHED and all listeners are CLEANED.
!!$   
!!$--------------------------------------------------------------------------------------------------------------------------------
!!$   Communication Points:
!!$   1. Task, Sync, Data and Listener messages are distingueshed by separate tags in MPI routines.
!!$   2. All communications are non-blocking using ISEND and IRECV MPI routines.
!!$   3. non-blocking receiving from any source is accomplished by IPROBE routine.
!!$   4. For checking the completeness of the sending messages, TESTANY is used. 
!!$   5. Synchronous mode of communicaion is used for SYNC messages.
!!$   6. ACk checking is synonym for checking the completenss of the sending messages(see 4 above).
!!$   7. Sync messages are implemented as special Task objects to be communicated.(with a separate tag)
!!$   
!!$   
!!$   
!!$--------------------------------------------------------------------------------------------------------------------------------
!!$- Procedures -------------------------------------------------------------------------------------------------------------------
!!$--------------------------------------------------------------------------------------------------------------------------------
!!$  check_ack_recv                  Polls the inbox to check for any receving ACKnowledge of previous sends.
!!$  check_data_recv                  ..     ..              ..    ..          Data object     ..   ..    .
!!$  check_lsnr_recv                  ..     ..              ..    ..          Listener object ..   ..    .
!!$  check_sync_recv                  ..     ..              ..    ..          Sync message    ..   ..    .
!!$  check_task_recv                  ..     ..              ..    ..          Task object     ..   ..    .
!!$  check_inbox                     Calls the xxxx_xxxx_recv routines until nothing remained in receive buffer.
!!$  check_outbox                    Sends Data,Tasks,Listeners whose statuses are set to SEND INIT.
!!$  check_scheduler                 check which Tasks can be executed and which Tasks are received.
!!$  mbox_check_sts_all              runs both InBox and OutBox.
!!$  remote_access_new               Creating New rma (this) object.
!!$  rma_all_sync_rcvd               Checks (by counting) that all the sync messages are received by other parties.
!!$  rma_can_terminate               Returns True if anything is OK to Terminate the whole processing.
!!$  rma_chunk_read                  Call-back function that is called by Point File Reader at every chunk.
!!$  rma_read_pts_chunk              Initiates the Chunk Read Points from File.
!!$  rma_create_data                 Creates a new Data object.
!!$  rma_create_task                 Creates a new Task object.
!!$  rma_create_access               Creates a new Access-Type object.
!!$  rma_cycle_one_step              Performs all the pollings and state transitions up to a steady state.
!!$  rma_generate_tasks              Generates the Tasks list for Broadcasting distribution method.(Node_0-->Node_j)
!!$  rma_generate_tasks_pipeline     Generates the Tasks list for Pipelining distribution method.(Node_i-->Node_i+1)
!!$  rma_init_this                   Initializes the This object.
!!$  rma_mat_assemble                Assembles the RBF Problem Matrix.
!!$  rma_send_all_lsnr               Returns TRUE if all the listeners are ACKED. (sent and also received by receiver)
!!$  rma_send_all_tasks              Sends all the Tasks in Tasks list.(first time that they are created)
!!$  rma_sync_bcast                  Sends a Sync message to all other nodes.
!!$  rma_synchronize                 Synchronizes nodes(parties) together implicitly and non-blocking.
!!$  rma_task_check_sts              Calls the task check status routine.
!!$  rma_test_data_packing           Tests the packing and unpacking of Data contents without being transmitted.
!!$  rma_unpack_data_pts             Unpacks the ingredients of Data(info,Point Types and Coordinates).
!!$  schd_check_sts_all              Calls the Check Scheduler. 
!!$  schd_data_dependences           Sets the minimum required version of Data for a Task.
!!$  schd_update_dependences         Loops for all tasks for setting their dependencies.
!!$------------------------------------------------------------------------------------------------------------------
Contains

!!$------------------------------------------------------------------------------------------------------------------

  Function check_ack_recv(this,event) Result(flag)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout) 		:: this
    Integer 		, Intent(inout)			:: event
    Logical 						:: flag
    Integer 						:: err,idx,task_dst

    

    Call MPI_TESTANY(Ubound(this%task_req,1),this%task_req,idx,flag,sts,err)
# 111

    If ( flag .And. idx >0) Then 
       this%task_req(idx) = MPI_REQUEST_NULL
       call instrument(EVENT_TASK_ACK,idx,0,0)
       task_dst = opt_get_option(this%opt,OPT_TASK_DISTRIBUTION)
       If ( task_dst  == TASK_DIST_1BY1) Then 
           
          Call rma_generate_single_task(this,this%task_list(idx)%proc_id,idx)
       End If
       this%event = EVENT_TASK_ACK
       Call rma_task_check_sts(this, this%task_list(idx),this%event)
    End If


    Call MPI_TESTANY(Ubound(this%lsnr_req,1),this%lsnr_req,idx,flag,sts,err)
# 128

    If ( flag .And. idx >0) Then 
       this%event = EVENT_LSNR_ACK
       this%lsnr_req(idx) = MPI_REQUEST_NULL
       
       call instrument(EVENT_LSNR_ACK,idx,0,0)
       Call lsnr_check_sts(this, this%lsnr_list(idx),this%event)
    End If


    Call MPI_TESTANY(Ubound(this%data_req,1),this%data_req,idx,flag,sts,err)
# 140

    If ( flag .And. idx >0) Then 
       this%data_req(idx) = MPI_REQUEST_NULL
       this%event = EVENT_DATA_ACK
       
       call instrument(EVENT_DATA_ACK,idx,0,0)
       Call data_check_sts(this, this%data_list(idx),this%event)
    End If

    this%event = EVENT_DONT_CARE
    flag = flag .And. idx >0

    

  End Function check_ack_recv
!!$------------------------------------------------------------------------------------------------------------------

  Function check_data_recv(this,event) Result(flag)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Integer 		, Intent(inout)	  		:: event
    Logical 						:: flag,flbuf
    Integer 						:: err,idx,data_id,mpipure



    

    flag = .False.

    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_DATA, this%mpi_comm,flag,sts,err)
# 175

    If ( flag ) Then 

       Do idx = 1, Ubound(this%data_list,1)
          If ( this%data_list(idx)%id == DATA_INVALID_ID ) Then 
             Exit
          End If
       End Do

       

       Call MPI_RECV ( this%data_buf(1), this%data_buf_size, MPI_BYTE, sts(MPI_SOURCE), MPI_TAG_DATA, this%mpi_comm, sts, err)

       
       Call rma_unpack_data_pts(this,this%data_buf,this%data_buf_size,data_id)
       write(*,*) event_name(EVENT_DATA_RECEIVED),',',(MPI_Wtime()),',',EVENT_DATA_RECEIVED,',',data_id,',',sts(MPI_SOURCE),',',thi&
&s&
# 190
%data_buf_size
       write(*,*)  "Data Received"," : ",this%data_list(data_id)%name
       


       
       this%event = EVENT_DATA_RECEIVED
       this%data_list(data_id)%status = DATA_STS_INITIALIZED
       Call data_check_sts(this,this%data_list(data_id),this%event)
       Call lsnr_data_recv(this,this%data_list(data_id))
       Call task_data_recv(this,this%data_list(data_id))
       event = this%event
       mpipure = opt_get_option( this%opt,OPT_PURE_MPI)
       If ( mpipure /= 0 ) flag = .False.
    End If

    

  End Function check_data_recv
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine check_inbox(this,event)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(inout) :: event
    Logical                             :: flag 

    Call check_sync_recv(this,event)
    flag = check_task_recv(this,event)
    flag = check_data_recv(this,event)
    flag = check_lsnr_recv(this,event)
    flag = check_ack_recv (this,event)

  End Subroutine check_inbox
!!$------------------------------------------------------------------------------------------------------------------

  Function check_lsnr_recv(this,event) Result(flag)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Integer 		, Intent(inout)	  		:: event
    Type (listener)                                     :: lsnr
    Logical 						:: flag
    Integer 						:: err,idx
    Type(data_handle)                                   :: Data
    character(len=MAX_DATA_NAME)                        :: dname


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_LSNR, this%mpi_comm,flag,sts,err)
    if ( err /= 0 ) then 
        write(*,*)  "Receiving Lsnr failed. err"," : ",err
    end if 
     

    If ( flag ) Then 
       write(*,*)  "Is Lsnr Rcvd?"," : ",flag
       Do idx = 1, Ubound(this%lsnr_list,1)
          If ( this%lsnr_list(idx)%id == LSNR_INVALID_ID ) Then 
             Exit
          End If
       End Do

       Call MPI_RECV ( this%lsnr_list(idx), sizeof(lsnr), MPI_BYTE, sts(MPI_SOURCE), MPI_TAG_LSNR, this%mpi_comm, sts, err)

       this%lsnr_list(idx)%status = LSNR_STS_CLEANED
       this%event	          = EVENT_LSNR_RECEIVED
       dname=this%lsnr_list(idx)%dname
       write(*,*)  "Lsnr from,dname "," : ",sts(MPI_SOURCE),dname
       Data = data_find_by_name_dbg(this,dname,"remote_access_class.F90",279)
       If ( data%id == DATA_INVALID_ID) Then 
          Data=data_create(this,dname,this%np,this%nd,this%lsnr_list(idx)%min_ver,this%proc_id)
       End If
       this%lsnr_list(idx)%req_data=>this%data_list(data%id)
       
       call instrument(EVENT_LSNR_RECEIVED,idx,0,0)
       Call lsnr_check_sts(this,this%lsnr_list(idx),this%event)
       event = this%event
    End If

  End Function  check_lsnr_recv
!!$------------------------------------------------------------------------------------------------------------------

  Function  check_outbox(this,event) Result ( flag ) 

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(inout) :: event
    Type(data_handle)                   :: Data
    Type(listener)                      :: lsnr
    Type(task_info)                     :: task
    Logical 				:: dummy,flag
    Type ( link_list_node) , Pointer    :: node
    integer :: noack

    
    flag = .False.
    If ( .Not. Associated (ll_get_cur(this%send_list)) ) Then 
       !ACKed tasks will be cleaned from task list
       noack = count (this%task_list(:)%id /= TASK_INVALID_ID .and. & 
                      this%task_list(:)%proc_id /= this%proc_id ) 
       noack = noack + count ( this%task_req(:) /= MPI_REQUEST_NULL  ) 
       if ( noack /= 0 ) return                       
       dummy=  rma_synchronize(this,SYNC_TYPE_LAST_TASK)
       Return  
    End If

    node => ll_pop_cur(this%send_list)
    Select Case (node%obj)
    Case (DATA_OBJ)
       Data = this%data_list(node%item)
       If ( data%id /= DATA_INVALID_ID ) Then 
          Call data_send(this,Data,event)
          this%data_list(data%id) = Data
          flag = .True.
       End If
    Case (TASK_OBJ)
       task = this%task_list(node%item)
       If ( task%id /= TASK_INVALID_ID ) Then 
          Call task_send(this,task%id)
          flag = .True.
       End If
    Case (LSNR_OBJ)
       lsnr=this%lsnr_list(node%item)
       If ( lsnr%id /= LSNR_INVALID_ID ) Then 
          Call lsnr_send(this,lsnr%id)
          flag = .True.
       End If
    End Select

    Deallocate ( node)
  End Function  check_outbox
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine check_scheduler (this,event)

    Type(remote_access),Intent(inout) :: this
    Integer            ,Intent(inout) :: event
    Type(task_info)                   :: task
    Type ( link_list_node) , Pointer  :: node

    
    If ( .Not. Associated (ll_get_cur(this%tlist_ready_sts)) ) Then 
       If ( rma_synchronize(this,SYNC_TYPE_TERM_OK) ) Then 
          event = EVENT_ALL_TASK_FINISHED
          call instrument(EVENT_ALL_TASK_FINISHED,0,0,0)
       End If
       Return  
    End If

    node => ll_pop_cur(this%tlist_ready_sts)

    task = this%task_list (node%item ) 
    If (task%id /= TASK_INVALID_ID) Then 
       Call task_run (this,task)
    End If

    Deallocate ( node)

  End Subroutine check_scheduler 
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine check_sync_recv(this,event)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Integer 		, Intent(inout)	  		:: event
    Type(task_info)                                     :: task
    Type(task_info)     , Dimension(1:1)                :: in_task
    Logical 						:: flag
    Integer 						:: err



    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_SYNC_LAST_TASK, this%mpi_comm,flag,sts,err)
# 393

    If ( flag ) Then 
       
       If ( rma_send_all_lsnr(this) ) Then

          Call MPI_RECV ( in_task(1), sizeof(task), MPI_BYTE, sts(MPI_SOURCE), &
               MPI_TAG_SYNC_LAST_TASK, this%mpi_comm, sts, err)

          this%sync_stage = SYNC_TYPE_LAST_TASK
          call instrument(EVENT_ALL_TASK_FINISHED,SYNC_TYPE_LAST_TASK,0,0)
       Else
          event = EVENT_DONT_CARE
          call instrument(EVENT_ALL_TASK_FINISHED,-1,0,0)
          Call lsnr_check_sts_all(this,NOTIFY_DONT_CARE,event)
       End If
    End If


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_SYNC_DATA_FREE, this%mpi_comm,flag,sts,err)

    If ( flag ) Then


       Call MPI_RECV ( in_task(1), sizeof(task), MPI_BYTE, sts(MPI_SOURCE),&
            MPI_TAG_SYNC_DATA_FREE, this%mpi_comm, sts, err)


    End If


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_SYNC_TERM_OK, this%mpi_comm,flag,sts,err)
# 438

    If ( flag ) Then 
       
       If (rma_can_terminate(this)) Then 


          Call MPI_RECV ( in_task(1), sizeof(task), MPI_BYTE, sts(MPI_SOURCE),&
               MPI_TAG_SYNC_TERM_OK, this%mpi_comm, sts, err)


          this%sync_stage = SYNC_TYPE_TERM_OK
          
          call instrument(EVENT_ALL_TASK_FINISHED,SYNC_TYPE_TERM_OK,0,0)
       End If
    End If


    

  End Subroutine check_sync_recv
!!$------------------------------------------------------------------------------------------------------------------

  Function  check_task_recv(this,event) Result(flag)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Integer 		, Intent(inout)	  		:: event
    Logical 						:: flag
    Integer 						:: err,req,idx,i
    Type(task_info)                                     :: task
    Character(len=MAX_DATA_NAME)                        :: dname



    


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK, this%mpi_comm,flag,sts,err)

    If ( flag ) Then 

       Do idx = 1, Ubound(this%task_list,1)
          If ( this%task_list(idx)%id == TASK_INVALID_ID ) Then 
             Exit
          End If
       End Do

        

       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK, this%mpi_comm, sts, err)

        
       

       this%task_list(idx)%status = TASK_STS_INITIALIZED
       this%task_list(idx)%id     = idx
       Do i = 1,Ubound(this%task_list(idx)%axs_list,1)
          Nullify(this%task_list(idx)%axs_list(i)%data)
       End Do
       this%event = EVENT_TASK_RECEIVED
       call instrument(EVENT_TASK_RECEIVED,idx,0,0)
       write(*,*)  "Task Name"," : ",this%task_list(idx)%name
       Call rma_task_check_sts(this,this%task_list(idx),this%event)
       dname = task_write_name(this,idx)
       event = this%event
    End If
  End Function check_task_recv


!!$------------------------------------------------------------------------------------------------------------------

  Subroutine mbox_check_sts_all(this,notified_from,event)

    Type(remote_access) , Intent(inout) :: this
    Integer		, Intent(inout) :: event
    Integer		, Intent(in   ) :: notified_from
    Logical                             :: dummy

    dummy = check_outbox(this,event)

  End Subroutine mbox_check_sts_all
!!$------------------------------------------------------------------------------------------------------------------

  Function remote_access_new(pid) Result (this)

   Type(remote_access)  ,Pointer  	:: this
   Integer 		,Intent(in)	:: pid

   
   Allocate(this)   
   this%proc_id = pid 

  End Function remote_access_new
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_all_sync_rcvd(this,sync) Result(answer)

    Type(remote_access) ,Intent(inout) 	:: this
    Integer 		,Intent(in)     :: sync
    Logical 				:: answer
    Integer 		                :: all_sync,rcv_sync

    
    answer = .False.
    all_sync = Count (this%task_list(:)%id /= TASK_INVALID_ID .And. this%task_list(:)%type == sync) 
    rcv_sync = Count (this%task_list(:)%id /= TASK_INVALID_ID .And. this%task_list(:)%type == sync .And. &
                      this%task_list(:)%status == COMM_STS_SEND_COMPLETE ) 
    answer = all_sync == rcv_sync

  End Function rma_all_sync_rcvd
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_can_terminate(this)Result(answer)

    Type(remote_access) ,Intent(inout) 	:: this
    Logical  				:: answer
    Integer 				:: tclr,lclr  
    
    tclr = Count ( this%task_list(:)%status /= TASK_STS_CLEANED .And. this%task_list(:)%id /= TASK_INVALID_ID )
    lclr = Count ( this%lsnr_list(:)%status /= LSNR_STS_CLEANED .And. this%lsnr_list(:)%id /= LSNR_INVALID_ID )
    answer = ( tclr + lclr ) == 0 

  End Function rma_can_terminate
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_chunk_read(arg1,arg2,flag)

      Character(len=1)	,Dimension(:)	,Intent(inout)	:: arg1,arg2
      Real(kind=rfp)	,Dimension(:,:)	,Pointer 	:: ptr        
      Integer 		,Dimension(1:2)			:: rg,sp
      Type(remote_access)				:: this
      Type(point_set)					:: pts
      Integer 						:: np ,nd,nt,id,f,t,Type,i,j,k,l,flag

      pts  = Transfer ( arg1, pts  )
      this = Transfer ( arg2, this )
      arg1 = Transfer ( pts , arg1 )
      arg2 = Transfer ( this, arg2 )

    End Subroutine rma_chunk_read
!!$------------------------------------------------------------------------------------------------------------------

    Subroutine  rma_read_pts_chunk(this,fname,ch,wt)

        Character(len=1) ,Dimension(:),Pointer  :: arg2
        Character(len=*) ,Intent(in)		:: fname
        Integer          ,Intent(in)		:: ch 
        Logical          ,Intent(in)		:: wt 
        Type(remote_access)			:: this
        Type(point_set)				:: pts

        Allocate(arg2(1:sizeof(this)))
        arg2 = Transfer(this,arg2)
        Call read_pts_part(pts,fname,ch,wt,callback = rma_chunk_read,obj = arg2)
        this = Transfer(arg2,this)

    End Subroutine rma_read_pts_chunk
!!$------------------------------------------------------------------------------------------------------------------

    Function rma_create_data(this,name,nr,nc,version,pid) Result(dh)

    Type(remote_access) ,Intent(inout) 					:: this
    Integer		,Dimension(AXS_TYPE_LBOUND:AXS_TYPE_UBOUND)     :: vers
    Type(data_handle)   ,Pointer 					:: dh
    Type(data_handle)                 					:: data
    Integer 		              					:: pid,nr,nc,version
    Character(len=*) 	              					:: name

    

    Data = data_find_by_name_dbg(this,name,"remote_access_class.F90",674)
    If ( data%id /= DATA_INVALID_ID ) Then 
       dh => this%data_list(data%id)
    Else
       dh =>  data_create(this,name,nr,nc,version,pid)
    End If
    call instrument(EVENT_DATA_ADDED,0,0,0)

  End Function rma_create_data
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_create_task(this,name,proc,axs_list,wght,sync) Result(task)

    Type(remote_access),Intent(inout)   	:: this
    Type(task_info)    ,Pointer 		:: task
    Type(data_access)  ,Dimension(:),Intent(in) :: axs_list
    Integer            ,Intent(in),Optional     :: sync
    Integer            ,Intent(in) 	        :: proc,wght
    Character(len=*) 				:: name
    Integer                                     :: i

    
    
    Allocate(task)
    task%name     = name
    task%proc_id  = proc
    task%axs_list = axs_list
    task%status   = TASK_STS_INITIALIZED
    task%weight   = wght
    Nullify(task%dmngr)

    If (Present(sync)) Then 
       task%type = sync
    Else
       task%type = SYNC_TYPE_NONE 
    End If

       
    Call task_add_to_list(this,task)
    call instrument(EVENT_TASK_ADDED,task%id,0,0)
    If ( task%type /= SYNC_TYPE_NONE) Then 
          
       Return
    End If
    Do i = Lbound(task%axs_list,1),Ubound(task%axs_list,1)
       If ( .Not. Associated(task%axs_list(i)%data) ) Then 
          
          Cycle
       End If
          
       If (task%axs_list(i)%data%id /= DATA_INVALID_ID) Then 
          If ( task%axs_list(i)%access_types == AXS_TYPE_READ ) Then 
             call instrument(EVENT_DATA_DEP,task%id,task%axs_list(i)%data%id,0)
          Else
             call instrument(EVENT_DATA_DEP,task%id,task%axs_list(i)%data%id,1)
          End If
       Else
       End If
    End Do
    

  End Function rma_create_task
!!$------------------------------------------------------------------------------------------------------------------
  
  Function rma_create_access(this,data,rwm) Result(daxs)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in)    :: rwm    
    Type(data_access)  			:: daxs
    Type(data_handle)   , Intent(in)   	:: Data

    daxs%access_types = rwm
    daxs%dname=data%name
    daxs%data => this%data_list(Data%id)
    daxs%dproc_id = data%proc_id
    daxs%min_ver=data%version

  End Function rma_create_access
!!$------------------------------------------------------------------------------------------------------------------

  Function current_task_load(this) Result (load)

    Type(remote_access) , Intent(inout) :: this
    Integer 				:: load,lbm,idx

    lbm = opt_get_option(this%opt,OPT_LOAD_BALANCE)
    load = 0 
    If ( lbm == LBM_NODES_GROUPING ) Then 
       Do idx = 1, Ubound(this%task_list,1)
          If ( this%task_list(idx)%proc_id == this%proc_id  .And.  this%task_list(idx)%id  /= TASK_INVALID_ID  ) Then 
             load = load + this%task_list(idx)%weight
          End If
       End Do
    Else
       load = Count (this%task_list(:)%proc_id == this%proc_id     .And. &
                     this%task_list(:)%id      /= TASK_INVALID_ID          ) 
    End If
     
  End Function current_task_load

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine task_steal_send(this)
    Type(remote_access) , Intent(inout) :: this
    Integer                             :: idx
    Type(task_info)                     :: task
    Integer 				:: err,tag

    
    Do idx=1,Ubound(this%task_list,1)
       If (this%task_list(idx)%id /= TASK_INVALID_ID) Then 
          Exit
       End If
    End Do

    If (idx <= Ubound(this%task_req,1)) Then 
       this%task_steal_dest = this%task_steal_dest+1
       If (this%task_steal_dest == this%proc_id) Then 
          this%task_steal_dest = this%task_steal_dest+1
       End If
       If (this%task_steal_dest >= this%node_cnt) Then 
          this%task_steal_dest = this%node_cnt
          return 
       End If
       
       this%task_list(idx)%id=0
       Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
       

       call instrument(EVENT_TASK_SEND_REQUESTED,idx,this%task_list(idx)%proc_id,0)
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       
    End If

     
  End Subroutine task_steal_send

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine task_steal_reply(this,load,dest)
    Type(remote_access) , Intent(inout)  :: this
    Integer 		, Intent(in)	 :: load,dest
    Integer                              :: idx,rj_idx,ac_idx,i,err
    Type(task_info)                      :: task

     
    Do i=1,Ubound(this%task_list,1)    ! ToDo: Strategy for exporting which task??
       If ( this%task_list(i)%id /= TASK_INVALID_ID) Then 
          ac_idx=i
       Else
          rj_idx=i
       End If
    End Do

    If ( load >0 ) Then 
       idx=rj_idx
       this%task_list(idx)%type=TTYPE_TASK_STEAL_REJECTED
       
    Else
       idx=ac_idx
       
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
       Call lsnr_redirect(this,this%task_list(idx)%id,dest)
    End If
    
    Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
         dest,MPI_TAG_TASK_STEAL_RESPONSE,this%mpi_comm,this%task_req(idx),err) 
    this%task_list(idx)%status = COMM_STS_SEND_PROGRESS

  End Subroutine task_steal_reply

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine task_export_reply(this,load,dest)

    Type(remote_access) , Intent(inout)   :: this
    Integer 		, Intent(in)	  :: load,dest
    Integer                               :: idx,rj_idx,ac_idx,i,err
    Type(task_info)                       :: task

    
    Do i=1,Ubound(this%task_list,1)    ! ToDo: Strategy for exporting which task??
       If ( this%task_list(i)%id /= TASK_INVALID_ID) Then 
          ac_idx=i
       Else
          rj_idx=i
       End If
    End Do

    If ( load >0 ) Then 
       idx=rj_idx
       
       this%task_list(idx)%type=TTYPE_TASK_STEAL_REJECTED
    Else
       idx=ac_idx
       
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
    End If
    write(*,*)  "LBM, export reply, send response "," : ",idx
    Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
         dest,MPI_TAG_TASK_STEAL_RESPONSE,this%mpi_comm,this%task_req(idx),err) 
    this%task_list(idx)%status = COMM_STS_SEND_PROGRESS

  End Subroutine task_export_reply

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine lsnr_redirect(this, task_id,new_owner)
    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in)	:: task_id,new_owner
    Type(data_handle)                   :: Data
    Integer                             :: i,j
    Character(len=MAX_DATA_NAME)        :: wrt_data_name

    write(*,*)  "LBM, lsnr redirect,task,owner "," : ",task_id,new_owner
    Do i=1,Ubound(this%task_list,1)
       If (this%task_list(i)%id == task_id ) Then 
          Do j=1,Ubound(this%task_list(i)%axs_list,1)
             If (this%task_list(i)%axs_list(j)%access_types /= AXS_TYPE_READ) Then 
                wrt_data_name =  this%task_list(i)%axs_list(j)%data%name
             End If
          End Do
       End If
    End Do
    
    
    Data=data_find_by_name_dbg(this,wrt_data_name,"remote_access_class.F90",911)
    If (data%id == DATA_INVALID_ID) Then 
       ! Todo: create data for future references
    End If
    data%status = DATA_STS_REDIRECTED
    data%proc_id = new_owner
    this%data_list(data%id)%status  = DATA_STS_REDIRECTED
    this%data_list(data%id)%proc_id = new_owner 

  End Subroutine lsnr_redirect
!!$------------------------------------------------------------------------------------------------------------------
  Subroutine task_export(this)
    Type(remote_access) , Intent(inout)	:: this
    Integer                             :: idx
    Type(task_info)                     :: task
    Integer 				:: err,tag


    Do idx=1,Ubound(this%task_list,1)
       If (this%task_list(idx)%id /= TASK_INVALID_ID .and.  this%task_list(idx)%status== TASK_STS_INITIALIZED) Then 
          Exit
       End If
    End Do

    If (idx <= Ubound(this%task_req,1)) Then 
       this%task_steal_dest = this%task_steal_dest+1
       If (this%task_steal_dest == this%proc_id) Then 
          this%task_steal_dest = this%task_steal_dest+1
       End If
       If (this%task_steal_dest >= this%node_cnt) Then 
          this%task_steal_dest = this%node_cnt
          return 
       End If
       write(*,*)  "LBM,TASK export,dest"," : ",this%task_steal_dest
       write(*,*)  "LBM,task idx,proc,comm,name"," : ",idx,this%task_list(idx)%proc_id,this%mpi_comm,this%task_list(idx)%name
       this%task_list(idx)%id=0
       Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
!       write(*,*)  "LBM,tag,err,req,size,id,pid,name after ISEND"," : ",(MPI_TAG_TASK,err,this%task_req(idx),sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,this%task_list(idx)%name)

       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       write(*,*)  "LBM,task sts"," : ",idx,task_sts_name(this%task_list(idx)%status) 
    End If
  End Subroutine task_export

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine check_task_export(this) 
    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Logical 						:: flag
    Integer 						:: err,req,idx,load
    Type(task_info)                                     :: task




    write(*,*)  "LBM,CHECK TASK export"," : ","ENTER"

    Do idx = 1, Ubound(this%task_list,1)
       If ( this%task_list(idx)%id == TASK_INVALID_ID ) Then 
          Exit
       End If
    End Do
    
    load=current_task_load(this)-this%node_capacity


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)
# 984

    If ( flag ) Then 
       write(*,*)  "LBM,task export received  error,flag,sts,name"," : ",err,flag,sts,this%task_list(idx)%name


       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)


       write(*,*)  "LBM,task export received  error,flag,sts,name"," : ",err,flag,sts,this%task_list(idx)%name
!       
!G_TASK&
!# 997
!,             this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name))

       Call task_export_reply(this,load,sts(MPI_SOURCE))
    End If
    If (this%task_steal_dest <0 .And. load >0 ) Then 
       call task_export(this)
    End If

    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)
# 1008

    If ( flag ) Then 
       write(*,*)  "LBM,task export  received  error,flag,sts,name"," : ",err,flag,sts,this%task_list(idx)%name


       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)

       If ( this%task_list(idx)%type == TTYPE_TASK_STEAL_REJECTED) Then 
             Call task_export(this)
       End if
       If ( this%task_list(idx)%type == TTYPE_TASK_STEAL_ACCEPTED) Then 
          this%task_list(this%task_list(idx)%id)%proc_id =  sts(MPI_SOURCE)
          Call lsnr_redirect(this,this%task_list(idx)%id,sts(MPI_SOURCE))
       end if 
    End If

  End Subroutine check_task_export

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine check_task_steal(this) 
    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)   		:: this
    Logical 						:: flag
    Integer 						:: err,req,idx,load
    Type(task_info)                                     :: task



    Do idx = 1, Ubound(this%task_list,1)
       If ( this%task_list(idx)%id == TASK_INVALID_ID ) Then 
          Exit
       End If
    End Do
    
    load=current_task_load(this)-this%node_capacity


    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)

    If ( flag ) Then 
       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)
       Call task_steal_reply(this,load,sts(MPI_SOURCE))
    End If
    If (this%task_steal_dest <0 .And. load <0 ) Then 
       call task_steal_send(this)
    End If

    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)

    If ( flag ) Then 
       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)

       If ( this%task_list(idx)%type == TTYPE_TASK_STEAL_REJECTED) Then 
             Call task_steal_send(this)
       End if
       If ( this%task_list(idx)%type == TTYPE_TASK_STEAL_ACCEPTED) Then 
          this%task_list(idx)%type = SYNC_TYPE_NONE
          this%task_list(idx)%proc_id  = this%proc_id
          this%event=EVENT_TASK_RECEIVED
          Call task_check_sts(this,this%task_list(idx),this%event,this%notify ) 
       end if 
    End If

  End Subroutine  check_task_steal

!!$------------------------------------------------------------------------------------------------------------------
  Function rma_get_upper_node(this) Result (node)
    Type(remote_access),Intent(inout) :: this
    Integer                           :: node

    node = (this%node_cnt-1) / this%group_size

  End Function rma_get_upper_node

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_transfer_task(this, f,t,w)
    Type(remote_access), Intent(inout)  :: this
    Integer            , Intent(in)     :: f,t,w
    Integer                             :: idx,err
    Type(task_info)                     :: task

    write(*,*)  "LBM,Xfer task,from,to,wght"," : ",f,t,w

    Do idx=1,Ubound(this%task_list,1)
       If (this%task_list(idx)%id /= TASK_INVALID_ID) Then 
          Exit
       End If
    End Do

    If (idx <= Ubound(this%task_req,1)) Then 
       this%task_list(idx)%id=w
       this%task_list(idx)%proc_id=t
       Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            f,MPI_TAG_TRANSFER_TASK,this%mpi_comm,this%task_req(idx),err) 
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
    End If
    
  End Subroutine rma_transfer_task

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_local_balance(this)
    Type(remote_access),Intent(inout)   :: this
    Integer                             :: wl_cnt,mx_wl,i,mx,mn,mxi,mni, from_host,to_host,wl

    mx = 0 ; mxi = 0 
    mn = 0 ; mni = 0

    Do While ( .True. )
       Do i = 1, Ubound(this%work_load_list,1)
          If (this%work_load_list(i,WL_NODE_ID) <0 ) Exit
          If (this%work_load_list(i,WL_WEIGHT) >mx) Then 
             mx = this%work_load_list(i,WL_WEIGHT)
             mxi = i 
          End If
          If (this%work_load_list(i,WL_WEIGHT) <mn) Then 
             mn = this%work_load_list(i,WL_WEIGHT)
             mni = i 
          End If
       End Do
       If (mx ==0 .Or. mn == 0 ) Exit
       from_host = this%work_load_list(mxi,WL_NODE_ID)
       to_host   = this%work_load_list(mni,WL_NODE_ID)
       wl = Min(Abs(mn),mx)
       Call rma_transfer_task(this,from_host,to_host,wl)
       this%work_load_list(mxi,WL_WEIGHT) = this%work_load_list(mxi,WL_WEIGHT) - wl
       this%work_load_list(mni,WL_WEIGHT) = this%work_load_list(mni,WL_WEIGHT) + wl
    End Do

    wl_cnt = Count (this%work_load_list(:,WL_NODE_ID) > 0 ) 
    mx_wl = this%group_size
    If (this%proc_id == (this%node_cnt-1)/ this%group_size) Then 
       mx_wl = Mod(this%node_cnt-1,this%group_size)
    End If
    If ( wl_cnt >= mx_wl ) then 
       Call rma_send_load(this)
    End If
  End Subroutine rma_local_balance

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_export_load(this,w,t)
    Type(remote_access) , Intent(inout)   :: this
    Integer             , Intent(in)      :: w,t
    Integer                               :: idx,mxwi,xw

    xw=w
    Do While ( .True.)
       mxwi=-1
       Do idx = 1, Ubound(this%task_list,1)
          If (this%task_list(idx)%id == TASK_INVALID_ID) Exit
          If (this%task_list(idx)%weight <=xw ) Then 
             If ( mxwi <0 ) Then 
                mxwi = idx
             Else
                If (this%task_list(idx)%weight >this%task_list(mxwi)%weight ) Then 
                   mxwi = idx
                End If
             Endif
          End If
       End Do
       If (mxwi<0) Exit
       this%task_list(mxwi)%proc_id=t
       Call task_send( this,mxwi)
       Call lsnr_redirect(this,this%task_list(mxwi)%id,t)
       this%task_list(mxwi)%type=TTYPE_TASK_STEAL_ACCEPTED
       xw = xw-this%task_list(mxwi)%weight
    End Do

  End Subroutine rma_export_load

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_send_load(this)
    Type(remote_access) , Intent(inout)   :: this
    Integer                               :: xload,upper_node,msg_size,req,err

    xload = current_task_load(this) - this%node_capacity
    upper_node = rma_get_upper_node(this)
    msg_size= size(this%work_load_list) 

    If (.Not. this%single_load_sent ) Then  
       this%work_load_list(:,:)=0
       this%work_load_list(1,0)=this%proc_id
       this%work_load_list(1,1)=xload
    End If
    Call  MPI_ISEND (this%work_load_list(1,1),msg_size,MPI_INTEGER, & 
         upper_node,MPI_TAG_WORK_LOAD,this%mpi_comm,req,err) 

  End  Subroutine rma_send_load

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine check_group_bal(this)

    Integer		, Dimension(1:MPI_STATUS_SIZE)  :: sts
    Type(remote_access) , Intent(inout)                 :: this
    Logical                                             :: flag
    Integer                                             :: err,idx
    Type(task_info)                                     :: task

    If (.Not. this%single_load_sent ) Then  
       Call rma_send_load(this)
       this%single_load_sent= .True.
    End If

    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_WORK_LOAD, this%mpi_comm,flag,sts,err)

    If ( flag ) Then 
       Call MPI_RECV ( this%work_load_list(1,1), Size(this%work_load_list), MPI_INTEGER,&
            sts(MPI_SOURCE), MPI_TAG_WORK_LOAD, this%mpi_comm, sts, err)
       call rma_local_balance(this)
    End If

    Do idx=1,Ubound(this%task_list,1)
       If (this%task_list(idx)%id /= TASK_INVALID_ID) Then 
          Exit
       End If
    End Do

    Call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TRANSFER_TASK, this%mpi_comm,flag,sts,err)


    If (flag .And. idx <= Ubound(this%task_req,1)) Then 
       Call  MPI_IRECV (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            sts(MPI_SOURCE),MPI_TAG_TRANSFER_TASK,this%mpi_comm,this%task_req(idx),err) 
       Call rma_export_load(this,this%task_list(idx)%id,this%task_list(idx)%proc_id)
    End If

  End Subroutine check_group_bal

!!$------------------------------------------------------------------------------------------------------------------
  Function rma_cycle_one_step(this) Result(sch_event)

    Type(remote_access),Intent(inout)   :: this
    Integer 				:: in_event,out_event,sch_event,event,notify,lbm

    

    in_event  = EVENT_DONT_CARE 
    out_event = EVENT_DONT_CARE 
    sch_event = EVENT_DONT_CARE 
    
    Call check_inbox    (this,in_event )
    Do While (  check_outbox   (this,out_event) ) ; End Do
    Call check_scheduler(this,sch_event)

    Call task_check_sts_all(this,NOTIFY_DONT_CARE,this%event)
    Call lsnr_check_sts_all(this,NOTIFY_DONT_CARE,this%event)
    Call data_check_sts_all(this,NOTIFY_DONT_CARE,this%event)

    If ( event /= EVENT_DONT_CARE .Or. notify /= NOTIFY_DONT_CARE ) Then 
        
       Call print_lists(this)
    End If 

    lbm=opt_get_option(this%opt,OPT_LOAD_BALANCE)
    
    If (lbm /= LBM_NONE ) Then 
       If (lbm == LBM_TASK_STEALING) Then 
          Call check_task_steal(this)
       End if
       If (lbm == LBM_TASK_EXPORTING) Then 
          Call check_task_export(this)
       End if
       If (lbm == LBM_NODES_GROUPING) Then 
          Call check_group_bal(this)
       End if
    End If

    
    call instrument(EVENT_CYCLED,0,0,0)

  End Function rma_cycle_one_step
!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_generate_1task_4all(this)
    Type(remote_access) , Intent(inout)  :: this
    Integer                              :: i,j

    Do i = 0 , this%pc-1
       Call rma_generate_single_task(this,i)
    End Do


  End Subroutine rma_generate_1task_4all
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_generate_single_task(this,pid,idx)
    Integer             , Intent(in)  , Optional   :: idx
    Type(remote_access) , Intent(inout)  :: this
    Integer             , Intent(in)     :: pid
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,d
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,t_cnt ,owner,node_owns_data,bcast_mtd,i,j
    Character                            :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer 		    	  	 :: ii

    If ( Present(idx)  ) Then 
       write(*,*)  "Gen Single Task pid,idx"," : ",pid,idx
       If ( this%task_list(idx)%id == TASK_INVALID_ID )  then 
          Return 
       end if 

       Read (this%task_list(idx)%axs_list(1)%data%name,"(A1 I2)") dummy,i
       Read (this%task_list(idx)%axs_list(2)%data%name,"(A1 I2)") dummy,j
       j= j+1
       If ( j >= this%node_cnt ) Return 
    Else
       write(*,*)  "Gen Single Task pid,w/out idx"," : ",pid
       i = pid
       j = pid 
    End If
    write(*,*)  "Task for pid, Di,Dj"," : ",pid,i,j


    t_name = TNAME_ASM_MAT

    pi_name  = "P"//Trim(to_str(i))
    pj_name  = "P"//Trim(to_str(j))
    dij_name = "D"//to_str(i)//"_"//to_str(j)

    
    d = data_find_by_name_dbg(this,pi_name,"remote_access_class.F90",1375)
    If ( d%id == DATA_INVALID_ID ) Then 
       pi = rma_create_data(this,pi_name,this%part_size,this%nd,v,COORDINATOR_PID)
    End If
    d = data_find_by_name_dbg(this,pj_name,"remote_access_class.F90",1379)
    If ( d%id == DATA_INVALID_ID ) Then 
       pj = rma_create_data(this,pj_name,this%part_size,this%nd,v,COORDINATOR_PID)
    End If
    
    d = data_find_by_name_dbg(this,dij_name,"remote_access_class.F90",1384)
    If ( d%id == DATA_INVALID_ID ) Then 
       d = rma_create_data(this,dij_name,this%part_size,this%part_size,v,pid)
    End If
    
    axs(1) = rma_create_access ( this ,  pi , AXS_TYPE_READ  )
    axs(2) = rma_create_access ( this ,  pj , AXS_TYPE_READ  )
    axs(3) = rma_create_access ( this ,  d  , AXS_TYPE_WRITE )
    Do ii = 4,Ubound(axs,1)
       nullify(axs(ii)%data)
    End Do

    bcast_mtd = opt_get_option(this%opt,OPT_BROADCAST_MTD)
    If (bcast_mtd == BCASTMTD_PIPE) Then 
       If ( pid > 0 ) Then 
          axs(1)%data%proc_id = pid -1
          axs(2)%data%proc_id = pid -1
       End If
    End If
    node_owns_data = opt_get_option(this%opt,OPT_NODE_AUTONOMY)
    If (node_owns_data /= 0 ) Then 
       Read (axs(1)%data%name,"(A1 I2)") dummy,owner
       
       axs(1)%data%proc_id=owner
       Read (axs(2)%data%name,"(A1 I2)") dummy,owner
       
       axs(2)%data%proc_id=owner
    End If
    t = rma_create_task ( this , t_name , pid , axs ,1)


  End Subroutine rma_generate_single_task
!------------------------------------------------------------------------------------------------------
  Function rma_generate_tasks_part(this) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: this
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,k,l,t_cnt ,owner,node_owns_data,bcast_mtd,d_cnt,di_pid,dj_pid
    Character :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii


    axs(:)%dproc_id = -1
    axs(:)%dname    = ""
    t_name = TNAME_ASM_MAT
    t_cnt  = 0


    bcast_mtd = opt_get_option(this%opt,OPT_BROADCAST_MTD)
    node_owns_data = opt_get_option(this%opt,OPT_NODE_AUTONOMY)
    

    Do i = 0 , this%node_cnt-1       
       pid = Mod(i,this%node_cnt)
       If ( pid < this%proc_id ) Cycle
       If ( pid > this%proc_id + this%group_size-1) Cycle

       Do j = 0 , this%node_cnt-1
          Do k = 1,this%pc
             Do l = 1, this%pc
                pi_name  = "P"//Trim(to_str(i))//"_"//Trim(to_str(k))
                pj_name  = "P"//Trim(to_str(j))//"_"//Trim(to_str(l))
                
                di_pid = COORDINATOR_PID
                dj_pid = COORDINATOR_PID
                
                If ( bcast_mtd == BCASTMTD_PIPE .And. pid > 0 ) Then 
                   di_pid = pid -1
                   dj_pid = pid -1
                End If
                If (node_owns_data /= 0 ) Then 
                   di_pid=i
                   dj_pid=j
                End If
                pi = rma_create_data(this,pi_name,this%part_size,this%nd,v,di_pid)
                 
                pj = rma_create_data(this,pj_name,this%part_size,this%nd,v,dj_pid)
                 

                dij_name = "D"//to_str(i)//"_"//to_str(k)//"--"//to_str(j)//"_"//to_str(l)
                dij = rma_create_data(this,dij_name,this%part_size,this%part_size,v,pid)
                 

                axs(1) = rma_create_access ( this ,  pi , AXS_TYPE_READ  )
                axs(2) = rma_create_access ( this ,  pj , AXS_TYPE_READ  )
                axs(3) = rma_create_access ( this , dij , AXS_TYPE_WRITE )
                Do ii = 4,Ubound(axs,1)
                   nullify(axs(ii)%data)
                End Do

                 
                t = rma_create_task ( this , t_name , pid , axs ,1)
                t_cnt = t_cnt + 1 
             End Do
          End Do
       End Do
    End Do
    Call print_lists(this)


  End Function rma_generate_tasks_part


!!$------------------------------------------------------------------------------------------------------------------
!!$  Task_ij = {NodeID,'AssembleMatrix', Read(P_i),Read(P_j), Write(D_ij)}
!!$      The Task_xj's are sent to node x for computing the D_xj.
!!$      All the P_i and P_j Data has a CORDINATOR_PID as host ID which is the root node.
!!$      Therefore all the tasks requests for the P_i Data from the Root.
!!$      That is, all nodes send listeners to Root node for P_i.
!!$
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$      Read P_i   Read P_i    Read P_i   Read P_i  
!!$      Read P_j   Read P_j    Read P_j   Read P_j
!!$      Write D_0j Write D_1j  Write D_2j Write D_3j
!!$      
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$       P_i  ---> P_i 
!!$       P_i  ---------------> P_i 
!!$       P_i  --------------------------> P_i
!!$------------------------------------------------------------------------------------------------------------------
  Function rma_generate_tasks(rma,part_cnt,part_size,node_cnt,dim_cnt) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: rma
    Integer             , Intent(in)     :: part_cnt,part_size,node_cnt,dim_cnt
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,t_cnt ,owner,node_owns_data
    Character :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii


    t_name = TNAME_ASM_MAT
    t_cnt  = 0

    Do i = 0 , part_cnt-1

       pi_name  = "P"//Trim(to_str(i))
       pid = Mod(i,node_cnt)

       Do j = 0 , part_cnt-1
          pj_name  = "P"//Trim(to_str(j))
          pi = rma_create_data(rma,pi_name,part_size,dim_cnt,v,COORDINATOR_PID)
          pj = rma_create_data(rma,pj_name,part_size,dim_cnt,v,COORDINATOR_PID)
          
          dij_name = "D"//to_str(i)//"_"//to_str(j)
          dij = rma_create_data(rma,dij_name,part_size,part_size,v,pid)

          axs(1) = rma_create_access ( rma ,  pi , AXS_TYPE_READ  )
          axs(2) = rma_create_access ( rma ,  pj , AXS_TYPE_READ  )
          axs(3) = rma_create_access ( rma , dij , AXS_TYPE_WRITE )
          Do ii = 4,Ubound(axs,1)
             nullify(axs(ii)%data)
          End Do

          node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
          If (node_owns_data /= 0 ) Then 
             Read (axs(1)%data%name,"(A1 I2)") dummy,owner
             axs(1)%data%proc_id=owner
             Read (axs(2)%data%name,"(A1 I2)") dummy,owner
             axs(2)%data%proc_id=owner
          End If
          t = rma_create_task ( rma , t_name , pid , axs ,1)
          t_cnt = t_cnt + 1
       End Do
    End Do

    write(*,*)  "DataObject#"," : ",t_cnt*3
    write(*,*)  "TaskObject#"," : ",t_cnt 

  End Function rma_generate_tasks
!!$------------------------------------------------------------------------------------------------------------------
!!$  Task_ij = {NodeID,'AssembleMatrix', Read(P_i),Read(P_j), Write(D_ij)}
!!$      The Task_xj's are sent to node x for computing the D_xj.
!!$      All the P_i for Task_xj are read from node 'x-1' for x>0. Root node does not request any Data.
!!$      Therefore all the tasks requests for the P_i Data from their left neighbor.(in line topology)
!!$
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$      Read P_i   Read P_i    Read P_i   Read P_i  
!!$      Read P_j   Read P_j    Read P_j   Read P_j
!!$      Write D_0j Write D_1j  Write D_2j Write D_3j
!!$      
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$       P_i ----> P_i  ------> P_i -----> P_i
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_generate_tasks_pipeline(rma,part_cnt,part_size,node_cnt,dim_cnt) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: rma
    Integer             , Intent(in)     :: part_cnt,part_size,node_cnt,dim_cnt
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,t_cnt ,owner,node_owns_data
    Character::dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii

    t_name = TNAME_ASM_MAT
    t_cnt  = 0

    Do i = 0 , part_cnt-1

       pi_name  = "P"//Trim(to_str(i))
       pid = Mod(i,node_cnt)

       Do j = 0 , part_cnt-1
          pj_name  = "P"//Trim(to_str(j))
          pi = rma_create_data(rma,pi_name,part_size,dim_cnt,v,COORDINATOR_PID)
          pj = rma_create_data(rma,pj_name,part_size,dim_cnt,v,COORDINATOR_PID)
          
          dij_name = "D"//to_str(i)//"_"//to_str(j)
          dij = rma_create_data(rma,dij_name,part_size,part_size,v,pid)

          axs(1) = rma_create_access ( rma ,  pi , AXS_TYPE_READ  )
          axs(2) = rma_create_access ( rma ,  pj , AXS_TYPE_READ  )
          axs(3) = rma_create_access ( rma , dij , AXS_TYPE_WRITE )
          Do ii = 3,Ubound(axs,1)
             nullify(axs(ii)%data)
          End Do

          If ( pid > 0 ) Then 
             axs(1)%data%proc_id = pid -1
             axs(2)%data%proc_id = pid -1
          End If
          node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
          If (node_owns_data /= 0 ) Then 
             Read (axs(1)%data%name,"(A1 I2)") dummy,owner
             
             axs(1)%data%proc_id=owner
             Read (axs(2)%data%name,"(A1 I2)") dummy,owner
             
             axs(2)%data%proc_id=owner
          End If
          t = rma_create_task ( rma , t_name , pid , axs ,1)
          t_cnt = t_cnt + 1

       End Do
    End Do
    write(*,*)  "DataObject#"," : ",t_cnt*3
    write(*,*)  "TaskObject#"," : ",t_cnt 

  End Function rma_generate_tasks_pipeline
!!$------------------------------------------------------------------------------------------------------------------


  Subroutine rma_init_chol_data(this) 
    Type(remote_access)	            :: this
    Integer 		            :: r,c
    Character(len=MAX_DATA_NAME)    :: dname
    Type(data_handle)		    :: d
    Type(point_set)                 :: pts
    integer :: part,p
    part = opt_get_option(this%opt,OPT_CHOL_PART_CNT)
    do p=0,part-1
       r = this%proc_id * part + p 
       do c= 0, r
          dname = "M_"//to_str(r)//"_"//to_str(c)//"_-1_00"
          d = rma_create_data(this,dname,this%np ,this%np,0,this%proc_id)
          
          call data_populate (this,dname,pts) 
       end do
    end do

  End Subroutine rma_init_chol_data

!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_init_this(this) 

    Type(remote_access)	:: this
    Integer 		:: i,d_cnt,t_cnt,inst_cnt

    inst_cnt = this%node_cnt
    t_cnt = (inst_cnt * this%pc)**2+inst_cnt+1
    d_cnt =  t_cnt + inst_cnt * this%pc +31

    If (opt_get_option(this%opt,OPT_CHOLESKY) /=0) Then 
       t_cnt = (this%node_cnt*5) **2
       d_cnt = t_cnt*3 
    End If
    write(*,*)  "Task,Data Cnt ,GrpSize,InstCnt,Nc"," : ",t_cnt,d_cnt,this%group_size,inst_cnt,this%nc
    
    

    Allocate ( this%task_list(1:t_cnt ) )
    Allocate ( this%data_list(1:d_cnt ) )
    Allocate ( this%lsnr_list(1:d_cnt ) )

    Allocate ( this%task_req(1:t_cnt ) ) 
    Allocate ( this%lsnr_req(1:d_cnt ) ) 
    Allocate ( this%data_req(1:d_cnt ) ) 


   this%data_list (:)%status = DATA_STS_CLEANED
   this%task_list (:)%status = TASK_STS_CLEANED
   this%lsnr_list (:)%status = LSNR_STS_CLEANED

   this%data_list (:)%name = ""

   this%data_list (:)%id = DATA_INVALID_ID
   this%task_list (:)%id = TASK_INVALID_ID
   this%lsnr_list (:)%id = LSNR_INVALID_ID

   Do i = Lbound(this%task_list ,1),Ubound(this%task_list ,1)
      this%task_list (i)%type = SYNC_TYPE_NONE
   End Do



   this%USE_MPI_RMA = .False. 
   this%USE_MPI_COMM = .True.
   this%task_cnt = 0 
   this%mpi_comm = MPI_COMM_WORLD

   this%task_req(:) = MPI_REQUEST_NULL
   this%data_req(:) = MPI_REQUEST_NULL
   this%lsnr_req(:) = MPI_REQUEST_NULL

   this%send_list       => ll_new_list()
   this%tlist_ready_sts => ll_new_list()

   

  End Subroutine rma_init_this
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_import_tasks(this,fname)
    Type(remote_access) , Intent(inout)        :: this
    Character(len=*)    , Intent(in)           :: fname
    Character (len=13)                         :: name
    Type(data_handle)		 	       :: d
    Type(task_info)		  	       :: t 
    Integer                                    :: iostat,part_size=100,i=1,val,ver,xcode,r,c
    Character(len=1)                           :: obj,prefix,sep
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK)       :: axs
    Integer                             :: ii


    write(*,*)  "Import Tasks"," : ","Enter"
    axs(:)%dname   = ""
    axs(:)%min_ver = 0 

    Open ( UNIT = 38,FILE = fname)
    iostat = 0

! TODO: no rows in node
    part_size=this%np


    Do While ( iostat == 0 ) 

       Read(38,"(A1 A14 I3)", IOSTAT = iostat) obj,name,val
       If ( iostat /= 0 ) Exit

       

       Select Case(obj)
          !Case ('I')
          Case ('NoI')
             
             axs(:)%min_ver   = 0 
             d = data_find_by_name_dbg(this,name,"remote_access_class.F90",1762)
             If ( d%id == DATA_INVALID_ID ) Then 
                d = rma_create_data(this,name,part_size,2,0,val)
                
             End If
             d = data_find_by_name_dbg(this,name,"remote_access_class.F90",1767)
             axs(1)=rma_create_access(this,d,AXS_TYPE_WRITE)
             axs(1)%dname=name
             Do ii = 2,Ubound(axs,1)
                Nullify(axs(ii)%data)
             End Do
             t = rma_create_task ( this, TNAME_INIT_DATA , val,axs,0,SYNC_TYPE_NONE ) 
             axs(:)%min_ver   = 0 
          Case ('D')
             
             Read ( name,"(A1 A1 I2 A1 I2 A1 I2 A1 I2)") obj,sep,r,sep,c,sep,xcode,sep ,ver
             If ( obj == 'X' .and. xcode == -1 ) Then 
                name=obj//sep//to_str(r)//sep//to_str(c)
                
             Else               
                ver=0
             End If
             d = data_find_by_name_dbg(this,name,"remote_access_class.F90",1784)
             If ( d%id == DATA_INVALID_ID ) Then 
                d = rma_create_data(this,name,part_size,2,0,val)
                
             End If
          Case ('A')
             
             Read ( name,"(A1 A1 I2 A1 I2 A1 I2 A1 I2)") obj,sep,r,sep,c,sep,xcode,sep ,ver
             If ( obj == 'X' .and. xcode == -1 ) Then 
                name=obj//sep//to_str(r)//sep//to_str(c)
             Else               
                ver=0
             End If
             d = data_find_by_name_dbg(this,name,"remote_access_class.F90",1798)
             axs(i)=rma_create_access(this,d,val)
             axs(i)%data=>this%data_list(d%id)
             axs(i)%dname   = name
             If ( val == AXS_TYPE_WRITE ) ver = 0 
             axs(i)%min_ver = ver
             i = i+1
          Case ('T')
             t = rma_create_task ( this, name , val,axs ,0,SYNC_TYPE_NONE) 
             i=1
             axs(:)%dname=""
             Nullify(axs(1)%data,axs(2)%data,axs(3)%data,axs(4)%data)
       End Select
    End Do
    Close(38)
    
    Call print_lists(this)

  End Subroutine rma_import_tasks
!!$------------------------------------------------------------------------------------------------------------------
  Function rma_make_pts(this,np,nt,dim_cnt) Result(pts)

    Type(remote_access) , Intent(inout)        :: this
    Integer             , Intent(in)           :: np,nt,dim_cnt
    Type ( point_set)                          :: pts
    Real(kind=rfp)     ,Dimension(:,:),Pointer :: x
    Integer            ,Dimension(:,:),Pointer :: id
    Integer                      :: i ,j,l

    Allocate ( x(1:np,1:dim_cnt) , id (1:nt,1:ID_COL_MAX)  ) 
    id = 0 
    x = 0.0
    Do i = 1, Ubound(x,1)
       Do j = 1, Ubound(x,2)
          x(i,j) = (i*10_rfp +j) / 1000.0_rfp
       End Do
    End Do

    j = Ubound(x,1) / Ubound(id,1)
    Do i = 1, Ubound(id,1)
       id(i,1) = i 
       id(i,ID_COL_FROM) = (i-1)*j+1
       id(i,ID_COL_TO  ) = (i  )*j
    End Do
    
    
    pts = new_pts(id,x)
    
  End Function rma_make_pts
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_make_this(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nc) Result(rma)

    Type(remote_access)          :: rma
    Integer         , Intent(in) :: pid,part_cnt,part_size,node_cnt,dim_cnt,np,nc
    Type(wfm)       , Intent(in) :: wf
    Type(options)   , Intent(in) :: opt
    Type(data_handle)            :: Data
    Real(kind=rfp)               :: pts_element
    Integer                      :: ids_element,instance_cnt

    rma = remote_access_new(pid)
    rma%sync_stage=0
    rma%np  = np/part_cnt
    rma%nc  = nc
    rma%nd  = dim_cnt
    rma%pc  = part_cnt
    rma%wf  = wf
    rma%opt = opt
    rma%node_cnt         = node_cnt
    rma%part_size        = part_size
    rma%data_buf_size    = sizeof(Data) +2*sizeof(ids_element)*ID_COL_MAX+np* dim_cnt * sizeof(pts_element)
    rma%single_load_sent = .False.
    rma%group_size       = opt_get_option(rma%opt,OPT_GROUP_SIZE )
    instance_cnt = node_cnt
    If ( opt_get_option(rma%opt,OPT_PURE_MPI ) /= 0 ) instance_cnt = node_cnt* nc
    If ( rma%group_size == 0 .Or. rma%group_size > instance_cnt) rma%group_size=instance_cnt
    rma%node_capacity    = opt_get_option(rma%opt,OPT_NODE_CAPACITY)
    rma%task_steal_dest  = -1
    If (opt_get_option(rma%opt,OPT_CHOLESKY)  /=0 ) Then 
       rma%data_buf_size = sizeof(Data)+np*np*sizeof(pts_element)
       rma%np = np / opt_get_option(rma%opt,OPT_CHOL_PART_CNT)
    End If
    Call rma_init_this(rma)

    write(*,*)  "@@data buf size "," : ",rma%data_buf_size,np,nc

  End Function rma_make_this
!---------------------------------------------------------------------------------------------------------------------------
  Function rma_mat_assemble(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt) Result (rma) 

    Integer         , Intent(in) :: pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt
    Type(wfm)       , Intent(in) :: wf
    Type(options)   , Intent(in) :: opt
    Type(remote_access)          :: rma
    Integer 		         :: cyc = 0,tc,err,event=EVENT_DONT_CARE,ids_element,lnp,lnd,lnt
    Type(data_handle)            :: Data
    Type ( point_set)            :: pts
    Character(len=5)             :: env
    Character(len=MAX_DATA_NAME) :: dname
    Integer                      :: part_idx = 1 ,time_out,node_owns_data,i,j=1,bcast_mtd,seq,import_tasks,lbm,tasks_node,cholesky
    Integer(kind=8)              :: task_dst
    Integer(kind=8)              :: time,drdy,dlag,dnow
    Real(kind=rfp)::start
    Type(task_info)::task
    Type(listener)::lsnr
    Logical :: populated = .False.



    Call Get_environment_variable("RMA_TIME_OUT",env)
    Read (env,"(I3)") time_out
    
     

    rma = rma_make_this(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt)
    import_tasks = opt_get_option(rma%opt,OPT_IMPORT_TASKS)
    seq = opt_get_option(rma%opt,OPT_SEQUENTIAL)
    if ( seq /= 0 .and. import_tasks ==0) then 
       write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
       Call rma_mat_assemble_seq(rma)
       write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
       Return
    End If
    
    pts = rma_make_pts(rma,rma%np,nt,dim_cnt)
    Call pts_get_info(pts,lnp,lnd,lnt)
    


    time_out= opt_get_option(rma%opt,OPT_TIME_OUT)

    write(*,*)  "@@data buf size1 "," : ",rma%data_buf_size,np,nt
    Allocate ( rma%data_buf  (rma%data_buf_size) )  
    write(*,*)  "@@data buf size2 "," : ",rma%data_buf_size,np,nt
    Allocate ( rma%data_buf2 (rma%data_buf_size) )
!!! MT: Dead code? Packs invalid Datas and corrupts memory
!    Call pack_data_pts(rma,Data,pts,rma%data_buf ,rma%data_buf_size)
!    Call pack_data_pts(rma,Data,pts,rma%data_buf2,rma%data_buf_size)

    If (import_tasks /= 0) Then 
       Call rma_init_chol_data(rma)
       if ( seq /= 0) then 
           write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
           data=data_find_by_name_dbg(rma,"M_00_00_-1_00","",0)
           if ( seq == 1 ) then 
              call chol_sequential(rma%data_list(data%id)%mat)
           elseif ( seq == 2 ) then 
              call chol_sequential_block(rma%data_list(data%id)%mat,nt)
           else
              call chol_sequential_cache_opt(rma%data_list(data%id)%mat,nt)
           end if
           write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
           call dump_timing()
           call print_chol_data(rma)
           call check_chol_result(rma)
           return
       end if
       If ( pid ==0 ) Then 
          rma%sync_stage = SYNC_TYPE_SENDING_TASKS
          Call rma_import_tasks(rma,"chtasks.txt")
          Call rma_send_all_tasks(rma) 
       End If
       
    Else
       node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
       write(*,*) event_name(EVENT_DTLIB_DIST),',',(MPI_Wtime()),',',EVENT_DTLIB_DIST,',',0,',',0,',',0
       lbm = opt_get_option (rma%opt,OPT_LOAD_BALANCE)
       tasks_node = 0
       If (lbm /= LBM_NONE )  Then 
          tasks_node = 0 !node_cnt-1
       End If
       If ( Mod(pid,rma%group_size) == 0  ) Then 
          rma%sync_stage = SYNC_TYPE_SENDING_TASKS
          task_dst = opt_get_option(rma%opt,OPT_TASK_DISTRIBUTION)
          If ( task_dst == TASK_DIST_ONCE ) Then 
             tc = rma_generate_tasks_part(rma)
          Else
             Call rma_generate_1task_4all(rma)
          End If
          Call schd_update_dependences(rma)
          If (lbm == LBM_NONE ) Then 
             Call rma_send_all_tasks(rma) 
          Else
             rma%task_list(:)%proc_id = pid
          End If
       Else
          If (node_owns_data ==0 ) & 
               Call set_pts_zero(pts)
       End If
    End If



    Call MPI_BARRIER(rma%mpi_comm,err)



    write(*,*) event_name(EVENT_DTLIB_DIST),',',(MPI_Wtime()),',',EVENT_DTLIB_DIST,',',0,',',0,',',0
    dlag = opt_get_option(rma%opt,OPT_DATA_READY_LAG)


    time = getime()/1       ;    
    drdy = time

    write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
    start=MPI_Wtime()

    cholesky=opt_get_option(rma%opt,OPT_CHOLESKY)
    Call print_lists(rma)
    Do While(  rma_cycle_one_step(rma) /= EVENT_ALL_TASK_FINISHED )
    
       dnow = getime()/1        - drdy 
       If ( cholesky ==0 ) Then 
          If (node_owns_data /= 0 ) Then 
             If (  (dnow >= dlag  .And. rma%sync_stage > SYNC_TYPE_SENDING_TASKS)  .And. drdy /= 0) Then 
                dname = "P"//to_str(rma%proc_id)//"_"//to_str(j)
                Call data_populate(rma,dname,pts)
                j = j+1
                If ( j > rma%pc)drdy =0 
                dnow = getime()/1        
                write(*,*)  "Populated P"," : ",rma%proc_id,dname ,getime()/1e3 - drdy ,rma%pc,rma%sync_stage
             End If
          Else          
             If ( .Not. populated) Then 
                Do i=0,rma%node_cnt-1
                   Do j=1,rma%pc
                      dname = "P"//to_str(i)//"_"//to_str(j)
                      Call data_populate(rma,dname,pts)
                      write(*,*)  "Populated P"," : ",rma%proc_id,dname ,dnow ,rma%pc,rma%sync_stage
                   End Do
                End Do
                populated = .True.
             End If
          End If
       Else
          drdy=0
          dnow = getime()/1        
       End If
       If ( (dnow-time) > time_out*1e6 ) Then 
          Write (*,*) " TBL TimeOut #",dnow,time,(dnow-time) , time_out*1       
          Exit
       End If


    End Do

    write(*,*) event_name(EVENT_DTLIB_EXEC),',',(MPI_Wtime()),',',EVENT_DTLIB_EXEC,',',0,',',0,',',0
    call dump_timing()
    Call print_lists(rma,.true.)
    Call check_chol_result(rma)
		
  End Function rma_mat_assemble
!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_mat_assemble_seq(this)
    Type(remote_access) , Intent(inout)	           :: this
    Real(kind=rfp)      , Dimension(:,:), Pointer  :: d,r
    Real(kind=rfp)      , Dimension(:,:), Pointer  :: x
    Integer                                        :: i,j,np,nd,nb,seqchunk,l,k
    Character(len=80)			           :: phi
    Character				           :: nprime
    Real(kind=rfp)			           :: eps

    np = opt_get_option(this%opt,OPT_CHUNK_PTS)
    seqchunk = opt_get_option(this%opt,OPT_SEQ_CHUNK)
    nb = Max(np/seqchunk,1)
    If ( nb == 1 ) seqchunk = np
    write(*,*)  "SEQ_CHUNK,nb"," : ",seqchunk,nb
    Do l=1,nb
       Do k=1,nb
          Allocate(x(seqchunk,1:2),d(seqchunk,seqchunk),r(seqchunk,seqchunk))
          Do i=1,Ubound(x,1)
             Do j=1,Ubound(x,1)
                d(i,j)= Sqrt((x(j,1)-x(i,1))**2+(x(j,2)-x(i,2))**2)
             End Do
          End Do
          eps = 2.0
          phi="gauss"
          nprime='0'
          nd = 2

          Call dphi(phi, nprime, nd, eps, d, r)
          Deallocate(x,d,r)
       End Do
    End Do


  End Subroutine rma_mat_assemble_seq
!!$------------------------------------------------------------------------------------------------------------------
# 2173

  Subroutine rma_save_data(this)
    Type(remote_access) ,Intent(inout)	     :: this
  End Subroutine rma_save_data

!!$------------------------------------------------------------------------------------------------------------------
  Function rma_send_all_lsnr(this) Result(all_sent)

    Type(remote_access),Intent(inout)	:: this
    Logical 				:: all_sent
    Integer 				:: remote_cnt,lsnr_cnt,i,task_cnt

    all_sent=.False.
    If ( .Not. Associated (ll_get_cur(this%send_list)) ) Then 

!       if ( count ( this%task_list(:)%id /= TASK_INVALID_ID) == 0 ) return

       task_cnt = Count ( this%task_list(:)%id /= TASK_INVALID_ID .and. this%task_list(:)%status == TASK_STS_INITIALIZED ) 
       write(*,*)  "Non-processed Tasks' Cnt"," : ",task_cnt
       
       If ( task_cnt /= 0 ) Return 
       
       lsnr_cnt = Count ( this%lsnr_list(:)%id /= LSNR_INVALID_ID  .And. &
                          !(this%lsnr_list(:)%status == LSNR_STS_ACTIVE .or. & 
                           this%lsnr_list(:)%status == COMM_STS_SEND_PROGRESS ) 
       lsnr_cnt = lsnr_cnt + count (this%lsnr_req(:) /= MPI_REQUEST_NULL ) 
       write(*,*)  "Non-sent Lsnr' Cnt"," : ",lsnr_cnt
       
       If ( lsnr_cnt /= 0 ) Return 
       
       Write(*,*) "TBL -------------------------------------------------------------Lsnr-"
       Write(*,*) "TBL id pid status d:id,pid,name         "
       Do i= Lbound(this%lsnr_list,1),Ubound(this%lsnr_list,1)
          If ( .Not. Associated ( this%lsnr_list(i)%req_data ) ) Cycle
          If (this%lsnr_list(i)%id /= LSNR_INVALID_ID ) Then 
             Write (*,*) "TBL",this%lsnr_list(i)%id,this%lsnr_list(i)%proc_id &
                  ,lsnr_sts_name(this%lsnr_list(i)%status),this%lsnr_list(i)%req_data%id &
                  ,this%lsnr_list(i)%req_data%proc_id,this%lsnr_list(i)%req_data%name
          End If
       End Do
       Write(*,*) "TBL =================================================================="
       all_sent=.True.
    End If
    

  End Function rma_send_all_lsnr
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_send_all_tasks(this)

    Type(remote_access) ,Intent(inout)  :: this
    Integer                             :: rmt_cnt , snt_cnt

    rmt_cnt = 0
    snt_cnt = 1

    Do While ( rmt_cnt /= snt_cnt ) 
       rmt_cnt = Count ( this%task_list(:)%proc_id /= this%proc_id     .And. &
                         this%task_list(:)%id      /= TASK_INVALID_ID          ) 
       snt_cnt = Count ( this%task_list(:)%proc_id /= this%proc_id     .And. &
                         this%task_list(:)%id      /= TASK_INVALID_ID  .And. &
                         this%task_list(:)%status  /= COMM_STS_SEND_INIT       ) 
       Call task_check_sts_all(this,NOTIFY_DONT_CARE,this%event)
    End Do

  End Subroutine rma_send_all_tasks
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_sync_bcast(this,sync_type)

    Type(remote_access) ,Intent(inout)  :: this
    Integer             ,Intent(in)     :: sync_type
    Type(task_info) 			:: task
    Integer 				:: i,n_p,err,req
    Type(data_access)   , &
      Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii

    
    Do ii = Lbound(axs,1),Ubound(axs,1)
       nullify(axs(ii)%data)
    End Do
    axs(:)%dproc_id = -1
    axs(:)%dname=""
    Call MPI_COMM_SIZE(this%mpi_comm,n_p,err)
# 2240

    Do i = 0,n_p-1
       If (i /= COORDINATOR_PID) Then 
          task = rma_create_task(this,"SYNC_"//Trim(stage_name(sync_type)),i,axs,0,sync_type)
       End If 
    End Do

    

  End Subroutine rma_sync_bcast
!!$------------------------------------------------------------------------------------------------------------------
!!$  Returns TRUE or FALSE indicating that synchronization with other rocessors has been successful.
!!$  Cases that may happen are:
!!$  A. LAST_TASK sync
!!$     1. Not root process, return FALSE.
!!$     2. Not already sent?
!!$        Broadcasts sync to all the nodes.
!!$        Return TRUE.
!!$  B. TERM_OK sync
!!$     1. For Root node 
!!$      1-1. Can terminate now? No. Return FLASE.
!!$      1-2. Sent already?
!!$           No.Broadcasts the TERM OK sync.
!!$           Yes. Are all of them are received?                 
!!$                No. Return FALSE.
!!$                Yes. Return TRUE.
!!$     2.For non-Root nodes:
!!$       2-1 If ever received the sync TERM OK ?
!!$           No. Returns FALSE.
!!$           Yes. Clean all data. Return TRUE.
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_synchronize(this,sync)Result(answer)

    Type(remote_access), Intent(inout)	:: this
    Integer            , Intent(in)     :: sync
    Logical                             :: answer

    

    
    answer = .False.
    Select Case(sync)

       Case (SYNC_TYPE_LAST_TASK)
          If ( this%proc_id /= COORDINATOR_PID ) Return
          If ( this%sync_stage == SYNC_TYPE_SENDING_TASKS ) Then 
             Call  rma_sync_bcast(this,SYNC_TYPE_LAST_TASK)
             this%sync_stage = SYNC_TYPE_LAST_TASK
             
             answer = .True.
          End If

       Case (SYNC_TYPE_TERM_OK)
          If ( this%proc_id == COORDINATOR_PID ) Then
             If (rma_can_terminate(this)) Then 
                If ( this%sync_stage >= SYNC_TYPE_LAST_TASK .And. this%sync_stage < SYNC_TYPE_TERM_OK ) Then 
                   Call rma_save_data(this)
                   Call rma_sync_bcast(this,SYNC_TYPE_TERM_OK)
                   this%sync_stage = SYNC_TYPE_TERM_OK
                   
                Else
                   If ( this%sync_stage == SYNC_TYPE_TERM_OK ) Then 
                      answer = rma_all_sync_rcvd(this,SYNC_TYPE_TERM_OK)
                   End If
                End If
             End If
          Else ! non-COORDINATOR nodes
             answer = this%sync_stage == SYNC_TYPE_TERM_OK
             If ( answer ) Then 
                Call rma_save_data(this)
                write(*,*)  "Data Clean All Called"," : ",this%sync_stage,SYNC_TYPE_TERM_OK
                Call data_clean_all(this)
             End If
          End If       

    End Select

    

  End Function rma_synchronize
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_task_check_sts(this,task,event)

    Type(remote_access),Intent(inout) :: this
    Integer            ,Intent(inout) :: event
    Type(task_info)    ,Intent(inout) :: task

    Call task_check_sts(this,task,event,this%notify)
    If ( this%notify > 0 ) Then 
       If ( ( mod(this%notify,NOTIFY_OBJ_DATA*10)/NOTIFY_OBJ_DATA ) /= 0&
# 2330
                                                       ) & 
            Call data_check_sts_all(this,NOTIFY_OBJ_TASK,event)
       If ( ( mod(this%notify,NOTIFY_OBJ_TASK*10)/NOTIFY_OBJ_TASK ) /= 0&
# 2332
                                                       ) & 
            Call task_check_sts_all(this,NOTIFY_OBJ_TASK,event)
       If ( ( mod(this%notify,NOTIFY_OBJ_LISTENER*10)/NOTIFY_OBJ_LISTENER ) /= 0&
# 2334
                                                       ) & 
            Call lsnr_check_sts_all(this,NOTIFY_OBJ_TASK,event)
       If ( ( mod(this%notify,NOTIFY_OBJ_MAILBOX*10)/NOTIFY_OBJ_MAILBOX ) /= 0&
# 2336
                                                       ) & 
            Call mbox_check_sts_all(this,NOTIFY_OBJ_TASK,event)
       If ( ( mod(this%notify,NOTIFY_OBJ_SCHEDULER*10)/NOTIFY_OBJ_SCHEDULER ) /= 0&
# 2338
                                                       ) & 
            Call schd_check_sts_all(this,NOTIFY_OBJ_TASK,event)
    End If


  End Subroutine rma_task_check_sts
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_test_data_packing(this)

    Type(remote_access),Intent(inout)	          :: this
    Real(kind=rfp)     ,Dimension(:,:) , Pointer  :: x
    Integer            ,Dimension(:,:) , Pointer  :: id
    Real(kind=rfp)     ,Dimension(:,:) , Pointer  :: pts_ptr,pts_ptr2        
    Integer            ,Dimension(:,:) , Pointer  :: ids_ptr,ids_ptr2
    Character(len=1)   ,Dimension(:)   , Pointer  :: buf
    Integer            ,Dimension(1:2)            :: rg
    Integer                                       :: i ,j,l,buf_size,idx
    Type(data_handle)                             :: d1,d2
    Type(point_set)                               :: pts

    Allocate ( x(1:1240,1:2) , id (1:4,1:ID_COL_MAX)  ) 

    
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
    
    

    d1%pts = new_pts(id,x)

    rg     = range_pts   (d1%pts)
    d1%np  = rg(2) - rg(1)+1
    d1%nd  = dims_pts    (d1%pts)
    d1%nt  = range_pts_id(d1%pts)

    Allocate ( ids_ptr (1:d1%nt,1:ID_COL_MAX) , pts_ptr (1:d1%np,1:d1%nd) ) 

    ids_ptr = get_ids_ptr(d1%pts)
    pts_ptr = get_pts_ptr(d1%pts)

    Call       pack_data_pts(this,d1,d1%pts,buf,buf_size)
    Call rma_unpack_data_pts(this,buf,buf_size,idx) 
    
    d2 = this%data_list(idx)
    Allocate ( ids_ptr2 (1:d2%nt,1:ID_COL_MAX) , pts_ptr2 (1:d2%np,1:d2%nd) ) 
    ids_ptr2 = get_ids_ptr(d2%pts)
    pts_ptr2 = get_pts_ptr(d2%pts)

  End Subroutine rma_test_data_packing
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine   rma_unpack_data_pts(this,buf,buf_size,idx) 

    Type(remote_access),Intent(inout)	                     :: this
    Type(data_handle)                                        :: Data,d2
    Integer            ,Intent(in)                           :: buf_size
    Integer , Intent(inout) :: idx
    Character(len=1)   ,Dimension(:)   , Pointer,Intent(in)  :: buf
    Real(kind=rfp)     ,Dimension(:,:) , Pointer             :: pts_ptr        
    Integer            ,Dimension(:,:) , Pointer             :: ids_ptr
    Real ( kind=rfp)                                         :: pts_element
    Integer                                                  :: ids_elements,choltrf,i
    Integer                                                  :: pts_offset,ids_offset,dim_cnt,d_size,mat_offset

    d_size = sizeof(data%name) + sizeof(data%id)*13 +1 

    Data = Transfer ( buf(1:d_size), Data)
    

    If ( opt_get_option(this%opt,OPT_CHOLESKY) /= 0 )  Then 
       
       d2 = data_find_by_name_dbg(this,data%name,"remote_access_class.F90",2434) 
       If ( d2%id /= DATA_INVALID_ID) Then 
          idx = d2%id
          
          Allocate (this%data_list(idx)%mat ( 1:this%data_list(idx)%nrows,1:this%data_list(idx)%ncols) )
          mat_offset = d_size + sizeof(d2%pts) + sizeof(d2%buf)    +1
          
          this%data_list(idx)%mat(:,:)  = Reshape ( Transfer (buf(mat_offset:buf_size), &
                                                              0.0_rfp, &
                                                              (buf_size-mat_offset+1)) &
                                                    , (/ d2%nrows,d2%ncols /) ) 
          this%data_list(idx)%nd  = data%nd
          this%data_list(idx)%np  = data%np
          this%data_list(idx)%nt  = data%nt
          
       End If
       return
       
    End If
    
    Allocate( ids_ptr(1:data%nt,1:ID_COL_MAX) )
    ids_offset = d_size
    pts_offset = ids_offset + Ubound(ids_ptr,2)* data%nt *sizeof(ids_ptr(1,1))
    
    

    ids_ptr(:,:) = Reshape ( Transfer (buf(ids_offset+1:pts_offset),ids_ptr(:,:) , & 
                                                           -ids_offset+pts_offset) , (/ data%nt,ID_COL_MAX /) ) 

    Allocate( pts_ptr(1:data%np,1:data%nd) )
    
    pts_ptr(:,:) = Reshape ( Transfer(buf(pts_offset+1:buf_size) , pts_ptr(:,:),-pts_offset+buf_size)  , (/ data%np,data%nd /) ) 

    d2 = data_find_by_name_dbg(this,data%name,"remote_access_class.F90",2467)
    If ( d2%id /= DATA_INVALID_ID) Then 
       idx = d2%id
       this%data_list(idx)%pts =  new_pts(ids_ptr,pts_ptr)
       this%data_list(idx)%nd  = data%nd
       this%data_list(idx)%np  = data%np
       this%data_list(idx)%nt  = data%nt

    Else
          
    End If
  End Subroutine   rma_unpack_data_pts




!!$------------------------------------------------------------------------------------------------------------------

  Subroutine schd_check_sts_all(this,notified_from,event)

    Type(remote_access) , Intent(inout) :: this
    Integer		, Intent(inout) :: event
    Integer		, Intent(in   ) :: notified_from

    Call check_scheduler(this,event)

  End Subroutine schd_check_sts_all
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine schd_data_dependences(this,axs,ver)

    Type(remote_access) , Intent(inout) :: this
    Type(data_access)   , Intent(inout) :: axs
    Integer 		, Intent(  out) :: ver
    Type(data_handle)                	:: Data
    Integer 				:: max_v,idx

    
    
    ver = 0 
    
    data=axs%data
    If (  Data%id /= DATA_INVALID_ID .And. data%name /= "") Then 
       
       max_v = Max(data%sv_r,data%sv_w,data%sv_m)
       
       If (( mod(axs%access_types,AXS_TYPE_READ*10)/AXS_TYPE_READ ) /= 0&
# 2521
                                                   ) Then 
          ver = data%sv_w
          data%sv_r = max_v +1
       End If
       If (( mod(axs%access_types,AXS_TYPE_WRITE*10)/AXS_TYPE_WRITE ) /= 0&
# 2525
                                                    ) Then 
          ver = max_v 
          data%sv_r = max_v+1
          data%sv_w = max_v+1
       End If
       If (( mod(axs%access_types,AXS_TYPE_MODIFY*10)/AXS_TYPE_MODIFY ) /= 0&
# 2530
                                                     ) Then 
          ver = data%sv_r
          data%sv_w = max_v+1
       End If
       idx=data%id
       this%data_list(idx) = Data 
       
    End If
   
    

  End Subroutine schd_data_dependences
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine schd_update_dependences(this)

   Type(remote_access)	:: this
   Integer 		:: i,j,ver,event = EVENT_DONT_CARE

   
 
   Do i= Lbound(this%task_list,1),Ubound(this%task_list,1)
      If (this%task_list(i)%status == TASK_STS_INITIALIZED) Then 
         Do j = Lbound(this%task_list(i)%axs_list,1),Ubound(this%task_list(i)%axs_list,1)
            If (Associated(this%task_list(i)%axs_list(j)%data) )Then 
               Call schd_data_dependences(this,this%task_list(i)%axs_list(j),ver)
               this%task_list(i)%axs_list(j)%min_ver = ver
            End If
         End Do
      End If
   End Do


   
    
  End Subroutine schd_update_dependences
!!$------------------------------------------------------------------------------------------------------------------
  Function to_str(no) Result(str)
        Integer :: no
        Character(len=2)::str
        Write (str,"(I2.2)") no
  End Function to_str


End Module remote_access_class

