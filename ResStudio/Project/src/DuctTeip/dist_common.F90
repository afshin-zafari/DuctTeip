
#include "debug.h"

Module dist_common

  Use fp
  Use mpi
  Use timer
  Use dist_const
  Use dist_types
  Use class_point_set
  use pthreads


  Implicit None
  type timing
     integer :: a,c,d
     character(len=20) :: b 
     real(kind=rfp)::t
  end type timing
  type(timing),dimension(1:2000000)::tlist
  integer ::tlist_idx=0
  integer,dimension(1:5)::task_cnt = (/0,0,0,0,0/)
  type(mutex) ::muTiming

Contains
!!$-------------------------------------------------------------------------------------------------------------------------
   Function axs_sts_name(sts) Result(name)

     Integer         , Intent(in) :: sts
     Character(len=3)             :: name
     
     name="---"
     If ( ( mod(sts,AXS_TYPE_READ*10)/AXS_TYPE_READ ) /= 0&
# 24
                                        ) name(3:3) = 'R'
     If ( ( mod(sts,AXS_TYPE_WRITE*10)/AXS_TYPE_WRITE ) /= 0&
# 25
                                        ) name(2:2) = 'W'
     If ( ( mod(sts,AXS_TYPE_MODIFY*10)/AXS_TYPE_MODIFY ) /= 0&
# 26
                                        ) name(1:1) = 'M'
     
   End Function axs_sts_name
!!$------------------------------------------------------------------------------------------------------------------------
   function block_get_range(M,rb,cb,nb) result (range)
     integer, dimension ( 1:4)::range
     integer , intent(in) :: rb,cb,nb
     real(kind=rfp) , dimension ( :,:),intent(in) :: M
     integer  :: blk_rows,blk_cols
     blk_rows = ubound(M,1)/nb
     blk_cols = ubound(M,2)/nb
     range(1)=(rb-1)*blk_rows+1
     range(2)=min(rb*blk_rows,ubound(M,1))
     range(3)=(cb-1)*blk_cols+1
     range(4)=min(cb*blk_cols,ubound(M,2))
   end function block_get_range

!!$------------------------------------------------------------------------------------------------------------------------

  Subroutine data_add_to_list(this,Data)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer 		                :: i

    

    Do i = Lbound(this%data_list,1), Ubound(this%data_list,1)
       If ( this%data_list(i)%status == DATA_STS_CLEANED ) Then 
          Exit
       End If
    End Do
    If ( i <= Ubound(this%data_list,1) ) Then 
       this%data_list(i) = Data 
       this%data_list(i)%id = i
       data%id = i
       this%data_list(i)%status = DATA_STS_INITIALIZED
    Else
       Write(*,*)  "*** Data List is Full"," : ",i,data%name
    End If

    

  End Subroutine data_add_to_list
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine data_clean(this,Data,event)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer 		, Intent(inout) :: event

    

    
!!$    If ( Associated (data%buf) ) Then 
!!$       Deallocate(data%buf)
!!$       Nullify(data%buf)
!!$    End If
    TRACEX("Data Cleaned,",data%name)
    data%id     = DATA_INVALID_ID


  ! call destruct_pts(data%pts)

    

  End Subroutine data_clean
!!$-------------------------------------------------------------------------------------------------------------------------

  Function data_equal(d1,d2) Result(answer)

    Type(data_handle)   , Intent(in) :: d1,d2
    Logical                          :: answer 

    answer = d1%name == d2%name

  End Function data_equal
!!$-------------------------------------------------------------------------------------------------------------------------

  Function data_sts_name(sts) Result(name)

     Integer          , Intent(in) :: sts
     Character(len=7)              :: name
     
     Select Case(sts)
        Case(DATA_STS_INITIALIZED)
           name=" INIT "
        Case(DATA_STS_READ_READY)
           name="READOK "
        Case(DATA_STS_MODIFY_READY)
           name="MOD_OK "
        Case(DATA_STS_CLEANED)
           name=" CLEAN "  
        Case(COMM_STS_SEND_INIT )
           name="SENT "
        Case(COMM_STS_SEND_PROGRESS)
           name=" NOACK "
        Case(COMM_STS_SEND_COMPLETE )
           name="ACKRCV "
        End Select
   End Function data_sts_name


