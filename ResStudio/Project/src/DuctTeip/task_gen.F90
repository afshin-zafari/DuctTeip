#include "debug.h"
module task_generation

  use mpi
  use tgen_types
  use ductteip_lib
  use data_link_list

  implicit none 
  type func_key
     character(len=MAX_TASK_NAME) :: key
  end type func_key
   
  type(task_gen) ,pointer,private:: this
  integer :: sdc=-1,edc,stc=-1,etc,dcf = -1, tcf=-1,dstage_no = 0,tstage_no = 0

contains 
!------------------------------------------------------------------------------------------
  subroutine print_full_list(this)
    type(task_gen) ,pointer,intent(inout):: this
    integer :: i ,j,k,c1,c2,w
    integer , dimension (:) , pointer :: cc,wrk   !comm. cost and work loads of nodes
    real :: m,v
    allocate ( cc ( 1: this%node_cnt ),wrk ( 1: this%node_cnt ))
    write (*,*) "-----------------------------"
    do i=1,this%last_task
       call print_full(this%tlist(i) ) 
    end do
    do i = 0,this%node_cnt-1
       w = sum (this%tlist(1:this%last_task)%work , mask = this%tlist(1:this%last_task)%host == i ) 
       c1 = 0 
       c2 = 0 
       do j=1,this%last_task
          if (this%tlist(j)%host /= i) cycle 
          if ( this%tlist(j)%data_axs(1)%dobj%host /= i) c1 = c1 + 1
          if ( this%tlist(j)%data_axs(2)%dobj%host /= i) c2 = c2 + 1
       end do
       write (*,"(A20 I2 A1 I2 A20 I2 A20 I2)") "Task# for node:",i,"=",count (this%tlist(1:this%last_task)%host == i )&
                   ,"Work Load :" , w &
                   ,"Remote Read-1:" ,c1 &
                   ,"Remote Read-2:" ,c2
       cc(i+1) = c1 + c2
       wrk(i+1) = w
    end do
    do i = 0,this%node_cnt-1
       write (*,*) "Node:",i,"Work:",wrk(i+1) , "Comm.Cost", cc(i+1)
       do j=1,this%last_task
          if ( this%tlist(j)%host == i ) then 
             call print_full(this%tlist(j) ) 
          end if
       end do
    end do
    ! ToDo : host filtering of data for print
!!$    do i = 0,this%node_cnt-1
!!$       do j = 1,ubound(this%vecD,1)
!!$          if (this%vecD(j)%dp%host /= i ) cycle
!!$          call tg_print_data(this%vecD(j)%dp ) 
!!$       end do
!!$    end do
    m = sum ( wrk(:) ) / size(wrk) 
    v = sqrt(sum ( (wrk(:) - m )**2 )/ size(wrk)  )
    write (*,*) "Stat. Total Tasks " , this%last_task
    write (*,*) "  Stat.Summary Work of Node:","Sum:", sum(wrk(:)) , "Avg.",m , "Std.Dev.",v
    m = sum ( cc(:) ) / size(cc) 
    v = sqrt(sum ( (cc(:) - m )**2 )  )
    write (*,*) "  Stat.Summary Comm of Node:","Sum:", sum(cc(:)) , "Avg.",m ,"Std.Dev." , v
    deallocate( cc ,wrk) 
  end subroutine print_full_list

!------------------------------------------------------------------------------------------
  subroutine  taskgen_add(this,T,err) 
    type(task_gen) ,pointer,intent(inout):: this
    type(task) , intent(in ) :: T
    integer , intent(inout) :: err
    this%task_cnt = this%task_cnt  + 1
    if ( .not. dt_register( this,T,this%data_cnt,this%task_cnt)) then 
       if (this%hit_in_region) err = ERR_OUT_OF_REGION
       return
    end if
    this%hit_in_region = .true.
    this%last_task = this%last_task + 1
    if ( this%last_task <= ubound (this%tlist,1) ) then 
       this%tlist(this%last_task) = T
       this%tlist(this%last_task)%executed = .false.
    else
       write (*,*) "Task List exhausted!!!",this%last_task
    end if
  end subroutine taskgen_add
!------------------------------------------------------------------------------------------
   function next_node(this) result ( node) 
     type(task_gen) ,pointer,intent(inout):: this

     integer :: node

     this%cur_node = mod ( (this%cur_node +1) , this%node_cnt ) 
     node = this%cur_node

   end function next_node
