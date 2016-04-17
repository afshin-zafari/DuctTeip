
#include "debug.h" 
#define  TIMING(a,b,c,d)


Module remote_access_class 

  use tl
  Use fp
  Use mpi 
  Use wfm_class 
  Use dist_const
  Use dist_types
  Use dist_common
  Use dist_data_class
  Use dist_task_class
  Use dist_lsnr_class
!  use dist_cholesky
!  use dist_mat_assemble
  use ductteip_lib
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
       VIZIT4(EVENT_TASK_ACK,idx,0,0)
       task_dst = opt_get_option(this%opt,OPT_TASK_DISTRIBUTION)
       If ( task_dst  == TASK_DIST_1BY1) Then 
!TODO : why is it so?           
!          Call rma_generate_single_task(this,this%task_list(idx)%proc_id,idx)
       End If
       this%event = EVENT_TASK_ACK
       Call rma_task_check_sts(this, this%task_list(idx),this%event)
    End If


    Call MPI_TESTANY(Ubound(this%lsnr_req,1),this%lsnr_req,idx,flag,sts,err)
# 128

    If ( flag .And. idx >0) Then 
       this%event = EVENT_LSNR_ACK
       this%lsnr_req(idx) = MPI_REQUEST_NULL
       
       VIZIT4(EVENT_LSNR_ACK,idx,0,0)
       Call lsnr_check_sts(this, this%lsnr_list(idx),this%event)
    End If


    Call MPI_TESTANY(Ubound(this%data_req,1),this%data_req,idx,flag,sts,err)
# 140

    If ( flag .And. idx >0) Then 
       this%data_req(idx) = MPI_REQUEST_NULL
       this%event = EVENT_DATA_ACK
       
       VIZIT4(EVENT_DATA_ACK,idx,0,0)
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
       TIMING(EVENT_DATA_RECEIVED,data_id,sts(MPI_SOURCE),this%data_buf_size)
       TRACEX("Data Received",(this%data_list(data_id)%name,this%data_list(data_id)%version,err))
       


       
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
        TRACEX("Receiving Lsnr failed. err",(err))
    end if 
     

    If ( flag ) Then 
       TRACEX("Is Lsnr Rcvd?",flag)
       Do idx = 1, Ubound(this%lsnr_list,1)
          If ( this%lsnr_list(idx)%id == LSNR_INVALID_ID ) Then 
             Exit
          End If
       End Do

       Call MPI_RECV ( this%lsnr_list(idx), sizeof(lsnr), MPI_BYTE, sts(MPI_SOURCE), MPI_TAG_LSNR, this%mpi_comm, sts, err)

       this%lsnr_list(idx)%status = LSNR_STS_CLEANED
       this%event	          = EVENT_LSNR_RECEIVED
       dname=this%lsnr_list(idx)%dname
       TRACEX("Lsnr from,v,dname ",(sts(MPI_SOURCE),this%lsnr_list(idx)%lsnr_min_ver,dname))
       Data = data_find_by_name_dbg(this,dname,"remote_access_class.F90",279)
       If ( data%id == DATA_INVALID_ID) Then 
          TRACEX("new data",dname)
          Data=data_create(this,dname,this%np,this%nd,this%lsnr_list(idx)%lsnr_min_ver,this%proc_id)
       End If
       this%lsnr_list(idx)%req_data=>this%data_list(data%id)
       
       VIZIT4(EVENT_LSNR_RECEIVED,idx,0,0)
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
          VIZIT4(EVENT_ALL_TASK_FINISHED,0,0,0)
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


    If ( flag ) Then 
       
       If ( rma_send_all_lsnr(this) ) Then

          Call MPI_RECV ( in_task(1), sizeof(task), MPI_BYTE, sts(MPI_SOURCE), &
               MPI_TAG_SYNC_LAST_TASK, this%mpi_comm, sts, err)

          this%sync_stage = SYNC_TYPE_LAST_TASK
          VIZIT4(EVENT_ALL_TASK_FINISHED,SYNC_TYPE_LAST_TASK,0,0)
       Else
          event = EVENT_DONT_CARE
          !VIZIT4(EVENT_ALL_TASK_FINISHED,-1,0,0)
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
          
          VIZIT4(EVENT_ALL_TASK_FINISHED,SYNC_TYPE_TERM_OK,0,0)
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
       VIZIT4(EVENT_TASK_RECEIVED,idx,0,0)
       TRACEX("Task Name",this%task_list(idx)%name)
       Call rma_task_check_sts(this,this%task_list(idx),this%event)
       !dname = task_write_name(this,idx)
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

    

    
    VIZIT4(EVENT_CYCLED,0,0,0)

  End Function rma_cycle_one_step
