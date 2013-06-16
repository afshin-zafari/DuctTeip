#define IS_NZERO(a,b)  ( mod(a,b*10)/b ) /= 0 
#include "debug.h" 

Module dist_task_class

  Use fp
  Use mpi
  Use dist_const
  Use dist_types
  Use dist_common
  use dist_externs
  Use op_ass_class
  Use options_class
  Use dist_data_class
  Use dist_lsnr_class
  Use class_point_set
!  use dist_cholesky
  Implicit None

!!$  ------------------------------------------------------------------------------------------------------------
!!$  task_add_to_list       Adds a new Task object to the list of tasks
!!$  task_can_run           Checks whether a Task can be run or not.
!!$  task_check_sts         Checks the new status of a Task regarding the events and/or current status
!!$  task_check_sts_all     Loops for all tasks to check their status.
!!$  task_clean             Destroys the Task object.
!!$  task_data_recv         Checks all tasks when a Data is received.
!!$  task_find_by_id        Returns the Task object whose ID is as given.
!!$  task_find_by_sts       Returns the Task object whose status is as given.
!!$  task_find_in_list      Gets a Task object and find its corresponding entry in the Tasks list.
!!$  task_match_access      Matches the Data Access types of a task with the current versions of the data.
!!$  task_rbf_mat_purempi   runs the RBF mat when all the cores are communicating using MPI only.
!!$  task_rbf_mat           Creates RBF_TASK for task library and executes it.
!!$  task_run               Executes a Task.
!!$  task_chol_local        In Cholesky algorithm, when a global task is received this function prepares appropriate 
!!$                         local sub-tasks for D-TEiP.
!!$  task_send              Sends a Task object.
!!$  task_sample            The Sample Task executable.
!!$  task_upgrade_data      Updates the Data of a task, called after execution.
!!$  task_write_name        Gets the name of the Data object that the given Task will write to.
!!$  ------------------------------------------------------------------------------------------------------------


Contains 
!!$  ------------------------------------------------------------------------------------------------------------
  Subroutine task_add_to_list(this,task)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info) 	, Intent(inout) :: task   
    Integer 				:: i

    Do i = Lbound(this%task_list,1), Ubound(this%task_list,1)
       If ( this%task_list(i)%status == TASK_STS_CLEANED ) Then 
          Exit
       End If
    End Do

    If ( i <= Ubound(this%task_list,1) ) Then 
       this%task_list(i)        = task 
       this%task_list(i)%id     = i
       task%id = i
       this%task_list(i)%status = TASK_STS_INITIALIZED
    Else
    End If
    VIZIT4(EVENT_TASK_ADDED,i,0,0)
  
    

  End Subroutine task_add_to_list
!!$  ------------------------------------------------------------------------------------------------------------

  Subroutine task_can_run(this,task,event,answer)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info) 	, Intent(inout) :: task   
    Integer		, Intent(inout) :: event
    Logical 		, Intent(  out)	:: answer
    Character(len=MAX_TASK_NAME)	:: dname
    Integer 				:: i
    

    task%status = TASK_STS_READY_TO_RUN
!!$    If (task%name(1:len(TNAME_INIT_DATA)) == TNAME_INIT_DATA ) Then 
!!$       answer = .True.
!!$       Call ll_add_item ( this%tlist_ready_sts,task%id,0)
!!$       Return
!!$    End If
    Do i=Lbound(task%axs_list,1) , Ubound(task%axs_list,1)
       If ( task%axs_list(i)%dname  == ""  .or. task%axs_list(i)%dproc_id <0) Cycle 
       If ( task%type /= SYNC_TYPE_NONE) Cycle
       Call task_match_access(this,task%id,i,answer)
       If (.Not. answer) Then 
          task%status = TASK_STS_WAIT_FOR_DATA
       End If
    End Do
    answer = task%status == TASK_STS_READY_TO_RUN
    If ( answer ) Then 
       Call ll_add_item ( this%tlist_ready_sts,task%id,0)
    Else
    End If

    VIZIT4(EVENT_TASK_CHECKED,task%id,0,0)
    

    

  End Subroutine task_can_run