!------------------------------------------------------------------------------------------
   subroutine taskgen_finish(this)
     type(task_gen) ,pointer,intent(inout):: this
     deallocate ( this%tlist  ) 
     call dl_destroy(this%dlist)
     deallocate  (this%dlist)

     deallocate ( this%matDT  ) 
     nullify ( this%tlist  )
     nullify ( this%matDT  )
     deallocate ( this  ) 
     nullify ( this  )
     
      
   end subroutine taskgen_finish
!------------------------------------------------------------------------------------------
   function make_dt_engine() result(rma)
     type(remote_access) , pointer :: rma
     rma => remote_access_new(0)
     rma%sync_stage=0
     rma%np  = 100
     rma%nc  = 4
     rma%nd  = 1
     rma%pc  = 1
     rma%node_cnt         = 10
     rma%part_size        = 1
     rma%single_load_sent = .false.
     rma%group_size=10
     rma%data_buf_size = 100*100*8
     call rma_init_this(rma)
  end function make_dt_engine
!------------------------------------------------------------------------------------------
   function taskgen_init(node_cnt) result ( this ) 
     type(task_gen) ,pointer:: this

     integer        , intent(in) :: node_cnt
     type(task_gen) , pointer    :: tg
     integer :: err

     allocate ( this ) 
     this%cur_node    = 0 
     this%last_task   = 0 
     this%task_cnt    = 0 
     this%data_cnt    = 0 
     this%node_cnt    = node_cnt
     allocate ( this%tlist  ( 1:TASK_SIZE_LIMIT ) ) 
     allocate ( this%matDT (1:DATA_SIZE_LIMIT,1:TASK_SIZE_LIMIT) ) 
     this%matDT (:,:) %ver  = -1
     this%matDT (:,:) %axs  = 0
     this%matDT (:,:) %id   = 0
     this%matDT (:,:) %name = ""
     this%didx    = 0 
     this%tidx    = 0 
     this%tlist (:  ) %work = 0
     this%tlist (:  ) %host = UNKNOWN
     !this% rma => make_dt_engine()
     this% grp_size = 1
!     this% grp_rows = 2
!     this% grp_cols = 2
     this%hit_in_region = .false.
     allocate ( this%dlist)
      this%dlist => dl_new_list()
     call MPI_COMM_RANK (MPI_COMM_WORLD,this% my_rank ,err) 
     tg=>this
   end function taskgen_init

!------------------------------------------------------------------------------------------
   function first_avail_axs(T) result (idx)
     type(task) , intent(in):: T
     integer                :: idx
     do idx = 1, ubound(t%data_axs,1) 
        if (T%data_axs(idx)%version <0) then 
           return 
        end if
     end do
   end function first_avail_axs
   
!------------------------------------------------------------------------------------------
   subroutine  print_daxs(D ) 
     type(tgdata_access) , intent(in ) :: D 

     write (*,*) "DAxs , dv" , d%version, "axs",d%axs 
     call tg_print_data(D%dobj)
   end subroutine  print_daxs
!------------------------------------------------------------------------------------------
   subroutine  tg_print_data(D) 
     type(data_object),intent(in ) :: D
     character(len=2) :: mt
      if (D%M%mat_type == MAT_TYPE_ORD) then 
         mt = "M_"
      else
         mt = "X_"
      end if
      write ( *,"(A2 I2.2 A1 I2.2 A1 I2.2 A1 I2.2 A1)")   mt,D%M%by   ,"_",& 
                                                             D%M%bx   ,",",&
                                                             D%version,"@",&
                                                             D%host   ," " 
     
    end subroutine  tg_print_data
!------------------------------------------------------------------------------------------
   subroutine  tg_print_task ( T ) 
     type(task) , intent ( in ) :: T 
     integer :: n 
     n = 0 
     write (*,* ) "Task:",T%name,"th:",T%host,"id:",T%id

     do n = 1,ubound(T%data_axs,1)
        call print_daxs(T%data_axs(n)) 
     end do
     
   end subroutine  tg_print_task
