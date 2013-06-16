#include "debug.h"

module dist_dlb_idle

  use fp
  use mpi 
  use wfm_class 
  use dist_const
  use dist_types
  use dist_common
  use dist_data_class
  use dist_task_class
  use dist_lsnr_class
  use class_point_set 
  use options_class
  use remote_access_class


  integer ,parameter  ::MAX_STS=10
  integer ,parameter  ::MPI_STS_SIZEIDX=2
  integer ,parameter  ::MPI_TAG_IDLE_SEND  = MPI_TAG_START + 4
  integer ,parameter  ::MPI_TAG_IDLE_REPLY = MPI_TAG_START + 5

  integer , parameter  ::TPL_TYPE_RANDOM    = 1
  integer , parameter  ::TPL_TYPE_GROUP     = 2
  integer , parameter  ::TPL_TYPE_HIERARCHY = 4
  integer , parameter  ::TPL_TYPE_CIRCULAR  = 8
  integer , parameter  ::TPL_TYPE_PROPAGATE = 16

  type topology_level
     integer :: thr,tpl_type,nodes_cnt
     integer , dimension(:),pointer :: node_list
  end type topology_level

  type topology
     integer :: tpl_type,level_cnt,nodes_cnt
     type(topology_level),dimension(:),pointer ::tpl_list     
  end type topology

  type packed_tasks
     integer :: t_cnt
     type(task_info),dimension(:),pointer::tlist
  end type packed_tasks

  type dlb_idle_type
     integer, dimension(:) , pointer :: mpi_req,node_id,amnt
     type(remote_access),pointer :: rma
     integer ::mpi_comm
     type(topology) :: tpl
  end type dlb_idle_type
  
  implicit none

contains

!!$------------------------------------------------------------------------------------------------------------------

  function current_task_load(this) result (load)

    type(remote_access) , intent(inout) :: this
    integer 				:: load,lbm,idx

    lbm = opt_get_option(this%opt,OPT_LOAD_BALANCE)
    load = 0 
    if ( lbm == LBM_NODES_GROUPING ) then 
       do idx = 1, ubound(this%task_list,1)
          if ( this%task_list(idx)%proc_id == this%proc_id  .and.  this%task_list(idx)%id  /= TASK_INVALID_ID  ) then 
             load = load + this%task_list(idx)%weight
          end if
       end do
    else
       load = count (this%task_list(:)%proc_id == this%proc_id     .and. &
                     this%task_list(:)%id      /= TASK_INVALID_ID          ) 
    end if
    TRACE2("LBM,Load is ",load) 
  end function current_task_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_steal_send(this)
    type(remote_access) , intent(inout) :: this
    integer                             :: idx
    type(task_info)                     :: task
    integer 				:: err,tag

    TRACE1("LBM,TASK steal SEND","ENTER")

    do idx=1,ubound(this%task_list,1)
       if (this%task_list(idx)%id /= TASK_INVALID_ID) then 
          exit
       end if
    end do

    if (idx <= ubound(this%task_req,1)) then 
       this%task_steal_dest = this%task_steal_dest+1
       if (this%task_steal_dest == this%proc_id) then 
          this%task_steal_dest = this%task_steal_dest+1
       end if
       if (this%task_steal_dest >= this%node_cnt) then 
          this%task_steal_dest = this%node_cnt
          return 
       end if
       TRACE2( "LBM,task idx,proc,comm,name",(idx,this%task_list(idx)%proc_id,this%mpi_comm,this%task_list(idx)%name) )
       this%task_list(idx)%id=0
       call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
       TRACE2("LBM,tag,err,req,size,id,pid,name after ISEND",(MPI_TAG_TASK,err,this%task_req(idx),&
            sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,this%task_list(idx)%name) )
       VIZIT3(EVENT_TASK_SEND_REQUESTED,idx,this%task_list(idx)%proc_id)
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       TRACE2( "LBM,task sts",(idx,task_sts_name(this%task_list(idx)%status) ) )
    end if

    TRACE1("LBM,TASK Steal SEND","EXIT") 
  end subroutine task_steal_send

