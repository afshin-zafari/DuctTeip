# 1 "dist_dlb.F90"
# 1 "./debug.h" 1 



# 11











# 24




                          



# 36


# 42






# 52






# 62






# 72






# 82






# 92





# 2 "dist_dlb.F90" 2 

module dist_dlb

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
  use remote_access_class

  type dlb_idle_type
     integer, dimension(:) , pointer :: mpi_req,node_id,amnt
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
     
  end function current_task_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_steal_send(this)
    type(remote_access) , intent(inout) :: this
    integer                             :: idx
    type(task_info)                     :: task
    integer 				:: err,tag

    

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
       
       this%task_list(idx)%id=0
       call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
       

       call instrument(EVENT_TASK_SEND_REQUESTED,idx,this%task_list(idx)%proc_id,0)
       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       
    end if

     
  end subroutine task_steal_send

!!$------------------------------------------------------------------------------------------------------------------
  subroutine task_steal_reply(this,load,dest)
    type(remote_access) , intent(inout)  :: this
    integer 		, intent(in)	 :: load,dest
    integer                              :: idx,rj_idx,ac_idx,i,err
    type(task_info)                      :: task

     
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
       
    else
       idx=ac_idx
       
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
       call lsnr_redirect(this,this%task_list(idx)%id,dest)
    end if
    
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
    else
       idx=ac_idx
       
       this%task_list(idx)%type=TTYPE_TASK_STEAL_ACCEPTED
    end if
    write(*,*)  "LBM, export reply, send response "," : ",idx
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

    write(*,*)  "LBM, lsnr redirect,task,owner "," : ",(task_id,new_owner)
    do i=1,ubound(this%task_list,1)
       if (this%task_list(i)%id == task_id ) then 
          do j=1,ubound(this%task_list(i)%axs_list,1)
             if (this%task_list(i)%axs_list(j)%access_types /= AXS_TYPE_READ) then 
                wrt_data_name =  this%task_list(i)%axs_list(j)%data%name
             end if
          end do
       end if
    end do
    
    write(*,*)  "LBM, lsnr redirect,dname "," : ",wrt_data_name
    data=data_find_by_name_dbg(this,wrt_data_name,"dist_dlb.F90",171)
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

    write(*,*)  "LBM,TASK export"," : ","ENTER"

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
       write(*,*)  "LBM,TASK export,dest"," : ",this%task_steal_dest
       write(*,*)  "LBM,task idx,proc,comm,name"," : ",(idx,this%task_list(idx)%proc_id,this%mpi_comm,this%task_list(idx)%name)
       this%task_list(idx)%id=0
       call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
       write(*,*)  "LBM,tag,err,req,size,id,pid,name after ISEND"," : ",(MPI_TAG_TASK,err,this%task_req(idx),            sizeof(tas&
&k&
# 210
),this%task_list(idx)%id,this%task_list(idx)%proc_id,this%task_list(idx)%name)

       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       write(*,*)  "LBM,task sts"," : ",(idx,task_sts_name(this%task_list(idx)%status) )
    end if
    write(*,*)  "LBM,TASK export"," : ","EXIT" 
  end subroutine task_export

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_task_export(this) 
    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)   		:: this
    logical 						:: flag
    integer 						:: err,req,idx,load
    type(task_info)                                     :: task


    write(*,*)  "LBM,CHECK TASK export"," : ","ENTER"

    do idx = 1, ubound(this%task_list,1)
       if ( this%task_list(idx)%id == TASK_INVALID_ID ) then 
          exit
       end if
    end do
    
    load=current_task_load(this)-this%node_capacity

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       write(*,*)  "LBM,task export received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)
       write(*,*)  "LBM,task export received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       write(*,*)  "LBM,task export received check its status src,tag,com,error,size,id,pid,req,name"," : ",(sts(MPI_SOURCE),MPI_TA&