!!$------------------------------------------------------------------------------------------------------------------
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
    t_cnt = 2000
    d_cnt = 2000
    TRACEX("TaskCount",(t_cnt,inst_cnt,this%pc))
    

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

  subroutine rma_initialize(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt) 

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
    character(len=STR_MAX_LEN)::tasks_file


    call init_timing()
    rma = rma_make_this(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt)
    time_out= opt_get_option(rma%opt,OPT_TIME_OUT)
    lbm = opt_get_option (rma%opt,OPT_LOAD_BALANCE)
    seq = opt_get_option (rma%opt,OPT_SEQUENTIAL)


    call instrument2(EVENT_DTLIB_DIST,"DCTP",0,0)
    call extern_init(rma)
    call instrument2(EVENT_DTLIB_DIST,"DCTP",0,0)
    if ( seq /=0 ) then 
       call dump_timing()
       call extern_event_notify (rma,EVENT_ALL_TASK_FINISHED,0)
       return 
    end if
    rma%sync_stage = SYNC_TYPE_SENDING_TASKS

!    call schd_update_dependences(rma)
    call rma_send_all_tasks(rma) 
    call MPI_BARRIER(rma%mpi_comm,err)

    start=getime()/1e6

    cholesky=opt_get_option(rma%opt,OPT_CHOLESKY)
    call print_lists(rma,.true.)
    !call instrument2(EVENT_DTLIB_EXEC,"DCTP",1,0)
    do while(  rma_cycle_one_step(rma) /= EVENT_ALL_TASK_FINISHED )
    
       dnow = getime()/1e6
       if (  (dnow-start) > time_out ) then 
          write (*,*) " TBL Time Out ",dnow,start,(dnow-start) , time_out
          exit
       end if
       
	     
    end do
    call tl_add_task_named( "TEND" , dummy , i, 2 , 0,0, 0 ,0 , 0,0)

    !call instrument2(EVENT_DTLIB_EXEC,"DCTP",1,0)
    call dump_timing()
    call print_lists(rma,.true.)
    call extern_event_notify (rma,EVENT_ALL_TASK_FINISHED,0)
  end subroutine rma_initialize
!!$------------------------------------------------------------------------------------------------------------------
  subroutine dummy()
    integer ::i
    i=0
  end subroutine dummy
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
    rma%grp_rows = opt_get_option(rma%opt,OPT_CHOL_GRP_ROWS )
    rma%grp_cols = opt_get_option(rma%opt,OPT_CHOL_GRP_COLS )
    rma%dm_blk_cnt = opt_get_option ( rma%opt, OPT_DIST_BLK_CNT)
    rma%dm_blk_size = np 
    Call rma_init_this(rma)


  End Function rma_make_this
!!$------------------------------------------------------------------------------------------------------------------
  Function rma_send_all_lsnr(this) Result(all_sent)

    Type(remote_access),Intent(inout)	:: this
    Logical 				:: all_sent
    Integer 				:: remote_cnt,lsnr_cnt,i,task_cnt

    all_sent=.False.
    If ( .Not. Associated (ll_get_cur(this%send_list)) ) Then 


       task_cnt = Count ( this%task_list(:)%id /= TASK_INVALID_ID)
       
       If ( task_cnt /= 0 ) Return 
       
       lsnr_cnt = Count ( this%lsnr_list(:)%id /= LSNR_INVALID_ID  .And. &
                          (this%lsnr_list(:)%status == LSNR_STS_ACTIVE .or. &
                           this%lsnr_list(:)%status == COMM_STS_SEND_PROGRESS) ) 
       lsnr_cnt = lsnr_cnt + count (this%lsnr_req(:) /= MPI_REQUEST_NULL ) 
       
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
    Integer                            :: rmt_cnt , snt_cnt

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
                !Call rma_save_data(this)
                TRACEX("Data Clean All Called",(this%sync_stage,SYNC_TYPE_TERM_OK))
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
          TRACEX("unpacked ",(data%version,data%name) ) 
          this%data_list(idx)%mat(:,:)  = Reshape ( Transfer (buf(mat_offset:buf_size), &
                                                              0.0_rfp, &
                                                              (buf_size-mat_offset+1)) &
                                                    , (/ d2%nrows,d2%ncols /) ) 
          this%data_list(idx)%nd  = data%nd
          this%data_list(idx)%np  = data%np
          this%data_list(idx)%nt  = data%nt
          this%data_list(idx)%version = data%version
          
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


End Module remote_access_class