!------------------------------------------------------------------------------------------
   subroutine print_full(T)
     type(task) ,intent(in) :: T
     character(len=14):: tr1,tr2,tw,mt
     integer :: n 
     tr1="   "
     tr2 = "   ------  "
     do n = 1, ubound(T%data_axs,1)
        if (T%data_axs(n)%version >= 0) then 
           if (.not. associated ( T%data_axs(n)%dobj ) ) then 
              cycle 
           end if
           if (T%data_axs(n)%dobj%M%mat_type == MAT_TYPE_ORD) then 
              mt = "M_"
           else
              mt = "X_"
           end if
           if (T%data_axs(n)%axs == READ_AXS) then 
              if ( tr1(1:1) == " ") then 
                 write ( tr1,"(A2 I2.2 A1 I2.2 A1 I2.2 A1 I2.2 A1)")   mt,T%data_axs(n)%dobj%M%by   ,"_",& 
                                                                          T%data_axs(n)%dobj%M%bx   ,",",&
                                                                          T%data_axs(n)%dobj%version,"@",&
                                                                          T%data_axs(n)%dobj%host   ," " 
              else
                 write ( tr2,"(A2 I2.2 A1 I2.2 A1 I2.2 A1 I2.2 A1)")   mt,T%data_axs(n)%dobj%M%by   ,"_",&
                                                                          T%data_axs(n)%dobj%M%bx   ,",",&
                                                                          T%data_axs(n)%dobj%version,"@",&
                                                                          T%data_axs(n)%dobj%host   ," "
              end if
           else
              write ( tw,"(A2 I2.2 A1 I2.2 A1 I2.2 A1 I2.2 A1)")   mt,T%data_axs(n)%dobj%M%by   ,"_",&
                                                                      T%data_axs(n)%dobj%M%bx   ,",",&
                                                                      T%data_axs(n)%dobj%version,"@",&
                                                                      T%data_axs(n)%dobj%host   ,"."
           end if
        end if
     end do
     write (*,*) T%work,T%name," h:",t%host,"R1:",tr1," R2:",tr2," W:",tw

   end subroutine print_full
!------------------------------------------------------------------------------------------
   function get_data_for( this,M ) result(D) 
     type(task_gen) ,pointer,intent(inout):: this
     type(matrix_block )    , intent(in )              :: M
     type(ptr_data_object )                            :: pData
     type(data_object)      , pointer                  :: D     
     character (len=1) :: mn
     integer :: d_idx
     D => get_defined_data_for(this%dlist,M)
     if ( .not. associated(D)   ) then 
        allocate ( D ) 
        pData%dp=> D
        allocate ( pData%dp%M ) 
        pData%dp%M = M
        pData%dp%version = UNKNOWN
        pData%dp%host    = UNKNOWN
        this%data_cnt = this%data_cnt +1
        pData%dp%Id = this%data_cnt
        call dl_add_data(this%dlist,D)
        write (pData%dp%name,"(A1 A1 I2.2 A1 I2.2)")  M%mat_type,"_",M%by,"_",M%bx
     end if 
   end function get_data_for
!------------------------------------------------------------------------------------------
   subroutine taskgen_reads(this,T,M) 
    type(task_gen) ,pointer,intent(inout):: this
     type(matrix_block) , intent(in) :: M
     type(task),pointer , intent(inout) :: T
     type(task),pointer              :: T2
     integer                         :: dv,dh,th,i
     
     th = 0 
     call dt_hosts_ver(this,M,READ_AXS,dh,th,dv)
     i = first_avail_axs(T)
     T2 => T !new_task(T%name,T%work)
     T2 = T 
     T2%data_axs(i)%axs          = READ_AXS
     T2%data_axs(i)%version      = dv
     T2%data_axs(i)%dobj=> get_data_for (this,M)
     T2%data_axs(i)%dobj%version = dv
     T2%data_axs(i)%dobj%host    = dh
     T2%host = th
     
  end subroutine taskgen_reads
!------------------------------------------------------------------------------------------
   
   subroutine  taskgen_writes(this,T,M) 

     type(task_gen) ,pointer,intent(inout):: this
     type(matrix_block) , intent(in) :: M
     type(task),pointer , intent(inout) :: T
     type(task),pointer              :: T2
     integer                         :: dv,dh,th,i

     th = 0 
     call dt_hosts_ver(this,M,WRITE_AXS,dh,th,dv)
     i = first_avail_axs(T)
     T2 => T !new_task(T%name,T%work)
     T2 = T 
     T2%data_axs(i)%axs          = WRITE_AXS
     T2%data_axs(i)%version      = dv
     T2%data_axs(i)%dobj => get_data_for (this,M)
     T2%data_axs(i)%dobj%host    = dh
     T2%host = th

   end subroutine taskgen_writes