!!$-------------------------------------------------------------------------------------------------------------------------

   Function event_name(event) Result(name)

    Integer          ,Intent(in):: event
    Character(len=20)            :: name

    Select Case(event)
        Case (EVENT_DONT_CARE )
          name = "NOEVENT "
        Case(EVENT_DATA_RECEIVED 	)
          name = "DATA_RECEIVED "
        Case(EVENT_LSNR_RECEIVED 	)
          name = "LSNR_RECEIVED "
        Case(EVENT_TASK_RECEIVED 	)
          name = "TASK_RECEIVED "
        Case(EVENT_LSNR_ACK 	)
          name = "LSNR_ACK "
        Case(EVENT_TASK_ACK 	)
          name = "TASK_ACK "
        Case(EVENT_DATA_ACK 	)
          name = "DATA_ACK "
        Case(EVENT_TASK_STARTED 	)
          name = "TASK_STARTED "
        Case(EVENT_TASK_RUNNING 	)
          name = "TASK_RUNNING "
        Case(EVENT_TASK_FINISHED 	    )
          name = "TASK_FINISHED "
        Case(EVENT_ALL_TASK_FINISHED    )
          name = "ALL_TASK_FINISHED "
        Case(EVENT_DATA_STS_CHANGED     )
          name = "DATA_STS_CHANGED "
        Case(EVENT_TASK_STS_CHANGED     )
          name = "TASK_STS_CHANGED "
        Case(EVENT_LSNR_STS_CHANGED     )
          name = "LSNR_STS_CHANGED "
        Case(EVENT_DATA_ADDED           )
          name = "DATA_ADDED "
        Case(EVENT_TASK_ADDED           )
          name = "TASK_ADDED "
        Case(EVENT_DATA_POPULATED       )
          name = "DATA_POPULATED "
        Case(EVENT_LSNR_CLEANED         )
          name = "LSNR_CLEANED "
        Case(EVENT_LSNR_DATA_RECEIVED   )
          name = "LSNR_DATA_RECEIVED "
        Case(EVENT_LSNR_DATA_SENT       )
          name = "LSNR_DATA_SENT "
        Case(EVENT_TASK_CHECKED         )
          name = "TASK_CHECKED "
        Case(EVENT_TASK_CLEANED         )
          name = "TASK_CLEANED "
        Case(EVENT_DATA_SEND_REQUESTED  )
          name = "DATA_SEND_REQUESTED "
        Case(EVENT_LSNR_SENT_REQUESTED  )
          name = "LSNR_SENT_REQUESTED "
        Case(EVENT_TASK_SEND_REQUESTED  )
          name = "TASK_SEND_REQUESTED "
        Case(EVENT_CYCLED               )
          name = "CYCLED "
        Case(EVENT_LSNR_ADDED           )
          name = "LSNR_ADDED "
        Case(EVENT_DATA_DEP             )
          name = "DATA_DEP "
        Case(EVENT_SUBTASK_STARTED 	    )
          name = "SUBTASK_STARTED "
        Case(EVENT_SUBTASK_FINISHED     )
          name = "SUBTASK_FINISHED "
        Case(EVENT_DTLIB_DIST           )
          name = "DTLIB_DIST "
        Case(EVENT_DTLIB_EXEC     	    )
          name = "DTLIB_EXEC "
        Case(EVENT_SRCH_LIST     	    )
          name = "SRCH_LIST "
        Case default
          name = "------- "
    End Select

  End Function event_name
!!$-------------------------------------------------------------------------------------------------------------------------

  Function first_n_pts(Data)Result(list)

    Type(data_handle) , Intent(in)            :: Data
    Type(point_set)                           :: pts
    Integer        , Dimension(:,:) , Pointer :: ids
    Real(kind=rfp) , Dimension(:,:) , Pointer :: ptr
    Integer                                   :: n,np,nd,p,d,t
    Character(len=30)                         :: list

    pts  = data%pts
    n    = 5 
    np   = data%np
    nd   = data%nd
    list = ""
    If ( np <= 0 .Or. nd <= 0) Return
    
    
  End Function first_n_pts