&G_TASK&
# 243
,             this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name)

       call task_export_reply(this,load,sts(MPI_SOURCE))
    end if
    if (this%task_steal_dest <0 .and. load >0 ) then 
       call task_export(this)
    end if
    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       write(*,*)  "LBM,task export  received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)
       write(*,*)  "LBM,task steal received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       write(*,*)  "LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name"," : ",(sts(MPI_SOURCE),MPI_TAG&
&_TASK&
# 256
,             this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name)

       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_REJECTED) then 
             call task_export(this)
       end if
       if ( this%task_list(idx)%type == TTYPE_TASK_STEAL_ACCEPTED) then 
          this%task_list(this%task_list(idx)%id)%proc_id =  sts(MPI_SOURCE)
          call lsnr_redirect(this,this%task_list(idx)%id,sts(MPI_SOURCE))
       end if 
    end if
    write(*,*)  "LBM,CHECK TASK export"," : ","Exit"

  end subroutine check_task_export

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_task_steal(this) 
    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)   		:: this
    logical 						:: flag
    integer 						:: err,req,idx,load
    type(task_info)                                     :: task


    write(*,*)  "LBM,CHECK TASK STEAL"," : ","ENTER"

    do idx = 1, ubound(this%task_list,1)
       if ( this%task_list(idx)%id == TASK_INVALID_ID ) then 
          exit
       end if
    end do
    
    load=current_task_load(this)-this%node_capacity

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       write(*,*)  "LBM,task steal received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)
       write(*,*)  "LBM,task steal received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       write(*,*)  "LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name"," : ",(sts(MPI_SOURCE),MPI_TAG&
&_TASK&
# 295
,             this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name)

       call task_steal_reply(this,load,sts(MPI_SOURCE))
    end if
    write(*,*)  "LBM, dest ,load"," : ",(this%task_steal_dest,load  )
    if (this%task_steal_dest <0 .and. load <0 ) then 
       call task_steal_send(this)
    end if
    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       write(*,*)  "LBM,task steal received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL_RESPONSE, this%mpi_comm, sts, err)
       write(*,*)  "LBM,task steal received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 
       write(*,*)  "LBM,task steal received check its status src,tag,com,error,size,id,pid,req,name"," : ",(sts(MPI_SOURCE),MPI_TAG&
&_TASK&
# 309
,             this%mpi_comm, err,sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,req,this%task_list(idx)%name)

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
    write(*,*)  "LBM,CHECK TASK STEAL"," : ","EXIT"

  end subroutine  check_task_steal

!!$------------------------------------------------------------------------------------------------------------------
  function rma_get_upper_node(this) result (node)
    type(remote_access),intent(inout) :: this
    integer                           :: node

    node = (this%node_cnt-1) / this%group_size
    write(*,*)  "LBM,Upper node"," : ",node

  end function rma_get_upper_node

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_transfer_task(this, f,t,w)
    type(remote_access), intent(inout)  :: this
    integer            , intent(in)     :: f,t,w
    integer                             :: idx,err
    type(task_info)                     :: task

    write(*,*)  "LBM,Xfer task,from,to,wght"," : ",(f,t,w)

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

    write(*,*)  "LBM,local balance "," : ","Enter"
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
       write(*,*)  "LBM,local balance min,max,minidx,maxidx"," : ",(mn,mx,mni,mxi)
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
       write(*,*)  "LBM,local balance send to upper node;cnt,max"," : ",(wl_cnt ,mx_wl)
       call rma_send_load(this)
    end if
  end subroutine rma_local_balance

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_export_load(this,w,t)
    type(remote_access) , intent(inout)   :: this
    integer             , intent(in)      :: w,t
    integer                               :: idx,mxwi,xw

    write(*,*)  "LBM,export load ;wght,to"," : ",(w ,t)
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
       write(*,*)  "LBM,export load ;mxwi"," : ",mxwi
       if (mxwi<0) exit
       this%task_list(mxwi)%proc_id=t
       call task_send( this,mxwi)
       call lsnr_redirect(this,this%task_list(mxwi)%id,t)
       this%task_list(mxwi)%type=TTYPE_TASK_STEAL_ACCEPTED
       xw = xw-this%task_list(mxwi)%weight
       write(*,*)  "LBM,export load ;new wght"," : ",xw
    end do

  end subroutine rma_export_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_send_load(this)
    type(remote_access) , intent(inout)   :: this
    integer                               :: xload,upper_node,msg_size,req,err

    write(*,*)  "LBM,send load "," : ","enter"
    xload = current_task_load(this) - this%node_capacity
    upper_node = rma_get_upper_node(this)
    msg_size= size(this%work_load_list) 
    write(*,*)  "LBM,send load, upper,extra loadm msg sz"," : ",(xload,upper_node,msg_size)

    if (.not. this%single_load_sent ) then  
       this%work_load_list(:,:)=0
       this%work_load_list(1,0)=this%proc_id
       this%work_load_list(1,1)=xload
    end if
    call  MPI_ISEND (this%work_load_list(1,1),msg_size,MPI_INTEGER, & 
         upper_node,MPI_TAG_WORK_LOAD,this%mpi_comm,req,err) 

    write(*,*)  "LBM,send load "," : ","exit"

  end  subroutine rma_send_load