!!$---------------------------------------------------------------------------------------------------------------------
!!$   This routine, checks the current status of the input Listener  and the last event happened in the program. Then it 
!!$   decides on what will be the next status and whom has to be notified accordingly. The procedure can be 
!!$   summarized as following table:
!!$   
!!$       Event        Condition            From Status    To Status         Notify
!!$   --------------   ----------           -----------    ---------         ---------
!!$    Task RCVD        --                   CLEANED        INIT             Data,Task
!!$     --             is remote             INIT          SEND INIT         MailBox    
!!$     --             is Not remote         INIT          RUN READY         Scheduler    
!!$                    and Can Run           
!!$     --             is Not remote         INIT          DATA WAIT         --
!!$                    and Cannot Run        
!!$    Data RCVD       Can Run               DATA WAIT     RUN READY         Scheduler    
!!$    Data RCVD       Cannot Run            DATA WAIT     DATA WAIT         --
!!$    Task Started     --                   RUN READY     SCHEDULED         --
!!$    Task Running     --                   SCHEDULED     RUNNING           --
!!$    Task Finished    --                   RUNNING       FINISHED          Data,Listener
!!$     --              --                   FINISHED      CLEANED           --
!!$    Task ACK         --                   NOT ACKED     ACKED             --
!!$     --              --                   ACKED         CLEANED           --
!!$---------------------------------------------------------------------------------------------------------------------

  Subroutine task_check_sts(this,task,event,notify)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info) 	, Intent(inout) :: task   
    Integer		, Intent(inout) :: event,notify
    Integer 				:: from_sts,to_sts
    Logical 				:: answer

    from_sts = task%status
    to_sts = from_sts
    notify = NOTIFY_DONT_CARE

    Select Case(from_sts)
      Case  (TASK_STS_CLEANED)
         If (event == EVENT_TASK_RECEIVED) Then 
            Call task_add_to_list(this,task)
            notify = NOTIFY_OBJ_DATA + NOTIFY_OBJ_TASK
         End If
         to_sts = TASK_STS_INITIALIZED
      Case  (TASK_STS_INITIALIZED)
         If ( task%proc_id /= this%proc_id ) Then  ! task is remote
            to_sts = COMM_STS_SEND_INIT
            Call ll_add_item ( this%send_list,task%id,TASK_OBJ)
            notify = NOTIFY_OBJ_MAILBOX
         Else
            Call task_can_run(this,task,event,answer)
            If ( answer ) Then 
               notify = NOTIFY_OBJ_SCHEDULER
               to_sts = TASK_STS_READY_TO_RUN
            Else
               to_sts = TASK_STS_WAIT_FOR_DATA
            End If
         End If
      Case  (TASK_STS_WAIT_FOR_DATA)
         If ( event == EVENT_DATA_RECEIVED .Or. .True. ) Then 
            Call task_can_run(this,task,event,answer)
            If ( answer ) Then 
               to_sts = TASK_STS_READY_TO_RUN
               notify = NOTIFY_OBJ_SCHEDULER
            Else
               to_sts = TASK_STS_WAIT_FOR_DATA
            End If
         End If         
      Case  (TASK_STS_READY_TO_RUN)
         If ( event == EVENT_TASK_STARTED  ) Then 
            to_sts =  TASK_STS_SCHEDULED
         End If
      Case (TASK_STS_SCHEDULED) 
         If ( event == EVENT_TASK_RUNNING) Then 
            to_sts  = TASK_STS_INPROGRESS
         End If
      Case  (TASK_STS_INPROGRESS)         
         If ( Associated(task%dmngr) ) Then 
            If ( event == EVENT_TASK_FINISHED .Or. dm_get_sync(task%dmngr)> 0  ) Then 
               to_sts  = TASK_STS_FINISHED
               call instrument(EVENT_TASK_FINISHED,task%id,task%task_idx,0)
               TIMING(EVENT_TASK_FINISHED,task%id,task%name,task%weight)
               notify = NOTIFY_OBJ_DATA + NOTIFY_OBJ_LISTENER
               Call task_upgrade_data(this,task)
            End If
         Else
         End If
      Case  (TASK_STS_FINISHED)
         to_sts = TASK_STS_CLEANED
         Call task_clean(this,task,event) 
      Case (COMM_STS_SEND_PROGRESS)
         If ( event == EVENT_TASK_ACK) Then
            to_sts = COMM_STS_SEND_COMPLETE
            Call task_clean(this,task,event) 
         End If
      Case (COMM_STS_SEND_COMPLETE)
         to_sts = TASK_STS_CLEANED
         Call task_clean(this,task,event) 
     Case default
    End Select

    task%status = to_sts
    this%notify = notify

    If (to_sts /= from_sts .Or. notify /= NOTIFY_DONT_CARE) Then 
       If (task%id /= TASK_INVALID_ID) Then 
          VIZIT4(EVENT_TASK_STS_CHANGED,task%id,0,0)
       End If
    End If
    If (task%status == TASK_STS_FINISHED) Then 
       Call print_lists(this)
    End If
     
  End Subroutine  task_check_sts