!------------------------------------------------------------------------------------------
   function task_reads(T,M) result(T2)

     type(matrix_block) , intent(in) :: M
     type(task),pointer , intent(in) :: T
     type(task),pointer              :: T2
     integer                         :: dv,dh,th,i
     
     th = 0 
     call dt_hosts_ver(this,M,READ_AXS,dh,th,dv)
     i = first_avail_axs(T)
     T2 => new_task(T%name,T%work)
     T2 = T 
     T2%data_axs(i)%axs          = READ_AXS
     T2%data_axs(i)%version      = dv
     T2%data_axs(i)%dobj=> get_data_for (this,M)
     T2%data_axs(i)%dobj%version = dv
     T2%data_axs(i)%dobj%host    = dh
     T2%host = th
     
     
   end function task_reads
!------------------------------------------------------------------------------------------
   
   function task_writes(T,M) result(T2)

     type(matrix_block) , intent(in) :: M
     type(task),pointer , intent(in) :: T
     type(task),pointer              :: T2
     integer                         :: dv,dh,th,i

     th = 0 
     call dt_hosts_ver(this,M,WRITE_AXS,dh,th,dv)
     i = first_avail_axs(T)
     T2 => new_task(T%name,T%work)
     T2 = T 
     T2%data_axs(i)%axs          = WRITE_AXS
     T2%data_axs(i)%version      = dv
     T2%data_axs(i)%dobj => get_data_for (this,M)
     T2%data_axs(i)%dobj%host    = dh
     T2%host = th

   end function task_writes
!------------------------------------------------------------------------------------------
   function new_task (Name,work) result (T) 

     character ( len=*)   , intent(in) :: Name
     type(task)           , pointer    :: T
     integer , intent ( in ) :: work

     allocate ( T) 

     T%Name = Name
     T%work = work
     T%data_axs(:)%version = UNKNOWN
     
   end function new_task
!------------------------------------------------------------------------------------------
  subroutine dt_hosts_ver(this,M,axs,dh,th,dv) 
    type(task_gen) ,pointer,intent(inout):: this
    type(matrix_block)     , intent(in   )            :: M
    integer                , intent(inout)            :: dh,dv,th
    integer                , intent(in   )            :: axs
!    type(ptr_data_object)  , dimension(:,:) , pointer :: dptr
    type(task)                              , pointer :: T
    type(data_object ),pointer :: dpTarget

    dpTarget => get_data_for (this,M)
    dh = dpTarget%host
    dv = dpTarget%version
    if (dh == UNKNOWN ) then 
       if (dv == UNKNOWN) then
          dpTarget%version = 0 
          dv = 0 
          dh = next_node(this)
          dpTarget%host = dh
          th = dh 
       end if
    end if
    if (axs == WRITE_AXS) then 
       dpTarget%version = dpTarget%version +1 
       th = dh 
    end if
    if (th == UNKNOWN ) then 
       th = next_node(this)
    end if
!    TRACEX("dh,th,dv",(dh,th,dv))
  end subroutine dt_hosts_ver
!------------------------------------------------------------------------------------------
  subroutine redirect_tasks(this,M,dv,dh)
    type(task_gen) ,pointer,intent(inout):: this
    integer , intent(in) :: dv,dh
    type(matrix_block),intent(in) ::M
    integer ::i,j
    do i=1,this%last_task
       do j=1,ubound(this%tlist(i)%data_axs,1)
          if (this%tlist(i)%data_axs(j) %axs /= READ_AXS) cycle
          if (this%tlist(i)%data_axs(j)%dobj%M%bx /= M%bx) cycle 
          if (this%tlist(i)%data_axs(j)%dobj%M%by /= M%by) cycle 
          if (this%tlist(i)%data_axs(j)%version /= dv) cycle 
          this%tlist(i)%data_axs(j)%dobj%host= dh          
       end do
    end do
  end subroutine redirect_tasks