!!$-------------------------------------------------------------------------------------------------------------------------
 subroutine dump_timing() 
   integer :: i 
   TRACEX( "Total Tasks Count" ,( task_cnt(i)/2,i=1,5))
   do i =1,tlist_idx
      !if (tlist(i)%t == 0.0 ) cycle
      write(*,*) event_name(tlist(i)%a),',',tlist(i)%t/1e6,',',tlist(i)%a,',',tlist(i)%b,',',tlist(i)%c,',',tlist(i)%d 
   end do
 end subroutine dump_timing
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine instrument(a,b,c,d)
    Integer , Intent (in) :: a,b,c,d
    if ( a == EVENT_CYCLED       ) return
    if ( a == EVENT_TASK_CHECKED ) return
    write(*,*) event_name(a),',',(MPI_Wtime()),',',a,',',b,',',c,',',d 
    
!    If ( a == EVENT_DTLIB_DIST   .or.  a == EVENT_DTLIB_EXEC .or. a == EVENT_LSNR_SENT_REQUESTED .or. a == EVENT_LSNR_ACK &
!         .or. a == EVENT_LSNR_RECEIVED ) Then 
!       write(*,*) event_name(a),(MPI_Wtime()),a,b,c,d 
!    End If 
  End Subroutine instrument

!!$-------------------------------------------------------------------------------------------------------------------------
  subroutine init_timing()
    call pthreads_mutex_init(muTiming)
  end subroutine init_timing
!!$-------------------------------------------------------------------------------------------------------------------------
  subroutine instrument2(a,b,c,d)
    Integer , Intent (in) :: a,c,d
    character(len=*) ,intent(in) :: b
    if ( a == EVENT_CYCLED       ) return
    if ( a == EVENT_TASK_CHECKED ) return
    call pthreads_mutex_lock(muTiming)
    tlist_idx = tlist_idx+1
    task_cnt(c)=task_cnt(c)+1
    if ( tlist_idx > ubound(tlist,1) ) then 
       write (*,*) "Timing List exhausted.!!!"
       call pthreads_mutex_unlock(muTiming)
       return 
    end if
    tlist(tlist_idx)%a = a 
    tlist(tlist_idx)%b = b 
    tlist(tlist_idx)%c = c 
    tlist(tlist_idx)%d = d 
    tlist(tlist_idx)%t = getime()
    call pthreads_mutex_unlock(muTiming)
  end subroutine instrument2

  Subroutine is_any_pending_lsnr(this,Data,event,answer)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer 		, Intent(inout) :: event
    Logical 		, Intent(  out) :: answer
    Type(listener)	                :: lsnr
    Integer 				:: i 

    

    answer = .False.
    Do i = Lbound(this%lsnr_list,1), Ubound(this%lsnr_list,1)
       ! MT: Checking that listener is valid.
       ! The check below requires that req_data has been nullified,
       ! which does not seem to happen.
       If ( this%lsnr_list(i)%id == LSNR_INVALID_ID ) Then
          Cycle
       End If

       If (.Not. Associated (this%lsnr_list(i)%req_data) ) Then 
          
          Cycle
       End If
       If ( data_equal(this%lsnr_list(i)%req_data, Data) ) Then 
          If (this%lsnr_list(i)%status ==  LSNR_STS_ACTIVE  .Or. & 
               this%lsnr_list(i)%status /= LSNR_STS_CLEANED  ) Then 
             answer = .True.
             lsnr = this%lsnr_list(i)             
             Return
          End If
       End If

    End Do

    

  End Subroutine is_any_pending_lsnr
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine  is_any_pending_task(this,Data,event,answer)

    Type(remote_access) , Intent(inout) :: this
    Type(data_handle)   , Intent(inout) :: Data
    Integer 		, Intent(inout) :: event
    Logical 		, Intent(  out) :: answer
    Integer 				:: i ,j
    answer = .False.
    Do i = Lbound(this%task_list,1), Ubound(this%task_list,1)
       Do j = Lbound(this%task_list(i)%axs_list,1), Ubound(this%task_list(i)%axs_list,1)
          If ( .Not. Associated(this%task_list(i)%axs_list(j)%data) ) Cycle
          If ( this%proc_id == this%task_list(i)%proc_id ) Then 
             If ( data_equal(this%task_list(i)%axs_list(j)%data, Data) ) Then
                If (this%task_list(i)%status /= TASK_STS_FINISHED  .And. & 
                     this%task_list(i)%status /= TASK_STS_CLEANED  ) Then 
                   answer = .True.
                   Return
                End If
             End If
          End If
       End Do
    End Do

  End Subroutine  is_any_pending_task
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine lsnr_add_to_list(this,lsnr)

    Type(remote_access) , Intent(inout) :: this
    Type(listener)      , Intent(inout) :: lsnr
    Integer 				:: i

    lsnr%id = LSNR_INVALID_ID

    Do i = Lbound(this%lsnr_list,1), Ubound(this%lsnr_list,1)
       If ( this%lsnr_list(i)%status == LSNR_STS_CLEANED ) Then 
          Exit
       End If
    End Do

    If ( i <= Ubound(this%lsnr_list,1) ) Then 
       this%lsnr_list(i) = lsnr 
       this%lsnr_list(i)%id = i
       lsnr%id = this%lsnr_list(i)%id 
       this%lsnr_list(i)%dname=lsnr%req_data%name 
       this%lsnr_list(i)%status = LSNR_STS_INITIALIZED       
    Else
       write(*,*)  "*** Lsnr List is Full"," : ",i
    End If
    i = lsnr%req_data%id
    VIZIT4(EVENT_LSNR_ADDED,i,0,0)
    

  End Subroutine lsnr_add_to_list