!!$  ------------------------------------------------------------------------------------------------------------

  Subroutine task_check_sts_all(this,notified_from,event)

    Type(remote_access) , Intent(inout) :: this
    Integer		, Intent(inout) :: event
    Integer		, Intent(in   ) :: notified_from
    Integer 				:: i ,notify,sts

    Do i = Lbound(this%task_list,1) , Ubound(this%task_list,1)
       If (this%task_list(i)%id /= TASK_INVALID_ID)  Then 
          Call task_check_sts(this,this%task_list(i),event,notify)
       End If
    End Do

  End Subroutine task_check_sts_all
!!$  ------------------------------------------------------------------------------------------------------------

  Subroutine task_clean(this,task,event)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info) 	, Intent(inout) :: task   
    Integer		, Intent(inout) :: event
    Integer                             :: SaveResults

    SaveResults = opt_get_option(this%opt,OPT_SAVE_RESULTS)
    VIZIT4(EVENT_TASK_CLEANED,task%id,0,0)
    task%status = TASK_STS_CLEANED
    task%id     = TASK_INVALID_ID

    if ( SaveResults == 0 ) then 
       task%name   = ""
       if ( associated(task%dmngr)) then 
          deallocate(task%dmngr)
          task%dmngr => null()
       end if
    end if
    

  End Subroutine task_clean
!!$  ------------------------------------------------------------------------------------------------------------

  Subroutine task_data_recv(this,Data) 

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle  ) , Intent(inout) :: Data
    Integer 				:: i ,event,notify

    event = EVENT_DATA_RECEIVED
    Do i = Lbound(this%task_list,1) , Ubound(this%task_list,1)
       If (this%task_list(i)%id /= TASK_INVALID_ID)  Then 
          Call task_check_sts(this,this%task_list(i),event,notify)
       End If
    End Do

  End Subroutine task_data_recv
!!$  ------------------------------------------------------------------------------------------------------------

  Function task_find_by_id(this,tid) Result(task) 

    Type(remote_access),Intent(inout) :: this
    Type(task_info)                   :: task
    Integer                           :: i,tid

    task%id = DATA_INVALID_ID
    
    Do i = Lbound(this%task_list,1),Ubound(this%task_list,1)
       If ( this%task_list(i)%id == tid) Then 
          task = this%task_list(i)
          Return
       End If
    End Do

  End Function task_find_by_id
!!$  ------------------------------------------------------------------------------------------------------------

  Function task_find_by_sts(this,sts) Result (task)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in   ) :: sts
    Type(task_info)                     :: task
    Integer 				:: i 
    Type(link_list_node) , Pointer :: tnode

    

    task%id = TASK_INVALID_ID

    Do i= Lbound(this%task_list,1),Ubound(this%task_list,1)
       If (this%task_list(i)%status == sts ) Then 
          task = this%task_list(i)
          Return
       End If
    End Do

    

  End Function task_find_by_sts
!!$  ------------------------------------------------------------------------------------------------------------

  Function task_find_in_list(this,in_task) Result(out_task)

    Type(remote_access),Intent(inout) :: this
    Type(task_info),Intent(in)        :: in_task
    Type(task_info)                   :: out_task
    Integer 			      :: i 
    
    Do i = Lbound(this%task_list,1), Ubound(this%task_list,1)
       If (this%task_list(i)%id == in_task%id) Then 
          out_task = this%task_list(i)
          Return
       End If
    End Do

  End Function task_find_in_list

