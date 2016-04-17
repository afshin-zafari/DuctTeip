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
       

       VIZIT4(EVENT_TASK_SEND_REQUESTED,idx,this%task_list(idx)%proc_id,0)
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
    TRACEX("LBM, export reply, send response ",idx)
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

    write(*,*)  "LBM, lsnr redirect,task,owner "," : ",(task_id,new_owner)
    Do i=1,Ubound(this%task_list,1)
       If (this%task_list(i)%id == task_id ) Then 
          Do j=1,Ubound(this%task_list(i)%axs_list,1)
             If (this%task_list(i)%axs_list(j)%access_types /= AXS_TYPE_READ) Then 
                wrt_data_name =  this%task_list(i)%axs_list(j)%data%name
             End If
          End Do
       End If
    End Do
    
    TRACE2("LBM, lsnr redirect,dname ",wrt_data_name)
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
       TRACEX("LBM,TASK export,dest",this%task_steal_dest)
       TRACEX("LBM,task idx,proc,comm,name",(idx,this%task_list(idx)%proc_id,this%mpi_comm,this%task_list(idx)%name))
       this%task_list(idx)%id=0
       Call  MPI_ISEND (this%task_list(idx),sizeof(task),MPI_BYTE, & 
            this%task_steal_dest,MPI_TAG_TASK_STEAL,this%mpi_comm,this%task_req(idx),err) 
!       TRACEX("LBM,tag,err,req,size,id,pid,name after ISEND",(MPI_TAG_TASK,err,this%task_req(idx),sizeof(task),this%task_list(idx)%id,this%task_list(idx)%proc_id,this%task_list(idx)%name))

       this%task_list(idx)%status = COMM_STS_SEND_PROGRESS
       TRACEX("LBM,task sts",(idx,task_sts_name(this%task_list(idx)%status) ))
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
       write(*,*)  "LBM,task export received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 


       Call MPI_RECV ( this%task_list(idx), sizeof(task), MPI_BYTE,&
            sts(MPI_SOURCE), MPI_TAG_TASK_STEAL, this%mpi_comm, sts, err)


       TRACEX("LBM,task export received  error,flag,sts,name",(err,flag,sts,this%task_list(idx)%name) )
!       TRACEX("LBM,task export received check its status src,tag,com,error,size,id,pid,req,name",(sts(MPI_SOURCE),MPI_TA&
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
       write(*,*)  "LBM,task export  received  error,flag,sts,name"," : ",(err,flag,sts,this%task_list(idx)%name) 


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

    write(*,*)  "LBM,Xfer task,from,to,wght"," : ",(f,t,w)

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