!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_steal_reply(this,load,dest)
    type(remote_access) , intent(inout)  :: this
    integer 		, intent(in)	 :: load,dest
    integer                              :: idx,rj_idx,ac_idx,i,err
    type(task_info)                      :: task

    TRACE2("LBM,Task Steal Reply, load,dest",(load,dest) ) 
    do i=1,ubound(this%task_list,1)    ! ToDo: Strategy for exporting which task??
       if ( this%task_list(i)%id /= TASK_INVALID_ID) then 
          ac_idx=i
       else
          rj_idx=i
       end if
    end do

    if ( load >0 ) then 
       idx=rj_idx
       this%task_list(idx)%type=TTYPE_TASK_STEAL_REJECTED
       TRACE2("LBM, Rejected",idx)
    else
       idx=ac_idx
       TRACE2("LBM, Accepted",idx)
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
       call lsnr_redirect(this,this%task_list(idx)%id,dest)
    end if
    TRACE2("LBM, Send steal response ",idx)
    call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
         dest,MPI_TAG_TASK_STEAL_RESPONSE,this%mpi_comm,this%task_req(idx),err) 
    this%task_list(idx)%status = COMM_STS_SEND_PROGRESS

  end subroutine task_steal_reply

!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_export_reply(this,load,dest)

    type(remote_access) , intent(inout)   :: this
    integer 		, intent(in)	  :: load,dest
    integer                               :: idx,rj_idx,ac_idx,i,err
    type(task_info)                       :: task

    TRACE2("LBM, export reply, load,dest ",(load,dest))
    do i=1,ubound(this%task_list,1)    ! ToDo: Strategy for exporting which task??
       if ( this%task_list(i)%id /= TASK_INVALID_ID) then 
          ac_idx=i
       else
          rj_idx=i
       end if
    end do

    if ( load >0 ) then 
       idx=rj_idx
       TRACE2("LBM, export reply, rejected ",idx)
       this%task_list(idx)%type=TTYPE_TASK_STEAL_REJECTED
    else
       idx=ac_idx
       TRACE2("LBM, export reply, accepted ",idx)
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
    end if
    TRACEX("LBM, export reply, send response ",idx)
    call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
         dest,MPI_TAG_TASK_STEAL_RESPONSE,this%mpi_comm,this%task_req(idx),err) 
    this%task_list(idx)%status = COMM_STS_SEND_PROGRESS

  end subroutine task_export_reply

!!$------------------------------------------------------------------------------------------------------------------
  subroutine lsnr_redirect(this, task_id,new_owner)
    type(remote_access) , intent(inout) :: this
    integer 		, intent(in)	:: task_id,new_owner
    type(data_handle)                   :: data
    integer                             :: i,j
    character(len=MAX_DATA_NAME)        :: wrt_data_name

    TRACEX("LBM, lsnr redirect,task,owner ",(task_id,new_owner))
    do i=1,ubound(this%task_list,1)
       if (this%task_list(i)%id == task_id ) then 
          do j=1,ubound(this%task_list(i)%axs_list,1)
             if (this%task_list(i)%axs_list(j)%access_types /= AXS_TYPE_READ) then 
                wrt_data_name =  this%task_list(i)%axs_list(j)%data%name
             end if
          end do
       end if
    end do
    
    TRACEX("LBM, lsnr redirect,dname ",wrt_data_name)
    data=data_find_by_name(this,wrt_data_name)
    if (data%id == DATA_INVALID_ID) then 
       ! Todo: create data for future references
    end if
    data%status = DATA_STS_REDIRECTED
    data%proc_id = new_owner
    this%data_list(data%id)%status  = DATA_STS_REDIRECTED
    this%data_list(data%id)%proc_id = new_owner 

  end subroutine lsnr_redirect