!!$  ------------------------------------------------------------------------------------------------------------
  Function task_get_read_pts(this,task,idx) Result ( pts)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info)     , Intent(inout) :: task
    Integer             , Intent(inout) :: idx
    Type(point_set)                     :: pts
    Type(data_handle)                   :: Data
    Integer                             ::  i 

    Do i = Lbound(task%axs_list,1),Ubound(task%axs_list,1)
       If ( ( mod(task%axs_list(i)%access_types,AXS_TYPE_READ*10)/AXS_TYPE_READ ) /= 0 ) Then 
          If ( idx == 1 ) Then 
             data = task%axs_list(i)%data
             pts = Transfer (  this%data_list(data%id)%pts , pts ) 
             
             Return
          Else
             idx  = idx - 1
          End If
       End If
    End Do
    
  End Function task_get_read_pts
!!$  ------------------------------------------------------------------------------------------------------------
!!$  Cases that may happen:
!!$    A. The Data is already in the List 
!!$       1. Is it local ?
!!$          1-1. Listener already defined for it?
!!$               No.Create a Listener and add it to the Listeners list.
!!$               Return .False.
!!$       2. 
!!$         2-1. The access is READ and the Data is READ OK 
!!$            .Or. 
!!$         2-2. The Data is local and access is WRITE
!!$            .Or. 
!!$         2-3. The Data is MODIFY OK and access is READ
!!$            .And. 
!!$         2-4. Versions match,
!!$             Set match_status to READ OK 
!!$             Return  .True.
!!$    B. The Data is new, then it has to be created and added to the Data list. 
!!$       Return .False.
!!$  ------------------------------------------------------------------------------------------------------------

  Subroutine task_match_access(this, task_id,axs_id, answer)

    Type(remote_access) , Intent(inout) :: this
    integer             , Intent(in   ) :: task_id,axs_id
    Logical             , Intent(  out) :: answer 
    Type(data_handle)   , Pointer       :: Data
    Type(data_handle)                   :: d1
    Type(listener)		        :: lsnr

    

    answer = .False.

#define axs this%task_list(task_id)%axs_list(axs_id)

    If ( Associated(axs%data) ) Then 
       data => this%data_list(axs%data%id)
       If (axs%data%proc_id /= this%proc_id) Then           
          if ( data%has_lsnr == 0 ) then              
             lsnr%req_data=>data
             lsnr%proc_id=axs%data%proc_id
             lsnr%dname = data%name
             lsnr%lsnr_min_ver=axs%min_ver
             data%has_lsnr = 1
             
             Call lsnr_add_to_list(this,lsnr)           
             Return
          End If
       End If
       If ( (( data%status == DATA_STS_READ_READY .or. data%status == DATA_STS_INITIALIZED)  .And. &
               IS_NZERO (axs%access_types,AXS_TYPE_READ) ) &
            .Or. &
            (data%proc_id == this%proc_id .And. & 
               IS_NZERO( axs%access_types,AXS_TYPE_WRITE)  ) &
             .Or. &
             (data%status == DATA_STS_MODIFY_READY   .And. & 
               IS_NZERO( axs%access_types,AXS_TYPE_READ) ) &
             ) Then 
          If (data%version >= axs%min_ver) Then 
             answer = .True.
             axs%match_status = DATA_STS_READ_READY
          End If
       End If
    Else
       d1 = data_find_by_name_dbg(this,axs%dname,"",0)
       If ( d1%id == DATA_INVALID_ID ) Then 
          if (axs%dname =="") return 
          data=>data_create(this,axs%dname,& 
                            this%np,this%nd,&
                            axs%min_ver, &
                            axs%dproc_id)          
          d1 = data_find_by_name_dbg(this,this%task_list(task_id)%axs_list(axs_id)%dname,"",0)
       End If
       Data=>this%data_list(d1%id)
       axs%data => data
       If (axs%dproc_id /= this%proc_id) Then 
          If ( data%has_lsnr == 0 ) Then 
             lsnr%req_data=>data
             lsnr%proc_id=data%proc_id
             data%has_lsnr = 1
             lsnr%lsnr_min_ver = axs%min_ver
             Call lsnr_add_to_list(this,lsnr)             
          End If
       End If
    End If
 
#undef axs    
    

  End Subroutine task_match_access