!!$-------------------------------------------------------------------------------------------------------------------------

  Function lsnr_sts_name(sts) Result(name)

       Integer         , Intent(in) :: sts
       Character(len=9)             :: name

       Select Case(sts)
          Case(LSNR_STS_RCVD)
             name="RECEIVED "
          Case(LSNR_STS_DATA_ACK)
             name="DATA_ACK "
          Case(LSNR_STS_DATA_SENT)
             name="DATASENT "
          Case(LSNR_STS_INITIALIZED)
             name="   INIT  "
          Case(LSNR_STS_DATA_RCVD )
             name="DATARCVD "
          Case(LSNR_STS_TASK_WAIT)
             name="TASKWAIT "
          Case(LSNR_STS_ACTIVE)
             name="  ACTIVE "  
          Case(LSNR_STS_CLEANED)
             name=" CLEANED "  
          Case(COMM_STS_SEND_INIT )
             name="   SENT  "
          Case(COMM_STS_SEND_PROGRESS)
             name="  NOACK  "
          Case(COMM_STS_SEND_COMPLETE )
             name=" ACK_RCV "
       End Select

     End Function lsnr_sts_name
!!$-------------------------------------------------------------------------------------------------------------------------

  Function notify_name(notify) Result(name)

    Integer         ,Intent(in) :: notify
    Character(len=30)           :: name

    name = ""
    If ( ( mod(notify,NOTIFY_OBJ_DATA*10)/NOTIFY_OBJ_DATA ) /= 0&
# 329
                                                 ) name = name // " DATA "
    If ( ( mod(notify,NOTIFY_OBJ_TASK*10)/NOTIFY_OBJ_TASK ) /= 0&
# 330
                                                 ) name = name // " TASK "
    If ( ( mod(notify,NOTIFY_OBJ_LISTENER*10)/NOTIFY_OBJ_LISTENER ) /= 0&
# 331
                                                 ) name = name // " LSNR "
    If ( ( mod(notify,NOTIFY_OBJ_SCHEDULER*10)/NOTIFY_OBJ_SCHEDULER ) /= 0&
# 332
                                                 ) name = name // " SCHD "
    If ( ( mod(notify,NOTIFY_OBJ_MAILBOX*10)/NOTIFY_OBJ_MAILBOX ) /= 0&
# 333
                                                 ) name = name // " MBOX "
    If (          notify== NOTIFY_DONT_CARE      ) name =         " NONE "

  End Function notify_name
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine  pack_data_pts(this,Data,pts, buf,buf_size)

    Character(len=1)   ,Dimension(:)   , Pointer ,Intent (inout)  :: buf
    Real(kind=rfp)     ,Dimension(:,:) , Pointer                  :: pts_ptr        
    Integer            ,Dimension(:,:) , Pointer                  :: ids_ptr
    Integer            ,Dimension(1:2)                            :: rg
    Type(remote_access),Intent(inout)	                          :: this
    Type(data_handle)  ,Intent(inout)                             :: Data
    Type(point_set)    ,Intent(inout)	                          :: pts
    Integer            ,Intent(inout)                             :: buf_size
    Real ( kind=rfp)                                              :: pts_element
    Integer                                                       :: pts_offset,ids_offset,dim_cnt,l,ids_element,mat_offset
    Character ( len=15) :: dname


    If (opt_get_option ( this%opt, OPT_CHOLESKY ) /= 0 ) Then 
       ! MT: Already allocated?
       if (.not. associated (buf ) ) then 
          allocate ( buf ( 1 : buf_size)  )
       end if 
       mat_offset = sizeof (data%name)+sizeof(data%id) * 13 + sizeof(data%pts)+sizeof(data%buf)      +1
       buf( 1            : mat_offset  ) = Transfer ( Data     , buf, mat_offset   )
       buf( 1+mat_offset : buf_size    ) = Transfer ( data%mat , buf, -mat_offset + buf_size     )       
       TRACEX("packed data",(data%version,data%name))
       Call print_data_values(data%mat)
       
       Return 
    End If
    rg  = range_pts(pts)
    data%np = rg(2) - rg(1)+1
    data%nd = dims_pts(pts)
    data%nt = range_pts_id(pts)

    
    If ( data%np <= 0 ) Return
    TRACE2(  "??Data%Np",(data%np,data%nd,data%nt))
    Allocate ( ids_ptr (1:data%nt,1:ID_COL_MAX) , pts_ptr (1:data%np,1:data%nd) ) 
    ids_ptr    = get_ids_ptr(pts)
    pts_ptr    = get_pts_ptr(pts)

    
    

    ids_offset = sizeof (data%name)+sizeof(data%id) * 13 + 1
    pts_offset = ids_offset +data%nt * ID_COL_MAX * sizeof(ids_element)

    buf_size   = pts_offset + data%np * data%nd * sizeof(pts_element)+sizeof(data%buf)    
    
    TRACEX("Data Buf Size",buf_size)