!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_export(this)
    type(remote_access) , intent(inout)	:: this
    integer                             :: idx
    type(task_info)                     :: task
    integer 				:: err,tag

    TRACEX("LBM,TASK export","ENTER")

    do idx=1,ubound(this%task_list,1)
       if (this%task_list(idx)%id /= TASK_INVALID_ID .and.  this%task_list(idx)%status== TASK_STS_INITIALIZED) then 
          exit
       end if
    end do

    if (idx <= ubound(this%task_req,1)) then 
       this%task_steal_dest = this%task_steal_dest+1
       if (this%task_steal_dest == this%proc_id) then 
          this%task_steal_dest = this%task_steal_dest+1
       end if
       if (this%task_steal_dest >= this%node_cnt) then 
          this%task_steal_dest = this%node_cnt
          return 
       end if
       TRACEX("LBM,TASK export,dest",this%task_steal_dest )
       TRACEX("LBM,task idx,proc,comm,name",(idx,this%task_list(idx)%proc_id,this%mpi_comm,this%task_list(idx)%name) )
       this%task_list(idx)%id=0
       call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
       TRACEX("LBM,tag,err,req,size,id,pid,name after ISEND",(MPI_TAG_TASK,err,this%task_req(idx),&
            sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,this%task_list(idx)%name) )
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       TRACEX( "LBM,task sts",(idx,task_sts_name(this%task_list(idx)%status) ) )
    end if
    TRACEX("LBM,TASK export","EXIT") 
  end subroutine task_export

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_task_export(this) 
    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)   		:: this
    logical 						:: flag
    integer 						:: err,req,idx,load
    type(task_info)                                     :: task


    TRACEX("LBM,CHECK TASK export","ENTER")

    do idx = 1, ubound(this%task_list,1)
       if ( this%task_list(idx)%id == TASK_INVALID_ID ) then 
          exit
       end if
    end do
    
    load=current_task_load(this)-this%node_capacity

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       TRACEX("LBM,task export received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name)) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)
       TRACEX("LBM,task export received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name) ) 
       TRACEX("LBM,task export received check its status src,tag,com,error,size,id,pid,req,name",(sts(MPI_SOURCE),MPI_TAG_TASK, & 
            this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name))
       call task_export_reply(this,load,sts(MPI_SOURCE))
    end if
    if (this%task_steal_dest <0 .and. load >0 ) then 
       call task_export(this)
    end if
    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       TRACEX("LBM,task export  received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name)) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)
       TRACEX("LBM,task steal received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name) ) 
       TRACEX("LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name",(sts(MPI_SOURCE),MPI_TAG_TASK, & 
            this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name))
       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_REJECTED) then 
             call task_export(this)
       end if
       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_ACCEPTED) then 
          this%task_list(this%task_list(idx)%id)%proc_id =  sts(MPI_SOURCE)
          call lsnr_redirect(this,this%task_list(idx)%id,sts(MPI_SOURCE))
       end if 
    end if
    TRACEX("LBM,CHECK TASK export","Exit")

  end subroutine check_task_export

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_task_steal(this) 
    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)   		:: this
    logical 						:: flag
    integer 						:: err,req,idx,load
    type(task_info)                                     :: task


    TRACEX("LBM,CHECK TASK STEAL","ENTER")

    do idx = 1, ubound(this%task_list,1)
       if ( this%task_list(idx)%id == TASK_INVALID_ID ) then 
          exit
       end if
    end do
    
    load=current_task_load(this)-this%node_capacity

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       TRACEX("LBM,task steal received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name)) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)
       TRACEX("LBM,task steal received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name) ) 
       TRACEX("LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name",(sts(MPI_SOURCE),MPI_TAG_TASK, & 
            this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name))
       call task_steal_reply(this,load,sts(MPI_SOURCE))
    end if
    TRACEX("LBM, dest ,load",(this%task_steal_dest,load  ))
    if (this%task_steal_dest <0 .and. load <0 ) then 
       call task_steal_send(this)
    end if
    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       TRACEX("LBM,task steal received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name)) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)
       TRACEX("LBM,task steal received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name) ) 
       TRACEX("LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name",(sts(MPI_SOURCE),MPI_TAG_TASK, & 
            this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name))
       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_REJECTED) then 
             call task_steal_send(this)
       end if
       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_ACCEPTED) then 
          this%task_list(idx)%type = SYNC_TYPE_NONE
          this%task_list(idx)%proc_id  = this%proc_id
          this%event=EVENT_TASK_RECEIVED
          call task_check_sts(this,this%task_list(idx),this%event,this%notify ) 
       end if 
    end if
    TRACEX("LBM,CHECK TASK STEAL","EXIT")

  end subroutine  check_task_steal

!!$------------------------------------------------------------------------------------------------------------------
  function rma_get_upper_node(this) result (node)
    type(remote_access),intent(inout) :: this
    integer                           :: node

    node = (this%node_cnt-1) / this%group_size
    TRACEX("LBM,Upper node",node)

  end function rma_get_upper_node

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_transfer_task(this, f,t,w)
    type(remote_access), intent(inout)  :: this
    integer            , intent(in)     :: f,t,w
    integer                             :: idx,err
    type(task_info)                     :: task

    TRACEX("LBM,Xfer task,from,to,wght",(f,t,w))

    do idx=1,ubound(this%task_list,1)
       if (this%task_list(idx)%id /= TASK_INVALID_ID) then 
          exit
       end if
    end do

    if (idx <= ubound(this%task_req,1)) then 
       this%task_list(idx)%id=w
       this%task_list(idx)%proc_id=t
       call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            f,MPI_TAG_TRANSFER_TASK,this%mpi_comm,this%task_req(idx),err) 
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
    end if
    
  end subroutine rma_transfer_task

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_local_balance(this)
    type(remote_access),intent(inout)   :: this
    integer                             :: wl_cnt,mx_wl,i,mx,mn,mxi,mni, from_host,to_host,wl

    mx = 0 ; mxi = 0 
    mn = 0 ; mni = 0

    TRACEX("LBM,local balance ","Enter")
    do while ( .true. )
       do i = 1, ubound(this%work_load_list,1)
          if (this%work_load_list(i,WL_NODE_ID) <0 ) exit
          if (this%work_load_list(i,WL_WEIGHT) >mx) then 
             mx = this%work_load_list(i,WL_WEIGHT)
             mxi = i 
          end if
          if (this%work_load_list(i,WL_WEIGHT) <mn) then 
             mn = this%work_load_list(i,WL_WEIGHT)
             mni = i 
          end if
       end do
       TRACEX("LBM,local balance min,max,minidx,maxidx",(mn,mx,mni,mxi))
       if (mx ==0 .or. mn == 0 ) exit
       from_host = this%work_load_list(mxi,WL_NODE_ID)
       to_host   = this%work_load_list(mni,WL_NODE_ID)
       wl = min(abs(mn),mx)
       call rma_transfer_task(this,from_host,to_host,wl)
       this%work_load_list(mxi,WL_WEIGHT) = this%work_load_list(mxi,WL_WEIGHT) - wl
       this%work_load_list(mni,WL_WEIGHT) = this%work_load_list(mni,WL_WEIGHT) + wl
    end do

    wl_cnt = count (this%work_load_list(:,WL_NODE_ID) > 0 ) 
    mx_wl = this%group_size
    if (this%proc_id == (this%node_cnt-1)/ this%group_size) then 
       mx_wl = mod(this%node_cnt-1,this%group_size)
    end if
    if ( wl_cnt >= mx_wl ) then 
       TRACEX("LBM,local balance send to upper node;cnt,max",(wl_cnt ,mx_wl))
       call rma_send_load(this)
    end if
  end subroutine rma_local_balance

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_export_load(this,w,t)
    type(remote_access) , intent(inout)   :: this
    integer             , intent(in)      :: w,t
    integer                               :: idx,mxwi,xw

    TRACEX("LBM,export load ;wght,to",(w ,t))
    xw=w
    do while ( .true.)
       mxwi=-1
       do idx = 1, ubound(this%task_list,1)
          if (this%task_list(idx)%id == TASK_INVALID_ID) exit
          if (this%task_list(idx)%weight <=xw ) then 
             if ( mxwi <0 ) then 
                mxwi = idx
             else
                if (this%task_list(idx)%weight >this%task_list(mxwi)%weight ) then 
                   mxwi = idx
                end if
             endif
          end if
       end do
       TRACEX("LBM,export load ;mxwi",mxwi)
       if (mxwi<0) exit
       this%task_list(mxwi)%proc_id=t
       call task_send( this,mxwi)
       call lsnr_redirect(this,this%task_list(mxwi)%id,t)
       this%task_list(mxwi)%type=TTYPE_TASK_STEAL_ACCEPTED
       xw = xw-this%task_list(mxwi)%weight
       TRACEX("LBM,export load ;new wght",xw)
    end do

  end subroutine rma_export_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_send_load(this)
    type(remote_access) , intent(inout)   :: this
    integer                               :: xload,upper_node,msg_size,req,err

    TRACEX("LBM,send load ","enter")
    xload = current_task_load(this) - this%node_capacity
    upper_node = rma_get_upper_node(this)
    msg_size= size(this%work_load_list) 
    TRACEX("LBM,send load, upper,extra loadm msg sz",(xload,upper_node,msg_size))

    if (.not. this%single_load_sent ) then  
       this%work_load_list(:,:)=0
       this%work_load_list(1,0)=this%proc_id
       this%work_load_list(1,1)=xload
    end if
    call  MPI_ISEND (this%work_load_list(1,1),msg_size,MPI_INTEGER, & 
         upper_node,MPI_TAG_WORK_LOAD,this%mpi_comm,req,err) 

    TRACEX("LBM,send load ","exit")

  end  subroutine rma_send_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_group_bal(this)

    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)                 :: this
    logical                                             :: flag
    integer                                             :: err,idx
    type(task_info)                                     :: task

    TRACEX("LBM,check group bal, single done?  ",this%single_load_sent )
    if (.not. this%single_load_sent ) then  
       call rma_send_load(this)
       this%single_load_sent= .true.
    end if

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_WORK_LOAD, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       TRACEX("LBM,check group bal, work load recvd,src ",sts(MPI_SOURCE) )
       call MPI_RECV ( this%work_load_list(1,1), size(this%work_load_list), MPI_INTEGER,&
            sts(MPI_SOURCE), MPI_TAG_WORK_LOAD, this%mpi_comm, sts, err)
       call rma_local_balance(this)
    end if

    do idx=1,ubound(this%task_list,1)
       if (this%task_list(idx)%id /= TASK_INVALID_ID) then 
          exit
       end if
    end do
    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TRANSFER_TASK, this%mpi_comm,flag,sts,err)

    if (flag .and. idx <= ubound(this%task_req,1)) then 
       TRACEX("LBM,check group bal, transfer task recvd,src ",sts(MPI_SOURCE) )
       call  MPI_IRECV (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            sts(MPI_SOURCE),MPI_TAG_TRANSFER_TASK,this%mpi_comm,this%task_req(idx),err) 
       call rma_export_load(this,this%task_list(idx)%id,this%task_list(idx)%proc_id)
    end if

  end subroutine check_group_bal

