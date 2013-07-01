# 1 "dist_lsnr_class.F90"

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





# 3 "dist_lsnr_class.F90" 2 

Module dist_lsnr_class

  Use fp
  Use mpi
  Use dist_const
  Use dist_types
  Use dist_common
  Use dist_data_class

  Implicit None

!!$-------------------------------------------------------------------------------------------------------------------
!!$  lsnr_check_sts_all              Loop for checking the status of listeners one by one
!!$  lsnr_check_sts                  Checks the new status of a listener based on events and/or current status
!!$  lsnr_clean                      Destroys the Listener and remove it from the list (flag it as cleaned)
!!$  lsnr_data_recv                  Processes the DATA RCVD event for all related Listeners
!!$  lsnr_find_by_sts                Returns the listener whose status is as given
!!$  lsnr_find_data                  Returns the Data object for which the listener is defined
!!$  lsnr_send                       Sends the listener object
!!$  lsnr_send_data                  Sends the data of a Listener
!!$  lsnr_wakeup                     Wakes up all the listeners whose Data is as given. Called when Data is received.
!!$-------------------------------------------------------------------------------------------------------------------

Contains 

!!$-------------------------------------------------------------------------------------------------------------------
  Subroutine lsnr_check_sts_all(this,notified_from,event)

    Type(remote_access) , Intent(inout) :: this
    Integer		, Intent(inout) :: event
    Integer		, Intent(in   ) :: notified_from
    Integer 				:: i 

    

    Do i = Lbound(this%lsnr_list,1) , Ubound(this%lsnr_list,1)
       If ( this%lsnr_list(i)%id /= LSNR_INVALID_ID) Then 
          Call lsnr_check_sts(this,this%lsnr_list(i),event)
       End If
    End Do

    

  End Subroutine lsnr_check_sts_all