!    MT: Already allocated?
!    Allocate ( buf ( 1 : buf_size)  )

    
    TRACE2("Before 1st buf copy",(ids_offset,0,sizeof(data)))
    buf( 1            : ids_offset  ) = Transfer ( Data     , buf, ids_offset   )
    TRACE2("Before 2nd buf copy",pts_offset)
    buf( 1+ids_offset : pts_offset  ) = Transfer ( ids_ptr  , buf, ( -ids_offset + pts_offset  ) )
    TRACE2("Before 3rd buf copy",buf_size)
    buf( 1+pts_offset : buf_size    ) = Transfer ( pts_ptr  , buf, ( -pts_offset + buf_size    ) )
    Deallocate (ids_ptr, pts_ptr)

    TRACE2("Before Xfer to name ",ids_offset)
    dname = Transfer ( buf( 1            : ids_offset  ), dname ) 
    TRACE2("After  Xfer to name ",dname)

    

  End Subroutine   pack_data_pts

!!$-------------------------------------------------------------------------------------------------------------------------
  Subroutine print_data(this,did,tname)
    Type(remote_access)	           :: this
    Character(len=MAX_TASK_NAME) ,Intent(in)  :: tname
    Integer , Intent(in)           :: did
    Integer 		           :: i,j,k
    Character(len=MAX_DATA_NAME)   :: dname
    Real(kind=rfp) , Dimension(:,:) , Pointer :: mat

    If ( Ubound(this%data_list(did)%mat,1) > 10 ) return 

    Write (*,*) "*******************-----------------------------------------*******************"
    i = did
    dname=this%data_list(i)%name
    TRACEX( "DataObj: ",(dname(1:13)," Id:",this%data_list(i)%id,i," ver: ",this%data_list(i)%version," output of: ",tname(1:7)))
    mat =>this%data_list(i)%mat
    Do j = 1,Ubound(mat,1)
       Do k = 1,Ubound(mat,2)
          print *, mat(j,k)
       End Do
       print *," "
    End Do
    Write (*,*) "******************------------------------------------------*******************"

  End Subroutine print_data
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine print_data_values(mat)
    Real(kind=rfp) , Dimension(:,:) , Pointer, Intent(in) :: mat
    Integer 		           :: i,j,k

    If ( Ubound(mat,1) > 10 ) return 
    Do j = 1,Ubound(mat,1)
       Do k = 1,Ubound(mat,2)
          Write ( *,"(F5.2)",advance='no') mat(j,k) 
       End Do
       Write (*,*) ""
    End Do

  End Subroutine print_data_values