!!$------------------------------------------------------------------------------------------------------------------
  function dlb_idle_init(rma) result(this)
    type(remote_access) , intent(inout),pointer    :: rma
    type(dlb_idle_type) ,pointer :: this
    integer :: tpl_type, nodes_cnt,idx

!!$  type topology_level
!!$     integer :: thr,tpl_type,nodes_cnt
!!$     integer , dimension(:),pointer :: node_list
!!$  end type topology_level
!!$
!!$  type topology
!!$     integer :: tpl_type,level_cnt,nodes_cnt
!!$     type(topology_level),dimension(:),pointer ::tpl_list     
!!$  end type topology


    allocate ( this ) 
    this%rma=>rma
    tpl_type = opt_get_option(this%rma%opt,OPT_TPL_TYPE)
    if ( tpl_type == TPL_TYPE_CIRCULAR ) then 
       nodes_cnt = 2
       this%tpl%level_cnt = 1
       idx = 1
    else
       nodes_cnt          = opt_get_option(this%rma%opt,OPT_TPL_NODESCNT)
       this%tpl%level_cnt = opt_get_option(this%rma%opt,OPT_TPL_LEVELCNT)
    end if
    allocate ( this%tpl%tpl_list(1:this%tpl%level_cnt) )
    allocate ( this%tpl%tpl_list(idx)%node_list(1:nodes_cnt) )  
    !               Level#           Node#       Nodes
    ! Circular :      1                2         Left,Right 
    ! Random   :      1                n         rand(1,n)
    
  end function dlb_idle_init