!!$-------------------------------------------------------------------------------------------------------------------
  Subroutine task_rbf_mat_purempi(this , taskid)
    Type(remote_access) , Intent(inout)          :: this
    Integer             , Dimension(1:2)         :: rg1,rg2, sp1,sp2 	
    Real(kind=rfp)      , Dimension(:,:),Pointer :: dist , p1_ptr,p2_ptr,rbfmat
    Type(task_info)                              :: task
    Integer                                      :: taskid,pno,i,j,nd,event,notify
    Type(point_set)                              :: p1,p2
    Character(len=80)			         :: phi
    Character				         :: nprime
    Real(kind=rfp)			         :: eps

    task=this%task_list(taskid)
    TIMING(EVENT_TASK_STARTED,task%id,0,0)
    Allocate(this%task_list(task%id)%dmngr)
    Call dm_new(this%task_list(task%id)%dmngr)
    Call dm_set_taskid(this%task_list(task%id)%dmngr,task%id)

    pno = 1;p1 = task_get_read_pts(this,task,pno)
    pno = 2;p2 = task_get_read_pts(this,task,pno)

    sp1 = span_pts_id(p1)
    sp2 = span_pts_id(p2)
    

    p1_ptr => get_pts_ptr( p1);
    p2_ptr => get_pts_ptr( p2);

    Allocate ( dist (1:Size(p1_ptr,1) ,1:Size(p2_ptr,1)) ) 
    

    Do i=1,Ubound(dist,1)
       Do j=1,Ubound(dist,2)
          dist(i,j) = Sqrt((p2_ptr(j,1)-p1_ptr(i,1))**2+(p2_ptr(j,2)-p1_ptr(i,2))**2)
       End Do
    End Do

    Allocate ( rbfmat ( 1:Size(dist,1) , 1:Size(dist,2) ) )  
    

    eps = 2.0
    phi="gauss"
    nprime='0'
    nd = 2
    
    
    
    
    Call dphi(phi, nprime, nd, eps, dist,rbfmat)
    
    Deallocate(dist)
    Deallocate(rbfmat)
    event = EVENT_TASK_RUNNING
    Call task_check_sts(this,this%task_list(task%id),event,notify)
    Call dm_set_sync(this%task_list(task%id)%dmngr,1)
    event = EVENT_TASK_FINISHED
    Call task_check_sts(this,this%task_list(task%id),event,notify)
    

    TIMING(EVENT_TASK_FINISHED,task%id,0,0)

  End Subroutine task_rbf_mat_purempi
!!$  ------------------------------------------------------------------------------------------------------------
  Subroutine task_rbf_mat(this,taskid)

    Type(remote_access),Intent(inout) :: this
    Type(op_ass)       , Pointer      :: op_asm ,  op_asm_wf
    Type(task_info)                   :: task
    Integer                           :: event,taskid,notify,nt,pno,idx,nd,np1,np2
    Type(point_set)                   :: pts,p1,p2
    Logical                           :: success
    Character(len=4)                  :: tname

    

    task = this%task_list(taskid)
    
    
    pno = 1;p1 = task_get_read_pts(this,task,pno)
    pno = 2;p2 = task_get_read_pts(this,task,pno)

    call pts_get_info(p1,np1,nd,nt)
    call pts_get_info(p1,np2,nd,nt)
    nt = range_pts_id(p1) + range_pts_id(p2)
    

    pts = new_pts(np1+np2,this%nd,nt)

    success= add_pts (pts,p1)
    success= add_pts (pts,p2)

    nt = opt_get_option(this%opt,OPT_BLOCK_CNT)
    call pts_set_nt(p1,nt)
    nt = opt_get_option(this%opt,OPT_BLOCK_CNT2)
    call pts_set_nt(p2,nt)
    op_asm_wf => wfm_get_op_asm(this%wf,1)
    op_asm =>  op_asm_new_pts ( op_asm_wf,p1,p2)
    Allocate(this%task_list(task%id)%dmngr)
    Call dm_new(this%task_list(task%id)%dmngr)
    Call dm_set_taskid(this%task_list(task%id)%dmngr,task%id)
    tname = this%task_list(task%id)%axs_list(1)%data%name(1:2)//this%task_list(task%id)%axs_list(2)%data%name(1:2)
    Call op_ass_task_dist(op_asm,this%task_list(task%id)%dmngr,tname,task%id,this%np)

    TIMING(EVENT_TASK_STARTED,task%id,0,0)
    VIZIT4(EVENT_TASK_STARTED,task%id,this%task_cnt,0)
    VIZIT4(EVENT_TASK_RUNNING,task%id,this%task_cnt,0)
    this%task_list(task%id)%task_idx = this%task_cnt
    this%task_cnt = this%task_cnt +1
    event = EVENT_TASK_RUNNING
    Call task_check_sts(this,this%task_list(task%id),event,notify)

    

  End Subroutine task_rbf_mat
