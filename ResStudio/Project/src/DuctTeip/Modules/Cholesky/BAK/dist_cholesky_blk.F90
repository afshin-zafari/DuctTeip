
#include "debug.h" 
!#define TIMING(a,b,c,d) call instrument2(a,b,c,d)
#define TIMING(a,b,c,d) 


#define KERN(a) a 
#define TASK(a) 

#define DIAG(a) a 
#define PNLU(a)  
#define MMUL(a) 
#define XADD(a)  
#define MADD(a)   

#define HANDLE(a) 
#define DRESULT(a) 
#define DRANGE(a) 

#define TRENTER(a)  
#define TREXIT(a) 


#define SEQ(a) 

!#define SG_DEBUG
#ifdef SG_DEBUG
#define SG_ADD_TASK call tl_add_task_debug
#define FUNC_NAME(a) this,#a
#else
#define SG_ADD_TASK call tl_add_task_named
#define FUNC_NAME(a) a
#endif

module cholesky

!  use tl
!  use dm_class
!  use fp
!  use timer
!  use dist_common


!  use dist_types
!  use dist_data_class
#ifdef LINUX
 use iso_c_binding
#define C_LOC(a) c_loc(a)
#else
#define C_LOC(a) a
#endif
  use ductteip_lib
  use cholesky_taskgen

  implicit none

#ifdef LINUX  
!GCC$ ATTRIBUTES DLLEXPORT :: hReads ,cptr_hreads
!  type(tl_handle) , dimension( 1:5),target ,bind(c),save:: hReads 
  type(C_PTR),bind(c) , save ::cptr_hreads
  
#endif 
  

  integer , parameter ::  AREA_Y1 = 1, AREA_Y2=2 , AREA_X1 = 3,AREA_X2=4
  integer , parameter :: DIAG=0,PNLU = 1, MMUL = 2, SUBT=3, GEMM=MMUL
  character(len=9),parameter :: CHOL_CONTEXT=" Cholesky"

  integer , parameter  :: NAME_LEN  = 20
  integer , parameter  :: TASK_MADD = 1
  integer , parameter  :: TASK_DIAG = 2
  integer , parameter  :: TASK_PNLU = 3
  integer , parameter  :: TASK_MMUL = 4
  integer , parameter  :: TASK_XADD = 5

  Character(len=*),Parameter :: TNAME_ASM_MAT   = "AsmblMat"
  Character(len=*),Parameter :: TNAME_CALC_DIST = "CalcDist"
  Character(len=*),Parameter :: TNAME_CHOL_DIAG = "Diag"
  Character(len=*),Parameter :: TNAME_CHOL_PNLU = "PnlU"
  Character(len=*),Parameter :: TNAME_CHOL_XADD = "XAdd"
  Character(len=*),Parameter :: TNAME_CHOL_MMUL = "MMul"
  Character(len=*),Parameter :: TNAME_CHOL_MADD = "MAdd"
  Character(len=*),Parameter :: TNAME_CHOL_SUBT = "SubT"
  Character(len=*),Parameter :: TNAME_CHOL_SYNC = "Init"
  Character(len=*),Parameter :: TNAME_INIT_DATA = "Init"

  character(len=NAME_LEN) , parameter  :: TASK_XADD_NAME = "XADD"
  character(len=NAME_LEN) , parameter  :: TASK_MMUL_NAME = "MMUL"//char(0)
  character(len=NAME_LEN) , parameter  :: TASK_PNLU_NAME = "PNLU"//char(0)
  character(len=NAME_LEN) , parameter  :: TASK_DIAG_NAME = "DIAG"//char(0)
  character(len=NAME_LEN) , parameter  :: TASK_MADD_NAME = "MADD"

  type area_type
     integer , dimension(1:4) ::pos
  end type area_type
  type chol_args
     character(len=NAME_LEN)    :: TName
     type(dist_chol) , pointer  :: this
     integer                    :: rb,cb,bidx
     logical                    :: Btranspose
     real(kind=rfp)             :: alpha,beta
     type(tl_handle) , dimension( 3 ) :: hList
     type(area_type) , dimension(1:3) :: areas
!     real(kind=rfp)  , dimension( :,:)    , pointer :: data1,data2,data3
     
  end type chol_args

  type dist_chol
     character(len=NAME_LEN)                                 :: TName
     character(len=NAME_LEN) , dimension ( 1:5)              :: Writes
     real(kind=rfp)          , dimension ( :,:)    , pointer :: data1=>NULL(),data2=>NULL(),data3=>NULL()
     type(dm)                                      , pointer :: Dmngr
     character(len=16) :: d1name,d2name,d3name
     integer                                                 :: nb,np,rows,cols,dm_blk_size,dm_blk_cnt
     type(remote_access) ,pointer:: rma
  end type dist_chol


contains 
!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_task_sync(arg)
    Type(chol_args) ,Intent(inout):: arg

    
    call dm_set_sync(arg%this%dmngr,1)
    TRACEX("CHOL_SYNC",arg%this%TName)

  End Subroutine chol_task_sync
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_diag_upd_local(arg)
    type(chol_args), intent(inout)               :: arg
    integer                                      :: col,r,fr,tr,fc,tc
    real(kind=rfp) , dimension ( :,: ) , pointer :: A

    A => arg%this%data1
    tr = arg%areas(1)%pos(AREA_Y2)
    fc = arg%areas(1)%pos(AREA_X1)
    tc = arg%areas(1)%pos(AREA_X2)

    TRACEX("Diag Enter",(arg%areas(1)))

    If ( .Not. Associated ( A)  ) Then 
       KERN(DIAG(TRACEX("CHOL_KERN,Diag. A  associated",Associated (A))))
       Return 
    End If

    TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,arg%bidx) 
    do col=fc,tc
       if ( col >fc ) then 
          do r = col,tr             
             A(r,col) = A(r,col) - dot_product( A(r,fc:col-1),A(col,fc:col-1) ) 
          end do
       end if
       fr=col+1
       A(col,col) = sqrt ( abs(A(col,col)))    
       A(fr:tr,col) = A(fr:tr,col) / A(col,col)
    end do
    TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,arg%bidx) 
    call chol_print_data ( A, 3,3 ) 
    KERN(DIAG(TRACEX( "CHOL_KERN,Diag.","Exit")))
  end subroutine chol_diag_upd_local
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_pnlu_local(arg)
    Type(chol_args), Intent(inout)               :: arg
    Real(kind=rfp) , Dimension ( :,: ) , Pointer :: A,B
    integer                                      :: col,r,fr,tr,fc,tc

    
    A => arg%this%data1
    B => arg%this%data2 
    fr = arg%areas(2)%pos(AREA_Y1)
    tr = arg%areas(2)%pos(AREA_Y2)
    fc = arg%areas(1)%pos(AREA_X1)
    tc = arg%areas(1)%pos(AREA_X2)


    TRACEX( "CHOL_KERN,PnlU.","Enter")
    call chol_print_data(A,3,3)
    TRACEX(" B----",B(1,1))
    call chol_print_data(B,3,3)
    TRACEX(" area1",(arg%areas(1)))
    TRACEX(" area2",(arg%areas(2)))
    if ( .not. associated (A)  ) then 
       TRACEX("zzz",(arg%this%d1name,associated(arg%this%rma%data_list(1)%mat)))
       return 
    end if
    if ( .not. associated (B)  )  then 
       TRACEX("zzz2",(arg%this%d2name))
       return 
    end if

    TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,arg%bidx) 
    do col=fc,tc
       if ( col >fc ) then 
          do r = fr,tr             
             B(r,col) = B(r,col) - dot_product( B(r,fc:col-1),A(col,fc:col-1) ) 
          end do
       end if
       B(fr:tr,col) = B(fr:tr,col)/ A(col,col)
    end do
    TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,arg%bidx) 
    call chol_print_data ( B, 3,3 ) 
    TRACEX( "CHOL_KERN,PnlU.","Exit")
  end subroutine chol_mat_pnlu_local
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_mul_local(arg)
    type(chol_args), intent(inout)             :: arg
    real(kind=rfp) , dimension(:,:) , pointer  :: A,B,C