!------------------------------------------------------------------------------------------
  function simulate_run(this) result(h) 
    type(task_gen) ,pointer,intent(inout):: this
    integer :: h,i,j,k,Ti,Tn,dv,tv
    logical :: f
    integer , dimension (:),pointer::cr  ! can run 
    type(task) , pointer :: T 
    integer , dimension ( :) , pointer :: wrk,nodes

    allocate ( wrk ( 1:this%node_cnt) ,nodes(1:this%node_cnt)) 
    allocate ( cr(1:this%last_task) ) 
    h = 0 
    Tn = this%last_task
    do while (.true.)
       h = h + Tn
       j = 0
       do Ti=1,Tn
          T=>this%tlist(Ti) 
          if ( T%executed) cycle
          f = .true.
          do k = 1,ubound(T%data_axs,1)
             tv = T%data_axs(k)%version
             if (.not. associated (T%data_axs(k)%dobj)) cycle 
             dv = T%data_axs(k)%dobj%version
             if (T%data_axs(k)%axs == WRITE_AXS ) dv=tv
             f = f .and. (dv == tv)
          end do
          if ( f ) then 
             j = j + 1
             cr(j) = Ti
          end if
       end do
       if (j == 0) exit
       h = h + j
       do i =1,j
          T=>this%tlist(cr(i)) 
          T%executed = .true.
          nodes = minloc (wrk(:)) 
          wrk(nodes(1)) = wrk(nodes(1)) + T%work
          write (*,*) "wrks" , wrk(:)
          T%host = nodes(1) -1
          do K = 1,ubound(T%data_axs,1)
             if (T%data_axs(k)%axs /= WRITE_AXS) cycle 
             T%data_axs(k)%dobj%host = T%host
             T%data_axs(k)%dobj%version = T%data_axs(k)%dobj%version +1
             call redirect_tasks(this,T%data_axs(k)%dobj%M,T%data_axs(k)%dobj%version,T%host)
          end do          
       end do
    end do
    ! ToDo : versions to be returned back to their original value
    deallocate (cr ,wrk,nodes ) 
  end function simulate_run
!------------------------------------------------------------------------------------------
  function my_row(this,r) result (row)
    type(task_gen) ,pointer,intent(inout):: this
    integer , intent(in ) ::r
    integer :: row
    row =  ( r / this%grp_size ) / this%grp_cols
  end function my_row
!------------------------------------------------------------------------------------------
  function my_col(this,r) result (col)
    type(task_gen) ,pointer,intent(inout):: this
    integer , intent(in ) ::r
    integer :: col
    col =  mod ( ( r / this%grp_size ) , this%grp_cols)
  end function my_col
!------------------------------------------------------------------------------------------
  function last_row(this,r) result (row)
    type(task_gen) ,pointer,intent(inout):: this
    integer , intent(in ) ::r
    integer :: row
    row =   this%grp_rows-1
  end function last_row
!------------------------------------------------------------------------------------------
  function last_col(this,r) result (col)
    type(task_gen) ,pointer,intent(inout):: this
    integer , intent(in ) ::r
    integer :: col
    col =   this%grp_cols-1
  end function last_col
!------------------------------------------------------------------------------------------
  subroutine create_task ( this,t_name,rd,wr) 
    type(task_gen) ,pointer,intent(inout):: this
    character(len=*) , intent(in ) ::t_name
    Type(data_access)              :: rd,wr
    type (task_info)               :: t
    type(data_access)   , &
         dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    axs(1) = rd
    axs(2) = wr
    t = rma_create_task ( this%rma , t_name , this%my_rank , axs ,1)
  end subroutine create_task
!------------------------------------------------------------------------------------------
  function READ_ACCESS(this,mat,M,N,row,col,v,h) result(axs)
    type(task_gen) ,pointer,intent(inout):: this
    character(len=5) , intent(in) :: mat
    integer          , intent(in) :: row, col,M,N,v,h
    character(len=11)              :: dn
    type(data_handle)              :: dh
    type(data_access )             :: axs
    write (dn,"(A5 A1 I2.2 A1 I2.2)") mat,"_",row,"_",col
    dh = rma_create_data ( this%rma,dn,M,N,v,h) 
    dh%version = v
    axs = rma_create_access ( this%rma ,  dh , AXS_TYPE_READ  )

  end function READ_ACCESS
!------------------------------------------------------------------------------------------
  function WRITE_ACCESS(this,mat,M,N,row,col,v,h) result(axs)
    type(task_gen) ,pointer,intent(inout):: this
    character(len=5) , intent(in) :: mat
    integer          , intent(in) :: row, col,M,N,v,h
    character(len=11)              :: dn
    type(data_handle)              :: dh
    type(data_access )             :: axs
    write (dn,"(A5 A1 I2.2 A1 I2.2)") mat,"_",row,"_",col 
    dh = rma_create_data ( this%rma,dn,M,N,v,h) 
    dh%version = v
    axs = rma_create_access ( this%rma ,  dh , AXS_TYPE_WRITE  )

  end function WRITE_ACCESS