!!$------------------------------------------------------------------------------------------------------------

  Subroutine task_run(this,task) 

    Type(remote_access),Intent(inout) :: this
    Type(task_info)                   :: task
    Type(point_set)                   :: pts
    Type(data_handle)                 :: Data
    Integer                           :: event,notify,purempi,tw
    Integer            ,Dimension(1:2)                            :: rg
    Character(len=MAX_DATA_NAME)  :: dname


    
    event = EVENT_TASK_STARTED
    Call task_check_sts(this,task,event,notify)
    this%task_list(task%id) = task
    event = EVENT_TASK_RUNNING
    Call task_check_sts(this,task,event,notify)
    this%task_list(task%id) = task

    call extern_run_task(this,task)

  End Subroutine task_run
!!$------------------------------------------------------------------------------------------------------------

  Subroutine task_send(this, idx)

    Type(remote_access) , Intent(inout)    :: this
    Integer             , Intent(in)       :: idx
    Type(task_info)                        :: task
    Integer 				   :: err,tag

#ifdef MT_LOG
    Integer(8) :: log_start, log_stop
#endif

    If (idx <= Ubound(this%task_req,1)) Then 
       If (this%task_list(idx)%type == SYNC_TYPE_NONE) Then

#ifdef MT_LOG
          call tl_log_time(log_start)
#endif
          Call  MPI_ISSEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
               this%task_list(idx)%proc_id,MPI_TAG_TASK,this%mpi_comm,this%task_req(idx),err)
#ifdef MT_LOG
          call tl_log_time(log_stop)
          call tl_log_add("MPI_ISEND task"//CHAR(0), log_start, log_stop)
#endif
       Else
#ifdef MT_LOG
          call tl_log_time(log_start)
#endif
          Call MPI_ISSEND (this%task_list(idx),sizeof(task),MPI_BYTE, &
               this%task_list(idx)%proc_id,this%task_list(idx)%type   ,this%mpi_comm,this%task_req(idx),err) 

#ifdef MT_LOG
          call tl_log_time(log_stop)
          call tl_log_add("MPI_ISSEND task"//CHAR(0), log_start, log_stop)
#endif
       End If
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       
    End If

    

  End Subroutine task_send
!!$------------------------------------------------------------------------------------------------------------

  Subroutine task_sample(this,taskid)

    Type(remote_access),Intent(inout) :: this
    Type(task_info)                   :: task
    Integer                           :: event,taskid,notify

    task = this%task_list(taskid)
    event = EVENT_TASK_RUNNING
    Call task_check_sts(this,task,event,notify)
    this%task_list(task%id) = task


    event = EVENT_TASK_FINISHED
    Call task_check_sts(this,task,event,notify)
    this%task_list(task%id) = task

  End Subroutine task_sample
!!$------------------------------------------------------------------------------------------------------------

  Subroutine task_upgrade_data(this,task)

    Type(remote_access) , Intent(inout) :: this
    Type(task_info)     , Intent(inout) :: task
    Type(data_handle) 			:: Data
    Integer 				:: i

    Do i=Lbound(task%axs_list,1) , Ubound(task%axs_list,1)
       If ( .Not. Associated (task%axs_list(i)%data) )  Cycle
       If (task%axs_list(i)%data%id /= DATA_INVALID_ID) Then 
          Data = data_find_by_name_dbg(this,task%axs_list(i)%data%name,__FILE__,__LINE__)
          If (data%id /= DATA_INVALID_ID) Then 
             If (task%axs_list(i)%access_types /= AXS_TYPE_READ ) Then 
!                If ( task%name /= TNAME_INIT_DATA ) Then 
                   this%data_list(data%id)%version = this%data_list(data%id)%version+1
!                end if 

                Call print_data (this,data%id,task%name)
                this%data_list(data%id)%status = DATA_STS_READ_READY
                Data = this%data_list(data%id)
                Call lsnr_wakeup(this,Data)
             End If
          End If
       End If
    End Do
    

  End Subroutine task_upgrade_data 
!!$------------------------------------------------------------------------------------------------------------
!!$------------------------------------------------------------------------------------------------------------

End Module dist_task_class