!!$---------------------------------------------------------------------------------------------------------------------
!!$   This routine, checks the current status of the input Listener  and the last event happened in the program. Then it 
!!$   decides on what will be the next status and whom has to be notified accordingly. The procedure can be 
!!$   summarized as following table:
!!$   
!!$       Event        Condition            From Status    To Status         Notify
!!$   --------------   ----------           -----------    ---------         ---------
!!$    Listener RCVD    --                   CLEANED        RECEIVED           --
!!$     --              --                   CLEANED        INIT               --
!!$     --              --                   RECEIVED       TASK WAIT          --
!!$     --             is remote             INIT           SEND INIT         MailBox
!!$    Data RCVD       is Not remote         INIT           DATA RCVD         Data
!!$    Listener  RCVD  is Not remote         INIT           TASK WAIT         --
!!$    Task Finished    --                   TASK WAIT      DATA SENT         MailBox,Data
!!$     --             Data is Read/Add OK   TASK WAIT      DATA SENT         MailBox,Data
!!$     --              --                   DATA SENT      DATA ACK           --             
!!$    Data ACK        No pending listener   ACTIVE         CLEANED            -- 
!!$    Data RCVD        --                   ACTIVE         DATA RCVD          -- 
!!$     --             No pending listener   DATA RCVD      CLEANED            -- 
!!$     --             No pending listener   DATA ACK       CLEANED            -- 
!!$    Listener ACK     --                   NO ACK         LSNR ACK           --
!!$     --              --                   LSNR ACK       ACTIVE             --
!!$---------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_check_sts(this,lsnr,event)

    Type(remote_access) , Intent(inout) :: this
    Type(listener)      , Intent(inout) :: lsnr
    Integer		, Intent(inout) :: event
    Type(data_handle)                   :: Data
    Integer 				:: from_sts,to_sts,notify
    Logical 				:: answer

    from_sts = lsnr%status
    to_sts = from_sts
    notify = NOTIFY_DONT_CARE

    Select Case (from_sts)
       Case (LSNR_STS_CLEANED) 
          If (event == EVENT_LSNR_RECEIVED) Then 
             
             Call lsnr_add_to_list(this,lsnr)
             to_sts = LSNR_STS_RCVD
          Else
             to_sts = LSNR_STS_INITIALIZED
          End If
       Case (LSNR_STS_RCVD)
          to_sts = LSNR_STS_TASK_WAIT

       Case (LSNR_STS_INITIALIZED)
          If ( lsnr%proc_id /= this%proc_id ) Then ! lsnr is for remote proc
             Call ll_add_item(this%send_list,lsnr%id,LSNR_OBJ)
             to_sts = COMM_STS_SEND_INIT
             notify = NOTIFY_OBJ_MAILBOX
          Else
             If ( event == EVENT_DATA_RECEIVED ) Then 
                to_sts = LSNR_STS_DATA_RCVD
                notify = NOTIFY_OBJ_DATA
             End If
             If ( event == EVENT_LSNR_RECEIVED ) Then 
                to_sts = LSNR_STS_TASK_WAIT
             End If

          End If

       Case (LSNR_STS_TASK_WAIT)
          If ( event == EVENT_TASK_FINISHED ) Then 
             to_sts = LSNR_STS_DATA_SENT
             notify = NOTIFY_OBJ_MAILBOX + NOTIFY_OBJ_DATA
             
             Call lsnr_send_data(this,lsnr,event,to_sts)
          Else
             
             data = lsnr%req_data
             If ( data%id /= DATA_INVALID_ID ) Then 
                If (data%status == DATA_STS_READ_READY .Or. data%status == DATA_STS_MODIFY_READY) Then 
                   to_sts = LSNR_STS_DATA_SENT
                   notify = NOTIFY_OBJ_MAILBOX + NOTIFY_OBJ_DATA
                   
                   Call lsnr_send_data(this,lsnr,event,to_sts)                   
                End If
             End If
          End If
       Case (LSNR_STS_DATA_SENT)
             to_sts = LSNR_STS_DATA_ACK
       Case (LSNR_STS_ACTIVE)
          If ( event == EVENT_DATA_ACK ) Then 
             Call is_any_pending_task(this,lsnr%req_data,event,answer)
             If (.Not. answer) Then 
                to_sts  = LSNR_STS_CLEANED
                
                Call lsnr_clean(this,lsnr,event)
             End If
          End If
          If ( event == EVENT_DATA_RECEIVED ) Then 
             to_sts  = LSNR_STS_DATA_RCVD
          End If
       Case (LSNR_STS_DATA_RCVD,LSNR_STS_DATA_ACK) 
          Call is_any_pending_task(this,lsnr%req_data,event,answer)
          If (.Not. answer) Then 
             to_sts  = LSNR_STS_CLEANED
             Call lsnr_clean(this,lsnr,event)         
          End If
       Case (COMM_STS_SEND_PROGRESS)
          If ( event == EVENT_LSNR_ACK) Then 
             to_sts = COMM_STS_SEND_COMPLETE
          End If
       Case (COMM_STS_SEND_COMPLETE)
          to_sts = LSNR_STS_ACTIVE
       Case DEFAULT

    End Select

    lsnr%status = to_sts
    this%notify = notify

    If (to_sts /= from_sts .Or. notify /= NOTIFY_DONT_CARE) Then 
       If (lsnr%id /= LSNR_INVALID_ID) Then 
          call instrument(EVENT_LSNR_STS_CHANGED,lsnr%id,0,0)
       End If
    End If

   
   

  End Subroutine lsnr_check_sts
!!$-------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_clean(this,lsnr,EVENT)

    Type(remote_access) , Intent(inout) :: this
    Type(listener)      , Intent(inout) :: lsnr
    Integer		, Intent(inout) :: event

    
    
    call instrument(EVENT_LSNR_CLEANED,lsnr%id,0,0)
    lsnr%status = LSNR_STS_CLEANED
    lsnr%id     = LSNR_INVALID_ID

  End Subroutine lsnr_clean
!!$-------------------------------------------------------------------------------------------------------------------


  Subroutine lsnr_data_recv(this,Data)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer                             :: i,event,to_sts

    Do i = Lbound(this%lsnr_list,1), Ubound(this%lsnr_list,1)
       If ( this%lsnr_list(i)%id /=  LSNR_INVALID_ID ) Then 
          If (this%lsnr_list(i)%req_data%id /= DATA_INVALID_ID) Then 
             If(this%lsnr_list(i)%proc_id == this%proc_id) Then  ! It is local means that we are waiting for the data
                If (this%lsnr_list(i)%req_data%name(1:MAX_DATA_NAME) == data%name(1:MAX_DATA_NAME)) Then 
                   this%lsnr_list(i)%status = LSNR_STS_DATA_RCVD
                   call instrument(EVENT_LSNR_DATA_RECEIVED,i,0,0)
                End If
             Else   ! It is Remote, means that some other processors need this received data. So, send the data to them.
                If (this%lsnr_list(i)%req_data%name(1:MAX_DATA_NAME) == data%name(1:MAX_DATA_NAME)) Then 
                   to_sts = LSNR_STS_DATA_SENT
                   event = EVENT_DATA_RECEIVED
                   call instrument(EVENT_LSNR_DATA_SENT,i,-1,0)
                   Call lsnr_send_data(this,this%lsnr_list(i),event,to_sts)
                   this%lsnr_list(i)%status = to_sts
                End If
             End If
          End If
       End If
    End Do

  End Subroutine lsnr_data_recv