!!$------------------------------------------------------------------------------------------------------------------
  function dlb_idle_check(this) result(answer)
    type(dlb_idle_type) :: this
    integer                                :: answer
    
    answer = 0 
    if ( .not. dlb_did_all_idle_replied(this) ) then 
       return 
    end if
    !TODO : if workload < idle_threshold
    call dbl_check_send_req(this)
    call dbl_check_send_reply(this)
    call dbl_check_recv_req(this)
    call dbl_check_recv_reply(this)

  end function dlb_idle_check

!!$------------------------------------------------------------------------------------------------------------------
  function dlb_idle_get_dests(this) result ( dest)
    type(dlb_idle_type) , intent(inout) :: this
    integer , dimension ( :) , pointer  :: dest
    integer , dimension ( :) , pointer :: nodes
    allocate ( nodes(1:10) )
! TODO : fill in the nodes by topology info
    select  case (this%tpl%tpl_type  ) 
       case (TPL_TYPE_RANDOM)
       case (TPL_TYPE_CIRCULAR) 
          
       
    end select
    dest => nodes
  end function dlb_idle_get_dests
!!$------------------------------------------------------------------------------------------------------------------

  subroutine dlb_send_idle_msg(this,nodes,amnt)
    type(dlb_idle_type) , intent(inout) :: this
    integer , dimension ( :) , pointer :: nodes
    integer , intent(in) :: amnt