!------------------------------------------------------------------------------------------
function is_in_my_data_part(this,dc,i) result(in_part)
  type(task_gen) ,pointer,intent(inout):: this
  integer ,intent(in) :: dc,i
  logical :: in_part
  in_part = .false.
  in_part = dc > (dstage_no * this%grp_rows*DATA_SIZE_LIMIT + (i-0) * DATA_SIZE_LIMIT)
  in_part = in_part .and. dc <= (dstage_no * this%grp_rows*DATA_SIZE_LIMIT + (i+1) * DATA_SIZE_LIMIT)
!  TRACEX("in p dc> ",( dc , (dstage_no * this%grp_rows*DATA_SIZE_LIMIT + (i-0) * DATA_SIZE_LIMIT)))
!  TRACEX("in p dc<=",( dc , (dstage_no * this%grp_rows*DATA_SIZE_LIMIT + (i+1) * DATA_SIZE_LIMIT)))
end function is_in_my_data_part
!------------------------------------------------------------------------------------------
function is_in_my_task_part(this,tc,j) result(in_part)
  type(task_gen) ,pointer,intent(inout):: this
  integer ,intent(in) :: tc,j
  logical :: in_part
  in_part = .false.
  in_part = tc > (tstage_no * this%grp_cols*TASK_SIZE_LIMIT + (j-0) * TASK_SIZE_LIMIT)
  in_part = in_part .and. tc <= (tstage_no * this%grp_cols*TASK_SIZE_LIMIT + (j+1) * TASK_SIZE_LIMIT)
!  TRACEX("in p tc> ", (tc , (tstage_no * this%grp_cols*TASK_SIZE_LIMIT + (j-0) * TASK_SIZE_LIMIT)))
!  TRACEX("in p tc<=", (tc , (tstage_no * this%grp_cols*TASK_SIZE_LIMIT + (j+1) * TASK_SIZE_LIMIT)))
end function is_in_my_task_part
  
!------------------------------------------------------------------------------------------
function dt_register(this,T,dc,tc) result(f)
  type(task_gen) ,pointer,intent(inout):: this
  type(task) , intent(in) :: T
  integer , intent(in ) :: dc, tc
  integer :: i,j,k,did,rowidx,colidx
  logical :: f
                       
  if ( mod ( this%my_rank,this%grp_size) /= 0) return 

  i = my_row(this,this%my_rank)
  j = my_col(this,this%my_rank)
  f = .true.
  if ( is_in_my_data_part ( this,dc,i ) ) then 
     if ( sdc == -1 ) sdc = dc
     edc = dc
  else
     f = .false.
  end if
  if ( is_in_my_task_part ( this,tc,j ) ) then 
     if ( stc == -1 ) stc = tc
     etc = tc
  else
     f = .false.
  end if
  if ( f ) then 
     do k = 1,ubound(T%data_axs,1)
        if (T%data_axs(k)%version == UNKNOWN ) cycle         
        did = T%data_axs(k)%dobj%id
        if ( .not. is_in_my_data_part(this,did,i)) then 
           TRACEX("d-id not in part",(did))
           cycle 
        end if        
        rowidx= did - dstage_no*DATA_SIZE_LIMIT*this%grp_rows - i*DATA_SIZE_LIMIT
        if ( rowidx <=0 .or. rowidx >ubound(this%matDT,1) ) then 
           TRACEX("out of range,rowidx",rowidx)
           cycle
        end if
        colidx = tc - tstage_no*this%grp_cols*TASK_SIZE_LIMIT - j*TASK_SIZE_LIMIT
        if ( colidx <=0 .or. colidx >ubound(this%matDT,2) ) then 
           TRACEX("out of range,colidx",colidx)
           cycle
        end if
        TRACEX("New data registered;id,mx,name",(did,rowidx,T%data_axs(k)%dobj%name) ) 
        this%matDT (rowidx,colidx)%ver  = T%data_axs(k)%version
        this%matDT (rowidx,colidx)%axs  = T%data_axs(k)%axs
        this%matDT (rowidx,colidx)%id   = did
        this%matDT (rowidx,colidx)%name = T%data_axs(k)%dobj%name
     end do
  end if
end function dt_register