!!$-------------------------------------------------------------------------------------------------------------------


  Function lsnr_find_by_sts(this,sts) Result (lsnr)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in   ) :: sts
    Type(listener)                      :: lsnr
    Integer 				:: i 

    lsnr%id = LSNR_INVALID_ID
    Do i= Lbound(this%lsnr_list,1),Ubound(this%lsnr_list,1)
       If (this%lsnr_list(i)%status == sts ) Then 
          lsnr = this%lsnr_list(i)
          Return
       End If
    End Do

  End Function lsnr_find_by_sts
!!$-------------------------------------------------------------------------------------------------------------------

  Function lsnr_find_data(this,lsnr) Result(Data)

    Type(remote_access) , Intent(inout) :: this
    Type(listener)      , Intent(inout) :: lsnr
    Integer                             :: i
    Type(data_handle)                   :: Data

    Do i = Lbound(this%lsnr_list,1), Ubound(this%lsnr_list,1)
       If ( this%lsnr_list(i)%id ==  lsnr%id ) Then 
          Data  = this%lsnr_list(i)%req_data
          Return
       End If
    End Do

  End Function lsnr_find_data

!!$-------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_send(this, idx)

    Type(remote_access) , Intent(inout) :: this
    Integer             , Intent(in)    :: idx
    Type(listener)                      :: lsnr
    Integer 				:: err

# 269


    If (idx <= Ubound(this%lsnr_req,1)) Then 
       this%lsnr_list(idx)%proc_id = this%proc_id
    
# 276


       Call MPI_ISSEND (this%lsnr_list(idx),sizeof(lsnr),MPI_BYTE,this%lsnr_list(idx)%req_data%proc_id, &
            MPI_TAG_LSNR,this%mpi_comm,this%lsnr_req(idx),err)

# 284

       
       call instrument(EVENT_LSNR_SENT_REQUESTED,idx,this%lsnr_list(idx)%req_data%proc_id,0)
       this%lsnr_list(idx)%status = COMM_STS_SEND_PROGRESS
       write(*,*)  "Listener Sent, idx,dname,tag,sz,err,to"," : ",(idx,this%lsnr_list(idx)%dname,MPI_TAG_LSNR,               sizeof&
# 288
(lsnr),err,this%lsnr_list(idx)%req_data%proc_id)

    End If

    
    
  End Subroutine lsnr_send
!!$-------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_send_data(this,lsnr,event,to_sts)

    Type(remote_access) , Intent(inout) :: this
    Type(listener)      , Intent(inout) :: lsnr
    Integer		, Intent(inout) :: event,to_sts
    Type(data_handle)                   :: Data

    
    data = lsnr%req_data
    Call data_send(this,Data,lsnr%proc_id)
    call instrument(EVENT_LSNR_DATA_SENT,data%id,0,0)

  End Subroutine lsnr_send_data
!!$-------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_wakeup(this,Data)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer                             :: i,event

    event = EVENT_TASK_FINISHED
    Do i = Lbound(this%lsnr_list,1), Ubound(this%lsnr_list,1)
       If ( this%lsnr_list(i)%id /=  LSNR_INVALID_ID ) Then 
          If (this%lsnr_list(i)%proc_id /= this%proc_id) Then 
             If (this%lsnr_list(i)%req_data%id /= DATA_INVALID_ID) Then 
                If (this%lsnr_list(i)%req_data%name(1:MAX_DATA_NAME) == data%name(1:MAX_DATA_NAME)) Then 
                   this%lsnr_list(i)%status = LSNR_STS_DATA_SENT
                   call instrument(EVENT_LSNR_DATA_SENT,this%lsnr_list(i)%req_data%id,-2,0)
                   Call lsnr_send_data(this,this%lsnr_list(i),event,this%lsnr_list(i)%status)
                Else
                End If
             End If
          End If 
       End If
    End Do
 
  End Subroutine lsnr_wakeup
!!$-------------------------------------------------------------------------------------------------------------------


End Module dist_lsnr_class