! TODO : MPI_ISEND ....
! TODO : copy nodes,mpi_req .... to this%mpi_..

  end subroutine dlb_send_idle_msg
  
!!$------------------------------------------------------------------------------------------------------------------
  function dlb_did_all_idle_replied(this) result ( answer ) 
    type(dlb_idle_type) , intent(inout) :: this
    logical :: answer 
    logical :: flag
    integer :: buf_size,err
    integer, dimension ( MPI_STATUS_SIZE,1:MAX_STS) ::aSts

    answer = .true.
    if (.not. associated (this%mpi_req) ) then 
       return 
    end if
    call MPI_TESTALL(ubound(this%mpi_req,1),this%mpi_req,flag,aSts,err)
    answer = flag

  end function dlb_did_all_idle_replied

!!$------------------------------------------------------------------------------------------------------------------
  subroutine dlb_idle_check_replied(this) 
    type(dlb_idle_type) , intent(inout) :: this
    character(len=1), dimension ( :) , pointer:: buf
    logical :: flag
    integer :: buf_size,err,idx
    integer, dimension ( 1:MPI_STATUS_SIZE) ::sts

    if ( .not. associated (this%mpi_req) ) return

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_IDLE_REPLY, this%mpi_comm,flag,sts,err)

    if ( .not. flag ) return

    allocate ( buf(1:sts(MPI_STS_SIZEIDX)) ) 
    call MPI_RECV ( buf(1), ubound(buf,1), MPI_BYTE, sts(MPI_SOURCE), MPI_TAG_IDLE_REPLY, this%mpi_comm, sts, err)
    call dlb_idle_migrate_tasks(this,buf,sts(MPI_SOURCE))
    
  end subroutine dlb_idle_check_replied