#define matblock(a)  arg%areas(a)%pos(AREA_Y1): arg%areas(a)%pos(AREA_Y2), &
                     arg%areas(a)%pos(AREA_X1): arg%areas(a)%pos(AREA_X2)
#define A_BLOCK A(matblock(1))
#define B_BLOCK B(matblock(2))
#define C_BLOCK C(matblock(3))

    A => arg%this%data1
    B => arg%this%data2
    C => arg%this%data3
    TRACEX("gemm_task Enter :  A,B,C",(arg%this%d1name,arg%this%d2name,arg%this%d3name))

    If ( .Not. Associated (A) ) Return 
    if ( .not. associated (B) ) return 
    If ( .Not. Associated (C) ) Return 
    TRACEX("mmul area1",(arg%areas(1)))
    TRACEX("mmul area2",(arg%areas(2)))
    TRACEX("zmmul area3",(arg%areas(3)))
    TRACEX("alpha , beta",(arg%alpha, arg%beta))
    call chol_print_data( C ,3,3) 
    TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,arg%bidx) 
    if ( arg%Btranspose ) then 
       C_BLOCK= arg%beta * C_BLOCK + arg%alpha* matmul(A_BLOCK ,transpose(B_BLOCK) )
    else 
       C_BLOCK= arg%beta * C_BLOCK + arg%alpha* matmul(A_BLOCK ,          B_BLOCK  )
    end if
    TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,arg%bidx) 
    call chol_print_data( C ,3,3) 
    TRACEX("CHOL_KERN,MMul.","Exit")
#undef matblock              
#undef A_BLOCK 
#undef B_BLOCK 
#undef C_BLOCK 


  end subroutine chol_mat_mul_local
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_subt_local(arg)
    type(chol_args), intent(inout)             :: arg
    real(kind=rfp) , dimension(:,:) , pointer  :: A,B,C
#define matblock(a)  arg%areas(a)%pos(AREA_Y1): arg%areas(a)%pos(AREA_Y2), &
                     arg%areas(a)%pos(AREA_X1): arg%areas(a)%pos(AREA_X2)
#define A_BLOCK A(matblock(1))
#define B_BLOCK B(matblock(2))
#define C_BLOCK C(matblock(3))


    A => arg%this%data1
    B => arg%this%data2
    C => arg%this%data3
    
    If ( .Not. Associated (A) ) Return 
    If ( .Not. Associated (B) ) Return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,arg%bidx) 
    C_BLOCK = B_BLOCK - A_BLOCK 
    TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,arg%bidx) 

    TRACEX("CHOL_KERN,MAdd.","Exit")
#undef matblock              
#undef A_BLOCK 
#undef B_BLOCK 
#undef C_BLOCK 


  end subroutine chol_subt_local
!!$------------------------------------------------------------------------------------------------------------------------
  function  dist_chol_new(nb,tname) result (this)
    type(dist_chol)         , pointer    :: this
    integer                 , intent(in) :: nb
    character(len=*) , intent(in) :: tname
    
    allocate(this ) 
    this%nb = nb
    this%tname = tname
    this%writes(TASK_XADD) = TASK_XADD_NAME
    this%writes(TASK_MADD) = TASK_MADD_NAME
    this%writes(TASK_DIAG) = TASK_DIAG_NAME
    this%writes(TASK_PNLU) = TASK_PNLU_NAME
    this%writes(TASK_MMUL) = TASK_MMUL_NAME
  end function dist_chol_new
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_sequential(mat)
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(in):: mat
    integer                                      :: col,row,fr,tr,tc,fc
    real(kind=rfp) , dimension ( :,: ) , pointer :: A,C,B
    real(kind=rfp)                               :: div,sm
    
    A => mat
    C => mat
    if ( .not. associated ( mat)  ) then 
       TRACEX("CHOL_SEQ,Diag. mat  associated",associated (mat))
       return 
    end if
    tr = ubound(mat,1)
    tc = ubound(mat,2)
    fc = lbound(mat,2)
    
    do col=fc,tc
       if ( col >1 ) then 
          do row = col,tr
             sm = sum(  A(row,1:col-1)*A(col,1:col-1)  )
             C(row,col) = A(row,col) - sm              
          end do
       end if
       fr = col+1

       if ( mod(col,100) ==0 ) write (*,*) 'Running' , col,getime()

       C(col,col) = sqrt ( abs(C(col,col)))    
       C(fr:tr,col) = A(fr:tr,col) / C(col,col)    
    end do

  end subroutine chol_sequential
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_sequential_block(mat ,nb) 
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(in):: mat
!    real(kind=rfp) , dimension ( :,: ) , pointer :: A
    integer , intent(in) :: nb 