!!$------------------------------------------------------------------------------------------------------------------
  subroutine check_group_bal(this)

    integer		, dimension(1:MPI_STATUS_SIZE)  :: sts
    type(remote_access) , intent(inout)                 :: this
    logical                                             :: flag
    integer                                             :: err,idx
    type(task_info)                                     :: task

    write(*,*)  "LBM,check group bal, single done?  "," : ",this%single_load_sent
    if (.not. this%single_load_sent ) then  
       call rma_send_load(this)
       this%single_load_sent= .true.
    end if

    call MPI_IPROBE(MPI_ANY_SOURCE,MPI_TAG_WORK_LOAD, this%mpi_comm,flag,sts,err)
    if ( flag ) then 
       write(*,*)  "LBM,check group bal, work load recvd,src "," : ",sts(MPI_SOURCE)
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
       write(*,*)  "LBM,check group bal, transfer task recvd,src "," : ",sts(MPI_SOURCE)
       call  MPI_IRECV (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            sts(MPI_SOURCE),MPI_TAG_TRANSFER_TASK,this%mpi_comm,this%task_req(idx),err) 
       call rma_export_load(this,this%task_list(idx)%id,this%task_list(idx)%proc_id)
    end if

  end subroutine check_group_bal

!!$------------------------------------------------------------------------------------------------------------------
  function dlb_is_idle(rma) result(answer)
    type(remote_access) , intent(inout)    :: rma
    type(dlb_idle_type) :: this
    integer                                :: answer
    
    answer = 0 
    if ( .not. dlb_did_all_idle_replied(this) ) then 
       return 
    end if
  end function dlb_is_idle

!!$------------------------------------------------------------------------------------------------------------------
  function dlb_get_dests_for_idle(this,rma) result ( dest)
    type(dlb_idle_type) , intent(inout) :: this
    type(remote_access) , intent(inout):: rma
    integer , dimension ( :) , pointer  :: dest
    integer , dimension ( :) , pointer :: nodes
    allocate ( nodes(1:10) )
    dest => nodes
  end function dlb_get_dests_for_idle
!!$------------------------------------------------------------------------------------------------------------------

  subroutine dlb_send_idle_msg(this,nodes,amnt)
    type(dlb_idle_type) , intent(inout) :: this
    integer , dimension ( :) , pointer :: nodes
    integer , intent(in) :: amnt

  end subroutine dlb_send_idle_msg
  
!!$------------------------------------------------------------------------------------------------------------------
  function dlb_did_all_idle_replied(this) result ( answer ) 
    type(dlb_idle_type) , intent(inout) :: this
    logical :: answer 
    answer = .false.
  end function dlb_did_all_idle_replied
!!$------------------------------------------------------------------------------------------------------------------

end module dist_dlb