!!$------------------------------------------------------------------------------------------------------------------

!!$------------------------------------------------------------------------------------------------------------------
  subroutine dlb_idle_migrate_tasks(this,buffer,from_node) 

    type(dlb_idle_type) , intent(inout) :: this
    character(len=1), dimension ( :) , pointer, intent(inout):: buffer
    integer , intent(in) :: from_node

    call dlb_idle_unpack_tasks(this,buffer,from_node) 

  end subroutine dlb_idle_migrate_tasks
!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_add_task_after(rma,task,idx)

    type(remote_access) , intent(inout) :: rma      
    type(task_info)     , intent(in)    :: task
    integer             , intent(inout) :: idx
    integer :: j

    do j=idx+1,ubound(rma%task_list,1)
       if (rma%task_list(j)%id == TASK_INVALID_ID ) cycle
       rma%task_list(j) = task
       idx=j
       exit
    end do
  end subroutine rma_add_task_after


!!$------------------------------------------------------------------------------------------------------------------
  subroutine dlb_idle_unpack_tasks(this,buf,from_node) 

    type(dlb_idle_type)                        , intent(inout) :: this
    character(len=1)    , dimension(:), pointer, intent(inout) :: buf
    integer                                    , intent(in)    :: from_node

    type(task_info)     :: task
    type(packed_tasks)  :: header
    integer             :: t_cnt,hdr_size,idx,fr=1
    
    hdr_size = sizeof(header)
    header =transfer(buf,header) 
    allocate ( header%tlist (1:header%t_cnt) ) 
    header%tlist = transfer ( buf(hdr_size+1:ubound(buf,1)),header%tlist,t_cnt*sizeof(task)-hdr_size)

    do idx=1,t_cnt
       task = header%tlist(idx)
       task%org_host = from_node
       call rma_add_task_after(this%rma,task,fr)
       if ( fr < 0 ) then 
          exit
       end if
    end do

    deallocate ( header%tlist ) 

  end subroutine dlb_idle_unpack_tasks
!!$------------------------------------------------------------------------------------------------------------------

!!$------------------------------------------------------------------------------------------------------------------
  subroutine dlb_busy_pack_tasks(this,tlist,buf) 

    type(dlb_idle_type) ,                         intent(inout) :: this
    integer             , dimension(:) , pointer, intent(in)    :: tlist
    character(len=1)    , dimension(:) , pointer, intent(inout) :: buf

    type(task_info)     :: task
    type(packed_tasks)  :: header
    integer             :: buf_size,t_cnt,hdr_size,t_size,idx,fr,to

    t_cnt = ubound(tlist,1)
    header%t_cnt = t_cnt
    hdr_size = sizeof(header)
    t_size = sizeof(task)
    allocate ( header%tlist (1:t_cnt) ) 
    buf_size =hdr_size +sizeof(task)*t_cnt
    allocate ( buf (buf_size) ) 
    buf(1:hdr_size)=transfer(header,buf,hdr_size)
    do idx = 1,t_cnt
       task = this%rma%task_list(tlist(idx))
       fr= hdr_size+1+(idx-1)*t_size+1
       to= fr + t_size
       buf(fr:to)= transfer(task,buf(fr:to), t_size)       
    end do
    

  end subroutine dlb_busy_pack_tasks
!!$------------------------------------------------------------------------------------------------------------------
  subroutine dbl_idle_check_recv(this)
    type(dlb_idle_type) ,                         intent(inout) :: this

    ! MPI_IPROBE()
    ! Is it sent from itself? Yes.=>  no positive response=> cancel Idle sending
    ! Consume locally
    ! Propagate Remaining IAND(tpl_type,TPL_TYPE_PROPAGATE)
    ! SenderLevel = get_level(sender_node_id)
    ! propage ( sender_Level,sender_node_id)

  end subroutine dbl_idle_check_recv


end module dist_dlb_idle