!    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
    type(chol_args) :: arg
    integer :: nrows,ncols ! number of columns and rows 
    integer :: rb   ,cb    ! number of rows or columns in block 
    integer :: rbno ,cbno  ! total number of blocks in rows or columns
    integer :: rbidx,cbidx ! index for row and column blocks 
    integer :: fr,tr,fc,tc ! range of rows and column in blocks 
    integer :: fr2,tr2     ! row range for panel updating
    integer :: fc2,tc2     ! column range for row updating
    integer :: r,c         ! index for row and columns in blocks
    integer :: seq,cbidx2


    if ( .not. associated ( mat ) ) then 
       TRACEX("Wrong Mat Input",associated ( mat ) )
       return
    end if
    
 
    nrows=ubound(mat,1)
    ncols=ubound(mat,2)
    rb= nrows / nb
    cb= ncols / nb 
    if ( ncols < 16 ) then 
       rbno = 4 
       cbno = 4
       rb = nrows / rbno
       cb = ncols / cbno
    end if

    !allocate ( Xmat ( 1:nrows,1:ncols) ) 
    !Xmat(:,:) = 0

    rbno = nrows / rb + 1
    cbno = ncols / cb + 1
    TRACEX("SeqBlock. BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    TIMING(EVENT_SUBTASK_STARTED,"TGEN",5,0)
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          fr =     (rbidx-1) * rb+1
          tr = min( rbidx    * rb  ,nrows)
          fc =     (cbidx-1) * cb+1
          tc = min( cbidx    * cb  ,ncols)
          if ( fr > ubound(mat,1) ) exit
          if ( fc > ubound(mat,2) ) exit

             ! gemm tasks
             fr2 = (cbidx-1)*rb +1 
             tr2 = min((cbidx)*rb , nrows)
             SEQ(TRACEX("SeqBlock. gemm update ",(fr2,tr2)))
             do cbidx2=1,cbidx-1
                fc2 = (cbidx2-1)   *cb + 1
                tc2 = (cbidx2  )*cb
                if ( fc2 > ubound(mat,2) ) exit 
                TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
                mat(fr:tr,fc:tc) = mat(fr:tr,fc:tc)-matmul(  mat (fr:tr,fc2:tc2) , transpose(mat (fr2:tr2,fc2:tc2)) )
                TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
                !TRACEX("SEQ.MUL",(cbidx,rbidx,fr,tr,fc,tc,'-',fr2,tr2,fc2,tc2))
				    !call chol_print_data( mat,rb,cb ) 

             end do
          
          SEQ(TRACEX("SeqBlock. Loop cb,rb, ranges",(cbidx,rbidx,fr,tr,fc,tc)))
          if ( rbidx > cbidx ) then 
             ! panel update
             SEQ(TRACEX("SeqBlock. Panel update",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
             do c = fc,tc
                if ( c > fc ) then 
                   do r=fr,tr
                      !SEQ(TRACEX("SeqBlock. Panel update inner loop ",(r,c)))
                      !SEQ(TRACEX("SeqBlock.",("mat(", c,',',fc,':',c-1,")*mat(",r,',',fc,':',c-1,"))")))
                      !SEQ(TRACEX("SeqBlock.",(sum(mat( c,fc:c-1)*mat(r,fc:c-1)))))
                      mat(r,c) = mat(r,c) - sum(mat( c,fc:c-1)*mat(r,fc:c-1))
                   end do
                end if
                mat ( fr:tr,c) =  mat ( fr:tr,c) / mat ( c,c)                   
             end do
             !TRACEX("SEQ","PNL")
             !call chol_print_data( mat,rb,cb ) 
             TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
          end if
          if (rbidx == cbidx) then 
             ! diagonal block 
             SEQ(TRACEX("SeqBlock. Diagonal update",(fc,tc)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             do c=fc,tc
                if ( c >1 ) then 
                   do r = c,tr
                      !SEQ(TRACEX("SeqBlock. Diagonal update inner loop ",(r,c)))
                      mat(r,c) = mat (r,c) - sum(  mat(r,fc:c-1)*mat(c,fc:c-1)  )              
                   end do
                end if
                fr = c+1
                mat(c,c) = sqrt ( abs(mat(c,c)))    
                mat(fr:tr,c) = mat(fr:tr,c) / mat(c,c)    
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             !TRACEX("SEQ","DIAG")
             !call chol_print_data( A,rb,cb ) 
          end if 
       end do ! row block idx
    end do ! col block idx

    TIMING(EVENT_SUBTASK_FINISHED,"TGEN",5,0)

  end subroutine chol_sequential_block
!!$------------------------------------------------------------------------------------------------------------------------

  function chol_get_handle(this,n,r,c,create) result(hnd)
    type(dist_chol) , intent(inout) , pointer  :: this
    integer         , intent(in)               :: r,c
    character(len=1), intent(in)               :: n
    logical         , intent(in)    , optional :: create
    character(len=MAX_DATA_NAME)               :: hname
    type(tl_handle)                            :: hnd
    integer :: fr,tr,fc,tc

    write ( hname,"(A4 A1 A1 A1 I4.4 A1 I4.4)")  this%writes(TASK_DIAG),'_',n,'_',r,'_',c
    fr = this%nb*(r-1)+1
    tr = this%nb*(r  )
    fc = this%nb*(c-1)+1
    tc = this%nb*(c )
    if ( present(create) .and. create  ) then 
       call  dm_matrix_add ( this%dmngr, hname)
       hnd = dm_handle_get ( this%dmngr, hname)
       TASK(HANDLE(TRACEX(" Handle Name +", (hname ,hnd,fr,':',tr,'-',fc,':',tc)) ))
       return 
    end if
    hnd = dm_handle_get(this%dmngr,hname)
    TASK(HANDLE(TRACEX(" Handle Name  ", (hname ,hnd,fr,':',tr,'-',fc,':',tc)) ))
    
  end function chol_get_handle

!!$------------------------------------------------------------------------------------------------------------------
  Subroutine check_chol_result(this)
    Type(remote_access) , Intent(in) :: this
    Integer :: i,n ,r,c,col,x,v,row,seq
    Character(len=MAX_DATA_NAME) :: dname
    character(len=1)  :: sep
    character(len=9)  :: ctx
    Character(len=8) :: obj
    Real(kind=rfp) :: sm,trc,err,tr_err

    seq = opt_get_option(this%opt,OPT_SEQUENTIAL)

    Do i = 1,Ubound(this%data_list,1)
       !If ( this%data_list(i)%id == DATA_INVALID_ID ) Cycle
       dname = this%data_list(i)%name
!       Read(dname,"(A1 A1 I2 A1 I2 A1 I2 A1 I2)") obj,sep,r,sep,c,sep,x,sep,v
       read(dname,"(A8 A1 I3 A1 I3)") obj,sep,r,sep,c
       TRACEX("chol result",('-',ctx,'-',obj,r,c,'-',obj(1:1),'-') ) 
       If ( obj(1:1) /= 'M' ) Cycle
       !If ( v== 0 .and. seq == 0) Cycle
       n = Ubound(this%data_list(i)%mat,2)
       If ( r /= c ) Then 
          sm = -Sum ( this%data_list(i)%mat(:,:))
          err = sm -n*n  
          tr_err = 0 
          trc=0
          If ( err /= 0.0 ) Then 
             write(*,*)  "CheckCholResult-Sum,Error"," : ",dname(1:13), sm, err
          Else
             write(*,*)  "CheckCholResult-Sum"," : ",dname(1:13), sm, err
          End If
       Else          
          sm = 0.0
          trc = 0.0
          Do col = 1, n
             sm = sm + Sum(this%data_list(i)%mat(col+1:,col))
             trc = trc + this%data_list(i)%mat(col,col)
          End Do          
          err = sm + n*(n-1)/2.0
          tr_err = trc - n 
          If ( err /= 0.0 ) Then 
             write(*,*)  "CheckCholResult-Sum,Error"," : ",dname(1:13), sm, err
          Else
             write(*,*)  "CheckCholResult-Sum   "," : ",dname(1:13), sm, err
          End If
          If ( tr_err /= 0.0 ) Then 
             write(*,*)  "CheckCholResult-Trace,Error"," : ",dname(1:13), trc , tr_err
          Else
             write(*,*)  "CheckCholResult-Trace "," : ",dname(1:13), trc , tr_err
          End If
       End If
    End Do
  End Subroutine check_chol_result
!!$-------------------------------------------------------------------------------------------------------------------------
  subroutine chol_print_data ( mat,rb,cb )
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(in):: mat
    integer , intent(in) :: rb,cb
    integer :: r,c
    
    
    if ( cb > 0 ) then 
       if ( ubound(mat,1) > 13 ) return 
       if ( ubound(mat,2) > 13 ) return 
    end if

    !TRACEX("CHOL_PRINT",(ubound(mat,1),ubound(mat,2),rb,cb))
    do r = 1,ubound(mat,1)
       write ( *,"(A)",advance='no') "CholData:"
       do c = 1,ubound(mat,2)
          write ( *,"(f3)",advance='no') mat(r,c)
          if (mod(c,cb ) ==0 )  write ( *,"(A)",advance='no') "|" 
       end do
       write (*,*) ""
!!$       TRACEX("mat",mat(r,:))
       if ( mod ( r,rb) == 0 ) then 
          write (*,*) "CholData:----------------------------------------------------------------------|"
       end if
    end do
    
  end subroutine chol_print_data
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine data_partition_matrix(this,nb)
    type(data_handle) ,intent(inout) :: this
    integer , intent(in) :: nb
    integer :: rb,cb,brows,bcols,k,l
    type(matrix),pointer:: mat
    integer , dimension(1:4) :: rg
    brows = ubound(this%mat,1)/nb
    bcols = ubound(this%mat,2)/nb
    TRACEX("+++++Matrix Size,r,c",(brows,bcols,size(this%mat)))
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
!!$-------------------------------------------------------------------------------------------------------------------------
  Subroutine print_chol_data(this)
    Type(remote_access)	           :: this
    Integer 		           :: i,j,k,r,c
    Character(len=10)               :: prefix,sep
    Character(len=MAX_DATA_NAME)   :: dname
    Real(kind=rfp) , Dimension(:,:) , pointer :: mat

    Write (*,*) "**********************************Cholesky Data ************************************"
    Do i = 1, Ubound(this%data_list,1)
       If ( this%data_list(i)%id == DATA_INVALID_ID ) Cycle
       If ( Ubound(this%data_list(i)%mat,1) > 12 ) Return 
       dname=this%data_list(i)%name       
       write(*,*)  "Chol Data"," : ",dname
       TRACEX("azaz",(dname))
       Read ( dname,"(A9 I3 A1 I3)") prefix,r,sep,c
       !If ( r /= this%proc_id ) Cycle
       If ( prefix(1:1) /= 'M') Cycle 
       Write (*,*) "Data: ",dname,"Id:",this%data_list(i)%id,i
       If (this%data_list(i)%id ==DATA_INVALID_ID)  Cycle
       mat =>this%data_list(i)%mat
       Do j = 1,Ubound(mat,1)
          Do k = 1,Ubound(mat,2)
             !Write ( *,"(F4.4)",advance='no') this%data_list(i)%mat(j,k) 
          End Do
          !write ( *,*) this%data_list(i)%mat(j,:) 
          Write (*,*) ""
!          TRACEX("data ",this%data_list(i)%mat(j,:))
       End Do   
    End Do
    Write (*,*) "*******************************************************************************"

  End Subroutine print_chol_data

!!$------------------------------------------------------------------------------------------------------------
  function task_chol_local(this,task) result(res)
    type(remote_access) , intent(inout) ,target   :: this
    type(task_info)     , Intent(in)       :: task
    integer :: res,notify,e=EVENT_TASK_STARTED
    Type(dist_chol )    , Pointer          :: chol_obj
    Character(len=MAX_DATA_NAME)           :: dname
    Type(data_handle)                      :: r1Data,r2Data,wrData

    TRACEX("In the chol task run",(task%name,task%id))
    res = 0
    chol_obj => dist_chol_new(this%pc,task%name)
    chol_obj%dm_blk_cnt = this%dm_blk_cnt
    chol_obj%dm_blk_size = this%dm_blk_size
    chol_obj%rma => this
    Allocate(this%task_list(task%id)%dmngr)
    Call dm_new(this%task_list(task%id)%dmngr)
    Call dm_set_taskid(this%task_list(task%id)%dmngr,task%id)



    ! Rows and Cols
    chol_obj%rows = this%np /chol_obj%dm_blk_cnt
    chol_obj%cols = chol_obj%rows

!    call print_lists ( this, .true. ) 
    !  Write Data
    dname = task_write_name(this,task%id)
    wrdata = data_find_by_name_dbg(this,dname,"dist_task_class.F90",751)
    TRACEX("wr data",(wrdata%id,wrdata%nrows, wrdata%ncols,this%data_list(wrdata%id)%name))
    if ( .not. associated ( this%data_list(wrdata%id)%mat ) ) then 
       TRACEX("wr data ! assoc",dname)
       allocate( this%data_list(wrdata%id)%mat(1:wrdata%nrows,1:wrdata%ncols) )
    end if
    chol_obj%data3 => this%data_list(wrdata%id)%mat
    chol_obj%d3name = dname
    TRACEX("wr data ",dname)
    allocate ( this%data_list(wrdata%id)%dmngr )
    call dm_new(this%data_list(wrdata%id)%dmngr  ) 
    !  Read1 Data
    dname =this%task_list(task%id)%axs_list(1)%data%name
    chol_obj%d1name = dname
    r1Data = data_find_by_name_dbg(this,dname,"dist_task_class.F90",761)
    chol_obj%data1 => this%data_list(r1data%id)%mat

    allocate ( this%data_list(r1data%id)%dmngr )
    call dm_new(this%data_list(r1data%id)%dmngr  ) 
    
    
    TRACEX("zzz22",(task%name))

    !  Read2 Data
    TRACEX("zzz3",(dname))
    dname =this%task_list(task%id)%axs_list(2)%data%name
    chol_obj%d2name = dname
    r2Data = data_find_by_name_dbg(this,dname,"dist_task_class.F90",769)
    chol_obj%data2 => this%data_list(r2data%id)%mat
    allocate ( this%data_list(r2data%id)%dmngr )
    call dm_new(this%data_list(r2data%id)%dmngr  ) 
       

    chol_obj%dmngr => this%task_list(task%id)%dmngr
    select case ( task%name)
        case (TNAME_CHOL_SUBT) 
           chol_obj%nb = this%nc
           call tgen_subt_shmem(chol_obj) 
        case (TNAME_CHOL_PNLU) 
           chol_obj%data1 => this%data_list(r1data%id)%mat
           chol_obj%data2 => this%data_list(r2data%id)%mat
           chol_obj%data3 => this%data_list(r2data%id)%mat
           this%data_list(wrdata%id)%mat =>this%data_list(r2data%id)%mat
           chol_obj%nb = this%nc
           call tgen_pnl_shmem(chol_obj)
        case (TNAME_CHOL_DIAG) 
           chol_obj%data2 => this%data_list(r1data%id)%mat
           chol_obj%data3 => this%data_list(r1data%id)%mat
           this%data_list(wrdata%id)%mat =>this%data_list(r1data%id)%mat
           chol_obj%nb = this%nc
           call tgen_diag_shmem ( chol_obj )
           TRACEX("dmngr sync",dm_get_sync(this%task_list(task%id)%dmngr))
        case (TNAME_CHOL_MMUL) 
           chol_obj%data3(:,:) = 0
           chol_obj%nb = this%nc
           TRACEX("gemm A,B,C",(chol_obj%d1name,chol_obj%d2name,chol_obj%d3name))
           call tgen_gemm_shmem(chol_obj) 
        end select

    
  end function  task_chol_local
!-------------------------------------------------------------------------------

  function  task_write_name(this,taskid) result(dname)
    Type(remote_access) , Intent(inout) :: this
    Integer , intent(in) :: taskid
    Character(len=MAX_DATA_NAME)  :: dname
    integer :: i

    dname="!"
    Do  i = 1,Ubound(this%task_list(taskid)%axs_list,1 )
       If ( this%task_list(taskid)%axs_list(i)%access_types == AXS_TYPE_WRITE) Then 
          dname = this%task_list(taskid)%axs_list(i)%dname
          Return
       End If
    End Do
    
  end function  task_write_name
!-----------------------------------------
  subroutine chol_init_data(this) 
    Type(remote_access)	            :: this
    Integer 		            :: r,c
    Character(len=16)               :: dname
    Type(data_handle)		    :: d
    Type(point_set)                 :: pts
    integer :: blk_cnt,p,q,h
    blk_cnt = opt_get_option(this%opt,OPT_DIST_BLK_CNT)
    do r = 1,blk_cnt
       do c = 1, r
          write ( dname, "(A9 I3.3 A1 I3.3)")  "MainMatM_",r,"_",c
          TRACE_LOC
          p = mod (r-1,this%grp_rows) 
          q = mod (c-1,this%grp_cols) 
          h = p * this%grp_cols + q
          if (h /= this%proc_id) cycle 
          d = rma_create_data(this,dname,this%np/blk_cnt ,this%np/blk_cnt,0,this%proc_id)
          TRACEX("populate name,NxN",(this%np,blk_cnt,dname,this%np/blk_cnt))
          call data_populate (this,dname,pts) 
       end do
    end do

  end subroutine chol_init_data
!-----------------------------------------
  subroutine rma_init_chol_data(this) 
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
          dname = "M_"//to_str(r)//"_"//to_str(c)
          d = rma_create_data(this,dname,this%np ,this%np,0,this%proc_id)
          
          call data_populate (this,dname,pts) 
       end do
    end do

  end subroutine rma_init_chol_data

!!$------------------------------------------------------------------------------------------------------------------
  function dt_init(dt_cfg) result(res)

    Integer          :: pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt
    Type(options)    :: opt
    type(remote_access) ,intent(inout)         :: dt_cfg
    Integer 		         :: cyc = 0,tc,err,event=EVENT_DONT_CARE,ids_element,lnp,lnd,lnt,blk_cnt
    Type(data_handle)            :: Data
    Type ( point_set)            :: pts
    Character(len=5)             :: env
    Character(len=MAX_DATA_NAME) :: dname
    Integer                      :: part_idx = 1 ,time_out,node_owns_data,i,j=1,bcast_mtd,seq,import_tasks,lbm,tasks_node,cholesky
    Integer(kind=8)              :: task_dst
    integer(kind=8)              :: time,drdy,dlag,dnow
    Real(kind=rfp)::start
    Type(task_info)::task
    Type(listener)::lsnr
    Logical :: populated = .False.
    character(len=STR_MAX_LEN)::tasks_file

    integer :: res
    seq = opt_get_option(dt_cfg%opt,OPT_SEQUENTIAL)
    res=22
    if ( seq /= 0) then 
       TRACEX("Init Cholesky Data",0)
       dt_cfg%grp_rows = 1
       dt_cfg%grp_cols = 1
       call chol_init_data(dt_cfg)
       call print_chol_data(dt_cfg)

       TRACEX("Init Cholesky Data",1)
       call instrument2(EVENT_DTLIB_EXEC,"DRUN",0,0)
       data=data_find_by_name_dbg(dt_cfg,"M_001_001","",0)
       if ( seq == 1 ) then 
          call chol_sequential(dt_cfg%data_list(data%id)%mat)
       elseif ( seq == 2 ) then 
          call chol_sequential_block(dt_cfg%data_list(data%id)%mat,dt_cfg%nc)
       end if
       call instrument2(EVENT_DTLIB_EXEC,"DRUN",0,0)
       call print_chol_data(dt_cfg)
       call check_chol_result(dt_cfg)
       return
    end if
    allocate ( dt_cfg%data_buf  (1:dt_cfg%np*dt_cfg%np) ) 
    allocate ( dt_cfg%data_buf2 (1:dt_cfg%np*dt_cfg%np) ) 
    if ( dt_cfg%proc_id ==0 ) then 
       blk_cnt = opt_get_option(dt_cfg%opt,OPT_DIST_BLK_CNT)
       call chol_task_gen(dt_cfg,blk_cnt,dt_cfg%grp_rows,dt_cfg%grp_cols,dt_cfg%np/blk_cnt)
    end if
    call chol_init_data(dt_cfg)
    call print_lists(dt_cfg,.true.)
    call print_chol_data(dt_cfg)
  end function dt_init
!!$------------------------------------------------------------------------------------------------------------------
  subroutine rma_import_tasks(this,fname)
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

  end subroutine rma_import_tasks
!-------------------------------------------------------------------------------
  function dt_event(dt_obj,event,obj) result (res)
    type(remote_access) , intent(inout) :: dt_obj
    integer , intent(in) :: event,obj
    integer :: res

    write (*,*) "Event Notify is called for event:",event,obj,dt_obj%task_list(obj)%name
    
    select case (event)
       case (EVENT_TASK_STARTED)
          res = task_chol_local(dt_obj,dt_obj%task_list(obj))
          write (*,*) "after calling task_chol_local:",res,dt_obj%task_list(obj)%name
       case (EVENT_ALL_TASK_FINISHED)
          TRACEX("check result",dt_obj%data_list(1)%mat(:,1))
          call check_chol_result(dt_obj)
    end select
    
  end function dt_event 
!------------------------------------------------------------------------------------------
   function block_region(i,j,b) result(area)
     integer , intent(in) :: i,j,b
     type(area_type) :: area
     area%pos (AREA_X1) = (j-1)*b+1
     area%pos (AREA_X2) = (j  )*b
     area%pos (AREA_Y1) = (i-1)*b+1
     area%pos (AREA_Y2) = (i  )*b
   end function block_region
!------------------------------------------------------------------------------------------
   subroutine chol_sync(task,this,m)
     type(dist_chol), pointer,intent(inout)::this
     integer , intent(in) :: task
     character(len=*),intent(in)::m
     character(len=38):: blk
     type(chol_args) :: arg
     type(tl_handle),dimension(:),pointer :: hReads
     type(tl_handle),dimension(1:1) ::hNull
     integer :: by,bx,n
     n=this%nb
     arg%this => this
     allocate ( hReads(1:n) ) 
     do by=1,n
        bx = n
        if(task == DIAG) bx = by
        blk=global_block_name(CHOL_CONTEXT,m,by,bx)
        hReads(by) = dm_handle_get (this%dmngr,blk) 
        TRACEX("blk in sync",(blk,hReads(by) ) )
     end do

     SG_ADD_TASK(TNAME_CHOL_SYNC,FUNC_NAME(chol_task_sync),arg,sizeof(arg),hReads,n, hNull,0 ,hNull,0)
#ifdef SG_DEBUG     
     call chol_task_sync(arg)
     call print_lists(this%rma,.true.)
#endif

     deallocate ( hReads ) 

   end subroutine chol_sync

!------------------------------------------------------------------------------------------
   subroutine addtask_subt(this,arg,blk1,area1,blk2,area2,blk3,area3)
     type(dist_chol), pointer,intent(inout)::this
     type(area_type) , intent(in)  :: area1,area2,area3
     character(len=*),intent(in):: blk1,blk2,blk3
     type(chol_args),intent(inout) :: arg
     type(tl_handle),dimension(1:2) :: hReads,hRW,hAdd
     arg%areas(1) = area1
     arg%areas(2) = area2
     arg%areas(3) = area3
     hReads(1) = dm_handle_get (this%dmngr,blk1) 
     hReads(2) = dm_handle_get (this%dmngr,blk2) 
     hRW   (1) = dm_handle_get (this%dmngr,blk3) 

     SG_ADD_TASK(TNAME_CHOL_SUBT,FUNC_NAME(chol_subt_local),arg,sizeof(arg),hreads,2,hRW,1,hAdd,0)


   end subroutine addtask_subt
!------------------------------------------------------------------------------------------
   subroutine addtask_diag(this,arg,blk1,area1)
     type(dist_chol), pointer,intent(inout)::this
     type(area_type) , intent (in )  :: area1
     type(chol_args),intent(inout) :: arg
     character(len=*),intent(in):: blk1
     type(tl_handle),dimension(1:1) :: hRead,hRW,hAdd

     arg%areas(1) = area1

     hRW(1) = dm_handle_get (this%dmngr,blk1)
     TRACEX("Diag read block",(blk1,area1,hRW))

     SG_ADD_TASK(TNAME_CHOL_DIAG,FUNC_NAME(chol_diag_upd_local),arg,sizeof(arg),hRead,0,hRW,1,hAdd,0)


   end subroutine addtask_diag
!------------------------------------------------------------------------------------------
   subroutine addtask_pnl(this,arg,blk1,area1,blk2,area2)
     type(dist_chol), pointer,intent(inout)::this
     type(area_type) , intent(in )  :: area1,area2
     type(chol_args),intent(inout) :: arg
     character(len=*),intent(in):: blk1,blk2
     type(tl_handle),dimension(1:1) :: hRead, hRW,hAdd
     arg%areas(1) = area1
     arg%areas(2) = area2
     hRead(1) = dm_handle_get (this%dmngr,blk1)
     hRW  (1) = dm_handle_get (this%dmngr,blk2)

     TRACEX("panel read blocks",(blk1,blk2))
     SG_ADD_TASK(TNAME_CHOL_PNLU,FUNC_NAME(chol_mat_pnlu_local),arg,sizeof(arg),hread,1,hRW,1,hAdd,0)


   end subroutine addtask_pnl
   !------------------------------------------------------------------------------------------
   subroutine addtask_gemm(this,arg,blk1,area1,blk2,area2,blk3,area3,alpha,beta)
     type(dist_chol), pointer,intent(inout)::this
     type(area_type) , intent(in )  :: area1,area2,area3
     character(len=*),intent(in):: blk1,blk2,blk3
     type(chol_args),intent(inout) :: arg
     type(tl_handle),dimension(1:2) :: hReads,hRW,hAdd
     real(kind=rfp), intent(in) :: alpha,beta

     arg%areas(1) = area1
     arg%areas(2) = area2
     arg%areas(3) = area3
     arg%alpha = alpha
     arg%beta  = beta
     arg%Btranspose = .true. 
     hReads(1) = dm_handle_get (this%dmngr,blk1) 
     hReads(2) = dm_handle_get (this%dmngr,blk2) 
     hRW   (1) = dm_handle_get (this%dmngr,blk3) 

     SG_ADD_TASK(TNAME_CHOL_MMUL,FUNC_NAME(chol_mat_mul_local),arg,sizeof(arg),hreads,2,hRW,1,hAdd,0)


   end subroutine addtask_gemm
!------------------------------------------------------------------------------------------
   subroutine tgen_subt_shmem(this) 
     type(dist_chol) ,intent(inout),pointer :: this
     type(chol_args) :: arg
     integer :: i,j,k,n,b
#define block_part(a,y,x) global_block_name(CHOL_CONTEXT,a,y,x),block_region(y,x,b)
#define AddTask_Subt(a,b,c) call addtask_subt(this,arg,a,b,c)
#define Subt_Constructor(a,b,c,d) call task_arg_constructor(SUBT,a,b,c,d)
#define Subt_Sync(a) 
#define _A this%d1name
#define _B this%d2name
#define _C this%d3name
#define Subt_Sync(a) call chol_sync(SUBT,this,a)
!!$     Subt(A,B,C): C= B-A
!!$     loop i=1,n
!!$       loop j=1,n
!!$            subt ( A(i,j), B(i,j), C( i,j))

     Subt_Constructor(this,arg,n,b)
     do i=1,n 
        do j = 1,n
              AddTask_Subt ( block_part(_A,i,j) , block_part(_B,i,j), block_part(_B,i,j))
        end do
     end do
     Subt_Sync(_C) 

#undef _A
#undef _B
#undef _C

   end subroutine tgen_subt_shmem
   !------------------------------------------------------------------------------------------
   subroutine tgen_gemm_shmem(this) 
     type(dist_chol) ,intent(inout),pointer :: this
     type(chol_args) :: arg
     integer :: i,j,k,n,b
#define block_part(a,y,x) global_block_name(CHOL_CONTEXT,a,y,x),block_region(y,x,b)
#define AddTask_Gemm(a,d,c) call addtask_gemm(this,arg,a,d,c,1.0_rfp,0.0_rfp)
#define Gemm_Constructor(a,e,c,d) call task_arg_constructor(GEMM,a,e,c,d)
#define _A this%d1name
#define _B this%d2name
#define _C this%d3name
#define Gemm_Sync(a) call chol_sync(GEMM,this,a)
!!$    Gemm(A,B,C): 
!!$    loop i = 1,n
!!$      loop j=1,n
!!$        loop k = 1,n
!!$          gemm ( A(i,k) , B(k,j), C(i,j))

     Gemm_Constructor(this,arg,n,b)
     do j = 1,n
        do i=j+1,n 
           do k = 1,n
              AddTask_Gemm ( block_part(_A,i,k) , block_part(_B,k,j), block_part(_C,i,j))
           end do
        end do
     end do

     Gemm_Sync(_C)
#undef _A
#undef _B
#undef _C

   end subroutine tgen_gemm_shmem
   !------------------------------------------------------------------------------------------
   subroutine tgen_pnl_shmem(this) 
     type(dist_chol) ,intent(inout),pointer :: this
     type(chol_args) :: arg
     integer :: i,j,k,n,b
#define block_part(a,y,x) global_block_name(CHOL_CONTEXT,a,y,x),block_region(y,x,b)
#define AddTask_Pnl(a,c) call addtask_pnl(this,arg,a,c)
#define AddTask_Gemm(a,d,c) call addtask_gemm(this,arg,a,d,c,-1.0_rfp,1.0_rfp)
#define Pnl_Constructor(a,e,c,d) call task_arg_constructor(PNLU,a,e,c,d)
#define _A this%d1name
#define _B this%d2name
#define Pnl_Sync(a) call chol_sync(PNLU,this,a)
!!$    PNL(A,B): 
!!$    loop j = 1,n
!!$      loop for i=j+1,n :  
!!$          pnl (A(j,j),B(i,j))
!!$      loop i=1,n
!!$        loop k = 1,j-1
!!$          gemm ( A(j,k) , B(i,k), B(i,j))

     Pnl_Constructor(this,arg,n,b)
     do j = 1,n
        do i=1,n
           do k = 1,j-1
              AddTask_Gemm ( block_part(_A,j,k) , block_part(_B,i,k), block_part(_B,i,j))
           end do
        end do
        do i=j+1,n 
           AddTask_Pnl ( block_part(_A,j,j),block_part(_B,i,j) )
        end do
     end do

     Pnl_Sync(_B)
#undef _A
#undef _B

   end subroutine tgen_pnl_shmem
!------------------------------------------------------------------------------------------
   subroutine tgen_diag_shmem(this) 
     type(dist_chol) ,intent(inout),pointer :: this
     type(chol_args) :: arg
     integer :: i,j,k,n,b
#define block_part(a,y,x) global_block_name(CHOL_CONTEXT,a,y,x),block_region(y,x,b)
#define AddTask_Pnl(a,b) call addtask_pnl(this,arg,a,b)
#define AddTask_Gemm(a,b,c) call addtask_gemm(this,arg,a,b,c,-1.0_rfp,1.0_rfp)
#define AddTask_Diag(a) call addtask_diag(this,arg,a)
#define Diag_Constructor(a,b,c,d) call task_arg_constructor(DIAG,a,b,c,d)
#define Diag_Sync(a) call chol_sync(DIAG,this,a)
#define _A this%d1name
!!$     Diag(A):
!!$       loop j=1,n
!!$       Diag(j,j)
!!$       loop i = j+1,n
!!$          pnl ( A(j,j),A(i,j) ) 
!!$       loop i =j+1,n
!!$         loop k =1,j
!!$            gemm ( A(j,k),A(i,k), A(i,j)) 
       
     Diag_Constructor(this,arg,n,b)
     do j = 1,n
        AddTask_Diag ( block_part(_A,j,j) )
        do i=j+1,n 
           AddTask_Pnl ( block_part(_A,j,j),block_part(_A,i,j) )
        end do
        do i=j+1,n
           do k = 1,j
              AddTask_Gemm ( block_part(_A,j+1,k) , block_part(_A,i,k), block_part(_A,i,j+1))
           end do
        end do
     end do
     Diag_Sync(_A) 

#undef _A

   end subroutine tgen_diag_shmem
!------------------------------------------------------------------------------------------
   subroutine task_arg_constructor(task,this,arg,n,b)
     integer , intent(in) :: task
     integer , intent(inout) :: n,b
     type(chol_args),intent(inout) :: arg
     type(dist_chol),pointer,intent(inout):: this
     n = this%nb
     b = this%rows / n
     arg%this => this
     TRACEX("SgBlockSz",(b,this%rows,this%nb))
     select case (task)

     case (DIAG)
        call chol_create_handles(this,this%d1name,DIAG)
     case (PNLU)
        call chol_create_handles(this,this%d1name,PNLU)
        call chol_create_handles(this,this%d2name,PNLU)
     case (MMUL,SUBT)
        TRACEX("zzz44",(this%d1name,this%d2name,this%d3name))
        call chol_create_handles(this,this%d1name,MMUL)
        call chol_create_handles(this,this%d2name,MMUL)
        call chol_create_handles(this,this%d3name,MMUL)
        
     end select
   end subroutine task_arg_constructor
!------------------------------------------------------------------------------------------
   function global_block_name(context,m,r,c)result (bname)
    integer         , intent(in)               :: r,c
    character(len=9), intent(in)               :: context
    character(len=16), intent(in)              :: m
    character(len=38)               :: bname
    write ( bname,"(A9 A1 A16 A3 I4.4 A1 I4.4)")  context,'.',m,'.B_',r,'_',c
   end function global_block_name
!------------------------------------------------------------------------------------------
  subroutine  chol_create_handle(this,context,m,r,c)
    type(dist_chol) , intent(inout) , pointer  :: this
    integer         , intent(in)               :: r,c
    character(len=9), intent(in)               :: context
    character(len=16), intent(in)              :: m
    character(len=38)               :: hname
    type(tl_handle) :: hnd,hNull    

    write ( hname,"(A9 A1 A16 A3 I4.4 A1 I4.4)")  context,'.',m,'.B_',r,'_',c
    hnd = dm_handle_get( this%dmngr, hname)
    if ( hnd%address == hNull%address ) then  
       call  dm_matrix_add ( this%dmngr, hname)
       hnd = dm_handle_get( this%dmngr, hname)
       TRACEX("d name,hnd",(hname,hnd))
    end if
    
  end subroutine chol_create_handle
!------------------------------------------------------------------------------------------

  subroutine chol_create_handles(this,m,task)
    type(dist_chol) , intent(inout) , pointer  :: this
    character(len=16), intent(in)               :: m
    integer ,intent(in) :: task
    character(len=9)                :: context
    integer                    :: r,c,cmax
    cmax = this%nb
    do r= 1, this%nb
       if ( task == DIAG ) cmax=r
       do c=1,cmax
          call chol_create_handle(this,CHOL_CONTEXT,m,r,c)
       end do
    end do
  end subroutine chol_create_handles
!------------------------------------------------------------------------------------------
  subroutine tl_add_task_debug(tname,this,func,arg,sz_arg,hr,nr,hw,nw,ha,na)
    type(dist_chol) , intent(inout) :: this
    integer ,intent(in) :: sz_arg,nr,nw,na
    type(chol_args) , intent(in)::arg
    type(tl_handle),dimension(:)::hr,hw,ha
    character(len=*),intent(in):: tname,func
    character(len=38) ::A,B,C,ctx
    character(len=3) :: sep
    integer :: mi,mj,ii,jj,kk,bz,i,yoff,xoff,dm_blk_size 
    type(area_type) :: ar
#define fx(a) dm_get_name(this%dmngr,a)    
#define area_part ar%pos(AREA_Y1)+yoff, ":" , ar%pos(AREA_Y2)+yoff , ",",ar%pos(AREA_X1)+xoff, ":" , ar%pos(AREA_X2)+xoff

    dm_blk_size = this%dm_blk_size
    bz = this%rows/this%nb 
    TRACEX("SG_TASK ",func(6:15))
    B=""
    TRACEX("hWr",(hw(1),hw(1)%address))
    C=fx(hw(1))
    if ( nr == 0 ) A=C
    TRACEX("hr1",(hr(1),hr(1)%address))
    if ( nr >= 1 ) A=fx(hr(1))
    TRACEX("hr2",(hr(2),hr(2)%address))
    if ( nr >  1 ) B=fx(hr(2))
    
    TRACEX("SG_TASK A",A)
    TRACEX("SG_TASK B",B)
    TRACEX("SG_TASK C",C)
!    "Cholesky.M_001_001.B_0004_0002"

    read ( A, "(A19 I3 A1 I3 A3 I4 A1 I4)") ctx,mi,sep,mj,sep,ii,sep,jj
    yoff = (mi-1)* dm_blk_size
    xoff = (mj-1)* dm_blk_size
    ar = block_region ( ii,jj,bz ) 
    TRACEX("SG_TASK ",( " A(",ii,",",jj,")" , "== M[",area_part,"]" ))

    if ( nr == 2 ) then 
       read ( B, "(A19 I3 A1 I3 A3 I4 A1 I4)") ctx,mi,sep,mj,sep,ii,sep,jj
       yoff = (mi-1)* dm_blk_size
       xoff = (mj-1)* dm_blk_size
       ar = block_region ( ii,jj,bz ) 
       TRACEX("SG_TASK ",( " B(",ii,",",jj,")" , "== M[",area_part,"]" ))
    end if
    if ( nr > 2 ) then 
       do i = 2,nr
          B=fx(hr(i))
          read ( B, "(A19 I3 A1 I3 A3 I4 A1 I4)") ctx,mi,sep,mj,sep,ii,sep,jj
          yoff = (mi-1)* dm_blk_size
          xoff = (mj-1)* dm_blk_size
          ar = block_region ( ii,jj,bz ) 
          TRACEX("SG_TASK ",( " B(",ii,",",jj,")" , "== M[",area_part,"]" ))
       end do
    end if
    read ( C, "(A19 I3 A1 I3 A3 I4 A1 I4)") ctx,mi,sep,mj,sep,ii,sep,jj
    yoff = (mi-1)* dm_blk_size
    xoff = (mj-1)* dm_blk_size
    ar = block_region ( ii,jj,bz ) 
    TRACEX("SG_TASK ",( " C(",ii,",",jj,")" , "== M[",area_part,"]" ))
 
    
  end subroutine tl_add_task_debug
end module cholesky