!------------------------------------------------------------------------------------------
subroutine print_matDT(this)
  type(task_gen) ,pointer,intent(inout):: this
  integer :: i,j,k,id
  character(len=1),dimension(0:2)::axsn
  type(data_object)  :: dp 
  axsn(:)= (/' ','R','W'/)
  do i = 1,ubound(this%matDT,1)
     ! ToDo: i+my_row*ds+dstg*grp_rows*ds
     id = maxval(this%matDT(i,:)%id) 
     write (*,"(A6)",advance='no') "matDT: "
     if ( id /= 0 ) then 
        dp = get_data_for_id(this%dlist,id)
        write (*,"(A7 A3)",advance='no') dp%name," : "
     else
        write (*,"(A7 A3)",advance='no') "       "," : "
     end if
     do j = 1,ubound(this%matDT,2)
        write (*,"(I2.2 A1 A1 A1)",advance='no') this%matDT(i,j)%ver,",",axsn(this%matDT(i,j)%axs)," "
     end do
     write (*,* ) "|"
  end do
end subroutine print_matDT

!------------------------------------------------------------------------------------------
subroutine dt_schedule(this,err,dstg,tstg) 
  type(task_gen) ,pointer,intent(inout):: this
  integer , intent(in) :: err
  integer , intent(inout) :: dstg,tstg
  integer :: i,j,k, r,c,left,ds,ts,me
  character(len=19)              :: dn
  character(len=9) ,parameter :: TNAME_CAN_RUN_LCL ="CanRunLoc"
  character(len=9) ,parameter :: TNAME_CAN_RUN_GLB ="CanRunGlb"
  character(len=9) ,parameter :: TNAME_DISTRIBUTE  ="DistTasks"
  character(len=9) ,parameter :: TNAME_DATA_UPGRADE="DataUpg"
  character(len=9) ,parameter :: TNAME_NEW_DATA    ="NewData"

  character(len=5), parameter :: DNAME_MATRIX_DT    = "mDaTs"
  character(len=5), parameter :: DNAME_VECTOR_TASKS = "vTask"
  character(len=5), parameter :: DNAME_VECTOR_DATA  = "vData"
  character(len=5), parameter :: DNAME_VECTOR_DIST  = "vDist"

#define VECTOR_DIST(a,b)  DNAME_VECTOR_DIST ,1,TASK_SIZE_LIMIT,a,b
#define VECTOR_TASKS(a,b) DNAME_VECTOR_TASKS,1,TASK_SIZE_LIMIT,a,b
#define VECTOR_DATA(a,b)  DNAME_VECTOR_DATA ,DATA_SIZE_LIMIT,1,a,b
#define MATRIX_DT(a,b)    DNAME_MATRIX_DT   ,DATA_SIZE_LIMIT,TASK_SIZE_LIMIT,a,b
  dcf = -1
  tcf = -1
  if ( sdc > 0 .and. stc > 0 ) then 
     TRACEX("matDT: ",("DStg.",dstage_no,"TStg.",tstage_no,DNAME_MATRIX_DT,"(",sdc,"-",edc,",",stc ,"-",etc,")") ) 
     call print_matDT(this) 
     if ( err == 0 ) then 
        TRACEX("final D#,T#",(this%data_cnt,this%task_cnt,DATA_SIZE_LIMIT))
        dstg = this%data_cnt / this%grp_rows / DATA_SIZE_LIMIT
        tstg = this%task_cnt / this%grp_cols / TASK_SIZE_LIMIT
     end if
  end if
  sdc = -1
  stc = -1
  if ( mod ( this%my_rank,this%grp_size) /= 0) return 
  me=this%my_rank
  i = my_row(this,this%my_rank)
  j = my_col(this,this%my_rank)
  r = last_row(this,this%my_rank)
  c = last_col(this,this%my_rank)

  return

  call create_task (this,TNAME_CAN_RUN_LCL, READ_ACCESS(this, MATRIX_DT(i,j),0,me) , WRITE_ACCESS(this,VECTOR_TASKS(i,j),0,me)) 
  if (i == r ) then 
     do k=0,r-1
        call create_task( this,TNAME_CAN_RUN_GLB, READ_ACCESS (this,VECTOR_TASKS(k,j),  1,rank(this,k,j)), &
                                            WRITE_ACCESS (this,VECTOR_TASKS(r,j),k+1,me) ) 
     end do
     call create_task ( this,TNAME_DISTRIBUTE,READ_ACCESS ( this,VECTOR_TASKS(r,j),r+1,me ) , &
                                             WRITE_ACCESS(this,VECTOR_DIST(i,j),0,me))
  end if
  call create_task ( this,TNAME_DATA_UPGRADE,READ_ACCESS (this,VECTOR_TASKS(r,j),r+1,rank(this,r,j)), &
                                             WRITE_ACCESS (this,VECTOR_DATA(i,j),0,me) ) 
  left = mod((j-1)+c+1 , c+1)
  call create_task ( this,TNAME_NEW_DATA, READ_ACCESS (this,VECTOR_DATA(i,left),1,rank(this,i,left)) , &
                                    WRITE_ACCESS (this,MATRIX_DT(i,j),1,me)  ) 