!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine print_lists(this,force)

   Type(remote_access)	           :: this
   Integer 		           :: i 
   Logical , Intent(in) , Optional :: force

   If ( Present(force)) Then 
      If ( this % pc > 4 .And. (.Not. force) ) Return 
      If ( this % pc > 4  ) Return 
   Else
      If ( this % pc > 0  ) Return 
      If ( this %sync_stage < SYNC_TYPE_NONE   .Or. this % sync_stage > SYNC_TYPE_TERM_OK ) Return 
   End If

   

   Write(*,*) "TBL =================================================================="
   Write(*,*) "TBL Stage:",stage_name(this%sync_stage)," Event:",event_name(this%event)," Notifiy: ",notify_name(this%notify)
   Write(*,*) "TBL =================================================================="
   Write(*,*) "TBL -------------------------------------------------------------Data-"
   Write(*,*) "TBL id  sch_ver version pid status name "
   Do i= Lbound(this%data_list,1),Ubound(this%data_list,1)
      If (this%data_list(i)%id /= DATA_INVALID_ID ) Then 
         Write (*,*) "TBL",this%data_list(i)%id," ",this%data_list(i)%sv_r &
              ,this%data_list(i)%sv_w,this%data_list(i)%sv_m &
              ," ",this%data_list(i)%version,this%data_list(i)%proc_id &
              ,data_sts_name(this%data_list(i)%status),this%data_list(i)%name ,this%data_list(i)%has_lsnr
      End If
   End Do

   Write(*,*) "TBL -------------------------------------------------------------Task-"
   Write(*,*) "TBL id pid status name     axs min_ver d%pid d%name ..."
   Do i= Lbound(this%task_list,1),Ubound(this%task_list,1)
      If (this%task_list(i)%id /= TASK_INVALID_ID ) Then 
         Call print_task(this,this%task_list(i)) 
      End If
   End Do

   Write(*,*) "TBL -------------------------------------------------------------Lsnr-"
   Write(*,*) "TBL id pid status d:id,pid,name         "
   Do i= Lbound(this%lsnr_list,1),Ubound(this%lsnr_list,1)
      If ( .Not. Associated ( this%lsnr_list(i)%req_data ) ) Cycle
      If (this%lsnr_list(i)%id /= LSNR_INVALID_ID ) Then 
         Write (*,*) "TBL",this%lsnr_list(i)%id,this%lsnr_list(i)%proc_id &
              ,lsnr_sts_name(this%lsnr_list(i)%status),this%lsnr_list(i)%req_data%id &
              ,this%lsnr_list(i)%req_data%proc_id,this%lsnr_list(i)%req_data%name, &
              this%lsnr_list(i)%req_data%version,this%lsnr_list(i)%lsnr_min_ver
      End If
   End Do
   Write(*,*) "TBL =================================================================="

   
    
  End Subroutine print_lists 