!  call print_lists ( this%rma,.true.)
end subroutine dt_schedule

!------------------------------------------------------------------------------------------
function rank(this,i,j) result(r) 
  type(task_gen) ,pointer,intent(inout):: this
  integer , intent(in) :: i,j
  integer ::r
  r = i*this%grp_cols + j 
end function rank
!------------------------------------------------------------------------------------------
subroutine kernel_can_run_local () 
! if all tasks are executed remove yourself and link your left to your right
end subroutine kernel_can_run_local
!------------------------------------------------------------------------------------------
subroutine kernel_can_run_global () 
end subroutine kernel_can_run_global
!------------------------------------------------------------------------------------------
subroutine kernel_distribute () 
end subroutine kernel_distribute
!------------------------------------------------------------------------------------------
subroutine kernel_data_upgrade () 
end subroutine kernel_data_upgrade
!------------------------------------------------------------------------------------------
subroutine kernel_new_data () 
end subroutine kernel_new_data
!------------------------------------------------------------------------------------------
subroutine set_stages(tg,data_stage,task_stage) 
  type(task_gen) ,pointer,intent(inout):: tg
  integer ,intent(in):: data_stage,task_stage
  dstage_no = data_stage
  tstage_no = task_stage
  this =>tg
end subroutine set_stages
!-------------------------------------------------------------------------------
function block_host(this,M) result(h) 
  type(task_gen) ,pointer,intent(inout):: this
  type(matrix_block) , intent(in):: M
  integer :: h,p,q
  p = mod (M%by-1,this%grp_rows) 
  q = mod (M%bx-1,this%grp_cols) 
  h = p * this%grp_cols + q
end function block_host

!-------------------------------------------------------------------------------
subroutine addtask (this,func,ro,rn,rw,wn,add,an) 
  type(task_gen) ,pointer,intent(inout):: this
  integer,intent(in):: rn,wn,an
  type(func_key)  ,intent(in) ::  func
  type(matrix_block),dimension(:),intent(in)::ro,rw,add
  character(len=17)              :: dn
  type(data_handle)              :: dh
  type(data_access ) ,dimension(:),pointer  :: axs
  integer :: i,j ,N,h,n,hd,ht,v
  type(matrix_block ) :: M
  type(task_info) :: dt
  type(task),pointer::T
  T=>new_task(func%key,1)
  n = rn+wn+an !ubound(ro,1) + ubound(rw,1) + ubound(add,1)
  allocate ( axs ( 1:max(n,MAX_DATA_AXS_IN_TASK) ) ) 
  j = 1
  do i = 1,rn
     M = ro(i)
     write (dn,"(A8 A1 I3.3 A1 I3.3)") M%mat_type,"_",M%by,"_",M%bx 
     N = M%w
     h = block_host(this,M)
     call dt_hosts_ver(this,M,READ_AXS,hd,ht,v)
     dh = rma_create_data ( this%rma,dn,N,N,v,h) 
     dh%version = v
     axs(j) = rma_create_access ( this%rma ,  dh , AXS_TYPE_READ  )
     j = j + 1
  end do


  do i = 1,wn
     M = rw(i)
     write (dn,"(A8 A1 I3.3 A1 I3.3)") M%mat_type,"_",M%by,"_",M%bx 
     N = M%w
     h = block_host(this,M)
     call dt_hosts_ver(this,M,WRITE_AXS,hd,ht,v)
     dh = rma_create_data ( this%rma,dn,N,N,v,h) 
     dh%version = v
     axs(j) = rma_create_access ( this%rma ,  dh , AXS_TYPE_WRITE  )
     j = j + 1
  end do
  do i=j,ubound(axs,1) 
     axs(i)%dname=""
  end do
  dt = rma_create_task ( this%rma , func%key , h , axs,wght = 1 )
  deallocate ( axs ,T ) 
end subroutine addtask

end module task_generation