!!$-------------------------------------------------------------------------------------------------------------------------

  Subroutine print_task(this,task)

   Type(remote_access)	:: this
   Type(task_info)	:: task
   Integer 		:: i  
   Character (len=100) :: dlist
   

   If ( task%type > SYNC_TYPE_NONE ) Then 
      Write (*,*) "TBL",task%id,task%proc_id,task_sts_name(task%status),task%name
      Return
   End If

   dlist = ""

   Do i = Lbound(task%axs_list,1),Ubound(task%axs_list,1)
      If ( .Not. Associated(task%axs_list(i)%data) ) Then 
         Cycle
      End If
      If ( task%axs_list(i)%data%id  /= DATA_INVALID_ID ) Then 
         Write(dlist((i-1)*25+1:i*25),"(A3 A1 I2 A1 I2 A1 A15)") axs_sts_name(task%axs_list(i)%access_types)," ", & 
              task%axs_list(i)%min_ver," ",task%axs_list(i)%data%proc_id,&
              " ",task%axs_list(i)%data%name
      Else
         Exit
      End If
   End Do
  Write (*,"(A5 I2 I2 A8 A5 A75) ") "TBL ",task%id,task%proc_id,   &
       task_sts_name(task%status),task%name(1:10),dlist(1:3*25)


  End Subroutine print_task

 Subroutine Reallocate_mem(dptr,tptr,lptr)
   Type(data_handle) , Dimension (:),Pointer , Intent(inout) :: dptr
   Type(task_info)   , Dimension (:),Pointer , Intent(inout) :: tptr
   Type(listener)    , Dimension (:),Pointer , Intent(inout) :: lptr
   Type(data_handle) , Dimension (:),Pointer                 :: dptr2
   Type(task_info)   , Dimension (:),Pointer                 :: tptr2
   Type(listener)    , Dimension (:),Pointer                 :: lptr2

   Integer :: new_size

   new_size = Size(dptr)*1.5
   Allocate ( dptr2(1:new_size) ) 
   dptr2(:)= dptr(:)
   Deallocate(dptr)
   dptr = dptr2

   new_size = Size(tptr)*1.5
   Allocate ( tptr2(1:new_size) ) 
   tptr2(:)= tptr(:)
   Deallocate(tptr)
   tptr = tptr2

   new_size = Size(lptr)*1.5
   Allocate ( lptr2(1:new_size) ) 
   lptr2(:)= lptr(:)
   Deallocate(lptr)
   lptr = lptr2
   


 End Subroutine Reallocate_mem

!!$-------------------------------------------------------------------------------------------------------------------------

  Function stage_name(stage) Result(name)

    Integer         ,Intent(in) :: stage
    Character(len=10)            :: name

    Select Case(stage)
       Case (SYNC_TYPE_NONE)
          name ="NONE "
       Case (SYNC_TYPE_SENDING_TASKS)
          name ="SENDING "
       Case (SYNC_TYPE_LAST_TASK)
          name ="LAST_TASK "
       Case (SYNC_TYPE_DATA_FREE)
          name ="DATA_FREE "
       Case (SYNC_TYPE_TERM_OK)
          name ="TERM_OK "
   End Select

 End Function stage_name
!!$-------------------------------------------------------------------------------------------------------------------------
 
 Function task_sts_name(sts) Result(name)

    Integer         ,Intent(in) :: sts
    Character(len=7)            :: name

    Select Case(sts)
       Case(TASK_STS_INITIALIZED)
          name=" INIT "
       Case(TASK_STS_WAIT_FOR_DATA)
          name=" WAIT "
       Case(TASK_STS_READY_TO_RUN)
          name="READY "
       Case(TASK_STS_SCHEDULED)
          name="SCHED "
       Case(TASK_STS_INPROGRESS)
          name=" RUNS "
       Case(TASK_STS_FINISHED)
          name="FINISH "
       Case(TASK_STS_CLEANED)
          name="CLEAN "
       Case(COMM_STS_SEND_INIT )
          name=" SENT "
       Case(COMM_STS_SEND_PROGRESS)
          name=" NOACK "
       Case(COMM_STS_SEND_COMPLETE )
          name="ACKRCV "
       End Select
     End Function task_sts_name



!!$------------------------------------------------------------------------------------------------------------------------

  function mat_name(L,rb,cb) result(mname)
    integer         , intent(in)               :: rb,cb
    character(len=1), intent(in)               :: L
    character(len=MAX_DATA_NAME)               :: mname
    write ( mname,"(A1 A1 I4.4 A1 I4.4)")  L,'_',rb,'_',cb
  end function mat_name
!!$------------------------------------------------------------------------------------------------------------------
  Function to_str(no) Result(str)
        Integer :: no
        Character(len=2)::str
        Write (str,"(I2.2)") no
  End Function to_str


End Module  dist_common
