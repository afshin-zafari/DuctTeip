
#include "debug.h" 
!#define TIMING(a,b,c,d) call instrument2(a,b,c,d)


#define KERN(a) 
#define TASK(a) 

#define DIAG(a)  a
#define PNLU(a)  
#define MMUL(a)  
#define XADD(a)  
#define MADD(a)   

#define HANDLE(a) 
#define DRESULT(a) 
#define DRANGE(a) 

#define TRENTER(a)  
#define TREXIT(a) 

#define CHOL_NOPRT(a)  a
#define CHOL_PRT(a)  

#define SEQ(a) 

module dist_cholesky

  use tl
  use dm_class
  use fp
  use timer
  use dist_common
  use dm_class
  use dist_data_class

  implicit none

  integer , parameter  :: NAME_LEN  = 20
  integer , parameter  :: TASK_MADD = 1
  integer , parameter  :: TASK_DIAG = 2
  integer , parameter  :: TASK_PNLU = 3
  integer , parameter  :: TASK_MMUL = 4
  integer , parameter  :: TASK_XADD = 5

  character(len=NAME_LEN) , parameter  :: TASK_XADD_NAME = "XADD"
  character(len=NAME_LEN) , parameter  :: TASK_MMUL_NAME = "MMUL"
  character(len=NAME_LEN) , parameter  :: TASK_PNLU_NAME = "PNLU"
  character(len=NAME_LEN) , parameter  :: TASK_DIAG_NAME = "DIAG"
  character(len=NAME_LEN) , parameter  :: TASK_MADD_NAME = "MADD"

  type chol_args
     character(len=NAME_LEN)                                 :: TName
     type(dist_chol) , pointer  :: this
!     integer                    :: fr,tr,fc,tc,fr2,tr2,fc2,tc2,pnlu_col ,i,j,dealloc=0,rb,cb,mul_overwrite=0
     integer                    :: rb,cb,bidx
     real(kind=rfp)          , dimension ( :,:)    , pointer :: data1,data2,data3
     type(tl_handle) , dimension(3) :: hList
  end type chol_args

  type dist_chol
     character(len=NAME_LEN)                                 :: TName
     character(len=NAME_LEN) , dimension ( 1:5)              :: Writes
     real(kind=rfp)          , dimension ( :,:)    , pointer :: data1,data2,data3
     type(data_handle)                             , pointer :: dobj1,dobj2,dobj3
     type(dm)                                      , pointer :: Dmngr
     integer                                                 :: nb,np,rows,cols
  end type dist_chol


contains 
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_task_sync(arg)
    type(chol_args) ,intent(inout):: arg

    
    TRACEX("CHOL_SYNC",arg%this%TName)

    TRACEX("CHOL_SYNC","D3")
    call data_combine_parts(arg%this%dobj3,arg%this%rows,arg%this%cols,arg%this%nb)
    TRACEX("CHOL_SYNC","D2")
    call data_combine_parts(arg%this%dobj2,arg%this%rows,arg%this%cols,arg%this%nb)
    TRACEX("CHOL_SYNC","D1")
    call data_combine_parts(arg%this%dobj1,arg%this%rows,arg%this%cols,arg%this%nb)

    call dm_set_sync(arg%this%dmngr,1)

  end subroutine chol_task_sync


!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_diag_upd(arg)
    type(chol_args), intent(inout)               :: arg
    integer                                      :: col,r,fr,cb,rb,tmp
    real(kind=rfp) , dimension ( :,: ) , pointer :: A,C,B
    real(kind=rfp)                               :: div,sm
    type(tl_handle)                              :: left_col,cur_col,top_row,hnull
    character (len=NAME_LEN)                     :: lname,tname,cname

    


    A => arg%data1
    C => arg%data1

    if ( .not. associated ( A)  ) then 
       KERN(DIAG(TRACEX("CHOL_KERN,Diag. A  associated",associated (A))))
       return 
    end if
    if ( .not. associated ( C)  ) then 
       KERN(DIAG(TRACEX("CHOL_KERN,Diag. C  associated",associated (C))))
       return 
    end if
    
    TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,arg%bidx) 

    do col=lbound(A,2),ubound(A,2)
       if ( col >1 ) then 
          
          do r = col,ubound(A,1)
             C(r,col) = A(r,col) - sum( A(r,1:col-1)*A(col,1:col-1) ) 
          end do
       end if
       fr = col+1
       C(col,col) = sqrt ( abs(C(col,col)))    
       C(fr:,col) = A(fr:,col) / C(col,col)
    end do

    TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,arg%bidx) 
    KERN(DIAG(TRACEX("CholData: Diag ","A")))
    call chol_print_data( A, arg%rb,arg%cb)
    KERN(DIAG(TRACEX( "CHOL_KERN,Diag.","Exit")))
  end subroutine chol_diag_upd

!!$------------------------------------------------------------------------------------------------------------------------
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_pnlu_local(arg)
    type(chol_args), intent(inout)               :: arg
    real(kind=rfp) , dimension ( :,: ) , pointer :: A,B
    integer                                      :: col,r,fr,cb
    character (len=NAME_LEN)                     :: name
    type(tl_handle)                              :: left_col,cur_col



    KERN(PNLU(TRACEX( "CHOL_KERN,Pnlu.  enter",(arg%fr,arg%tr,arg%fc,arg%tc))))
    
    A => arg%data1
    B => arg%data2 

    if ( .not. associated (A)  ) return 
    if ( .not. associated (B)  ) return 


    TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,arg%bidx) 
    do col=1,ubound(B,2)
       
       !KERN(PNLU(TRACEX("CHOL_KERN,Pnlu.  c,f:t",(col,",",arg%fr,arg%tr,"x",arg%fc,arg%tc))))
       if ( col >1 ) then 
          do r = 1,ubound(A,1)
             A(r,col) = A(r,col) - sum( A(r,1:col-1)*B(col,1:col-1) ) 
          end do
       end if
       
       A(:,col) = A(:,col)/ B(col,col)

    end do
    TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,arg%bidx) 
!    TRACEX("CholData: Pnlu A=p(A,B)","A")
    call chol_print_data( A, arg%rb,arg%cb)
!    TRACEX("CholData: Pnlu ","B")
    call chol_print_data( B, arg%rb,arg%cb)

  end subroutine chol_mat_pnlu_local


!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_mul_local(arg)
    type(chol_args), intent(inout)             :: arg
    real(kind=rfp) , dimension(:,:) , pointer  :: A,B,C

    KERN(MMUL(TRACEX("CHOL_KERN,MMul.  mul overwrt",arg%mul_overwrite)))

    A => arg%data1
    B => arg%data2
    C => arg%data3

    if ( .not. associated (A) ) return 
    if ( .not. associated (B) ) return 
    if ( .not. associated (C) ) return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,arg%bidx) 


    C= matmul( A ,transpose(B) )

    TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,arg%bidx) 
    CHOL_PRT(TRACEX("CholData:","CHOL_KERN,MMul_local"))
!    TRACEX("CholData: MMul C=A*Bt","A")
    call chol_print_data( A, arg%rb,arg%cb)
!    TRACEX("CholData: MMul ","B")
    call chol_print_data( B, arg%rb,arg%cb)
!    TRACEX("CholData: MMul ","C")
    call chol_print_data( C, arg%rb,arg%cb)
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.","Exit")))

  end subroutine chol_mat_mul_local
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_add_local(arg)
    type(chol_args), intent(inout)             :: arg
    real(kind=rfp) , dimension(:,:) , pointer  :: A,B,C


    A => arg%data1
    B => arg%data2
    C => arg%data3
    
    if ( .not. associated (A) ) return 
    if ( .not. associated (B) ) return 
    if ( .not. associated (C) ) return 


 !   TRACEX("CholData: MAdd C=B-A","C0")
    call chol_print_data( C, arg%rb,arg%cb)
    
    TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,arg%bidx) 
    C(:,:)=B(:,:)-A(:,:)

    TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,arg%bidx) 
  !  TRACEX("CholData: MAdd ","C")
    call chol_print_data( C, arg%rb,arg%cb)
  !  TRACEX("CholData: MAdd ","B")
    call chol_print_data( B, arg%rb,arg%cb)
  !  TRACEX("CholData: MAdd ","A")
    call chol_print_data( A, arg%rb,arg%cb)
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.","Exit")))


  end subroutine chol_mat_add_local
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
!    Type(dist_chol) , Intent(inout), Target  :: this
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(inout):: mat
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
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(inout):: mat
    integer , intent(in) :: nb 
    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
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

    allocate ( Xmat ( 1:nrows,1:ncols) ) 
    Xmat(:,:) = 0

    rbno = nrows / rb + 1
    cbno = ncols / cb + 1
    TRACEX("SeqBlock. BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          fr =     (rbidx-1) * rb+1
          tr = min( rbidx    * rb  ,nrows)
          fc =     (cbidx-1) * cb+1
          tc = min( cbidx    * cb  ,ncols)
          if ( fr > ubound(mat,1) ) exit
          if ( fc > ubound(mat,2) ) exit
          
          SEQ(TRACEX("SeqBlock. Loop cb,rb, ranges",(cbidx,rbidx,fr,tr,fc,tc)))
          if ( rbidx > cbidx ) then 
             ! panel update
             SEQ(TRACEX("SeqBlock. Panel update",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
             do c = fc,tc
                if ( c > fc ) then 
                   do r=fr,tr
                      SEQ(TRACEX("SeqBlock. Panel update inner loop ",(r,c)))
                      SEQ(TRACEX("SeqBlock.",("mat(", c,',',fc,':',c-1,")*mat(",r,',',fc,':',c-1,"))")))
                      SEQ(TRACEX("SeqBlock.",(sum(mat( c,fc:c-1)*mat(r,fc:c-1)))))
                      mat(r,c) = mat(r,c) - sum(mat( c,fc:c-1)*mat(r,fc:c-1))
                   end do
                end if
                !call chol_print_data( mat,rb,cb ) 
                mat ( fr:tr,c) =  mat ( fr:tr,c) / mat ( c,c)                   
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
             ! gemm tasks
             fr2 = (cbidx)*rb +1 
             tr2 = min((cbidx+1)*rb , nrows)
             SEQ(TRACEX("SeqBlock. gemm update ",(fr2,tr2)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
             Xmat(fr:tr,fc:tc) = matmul(  mat (fr:tr,fc:tc) , transpose(mat (fr2:tr2,fc:tc)) )
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
             do cbidx2=cbidx,rbidx-1
                fc2 =  cbidx2   *cb + 1
                tc2 = (cbidx2+1)*cb
                if ( fc2 > ubound(mat,2) ) exit 
                TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,rbidx*cb+cbidx2) 
                mat(fr:tr,fc2:tc2) = mat(fr:tr,fc2:tc2) - Xmat(fr:tr,fc:tc)
                TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,rbidx*cb+cbidx2) 
             end do
             call chol_print_data( mat ,rb,cb ) 
             call chol_print_data( Xmat,rb,cb ) 
          end if
          if (rbidx == cbidx) then 
             ! diagonal block 
             SEQ(TRACEX("SeqBlock. Diagonal update",(fc,tc)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             do c=fc,tc
                if ( c >1 ) then 
                   do r = c,tr
                      SEQ(TRACEX("SeqBlock. Diagonal update inner loop ",(r,c)))
                      mat(r,c) = mat (r,c) - sum(  mat(r,fc:c-1)*mat(c,fc:c-1)  )              
                   end do
                end if
                fr = c+1
                mat(c,c) = sqrt ( abs(mat(c,c)))    
                mat(fr:tr,c) = mat(fr:tr,c) / mat(c,c)    
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             call chol_print_data( mat,rb,cb ) 
          end if 
       end do ! row block idx
    end do ! col block idx


  end subroutine chol_sequential_block
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_sequential_cache_opt(mat ,nb) 
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(inout):: mat
    integer , intent(in) :: nb 
    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
    type(chol_args) :: arg
    integer :: nrows,ncols ! number of columns and rows 
    integer :: rb   ,cb    ! number of rows or columns in block 
    integer :: rbno ,cbno  ! total number of blocks in rows or columns
    integer :: rbidx,cbidx ! index for row and column blocks 
    integer :: fr,tr,fc,tc ! range of rows and column in blocks 
    integer :: fr2,tr2     ! row range for panel updating
    integer :: r,c         ! index for row and columns in blocks
    integer :: seq


    if ( .not. associated ( mat ) ) return
 
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

    allocate ( Xmat ( 1:nrows,1:cb) ) 
    Xmat(:,:) = 0

    rbno = nrows / rb + 1
    cbno = ncols / cb + 1
    TRACEX("SeqBlock. BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          fr =     (rbidx-1) * rb+1
          tr = min( rbidx    * rb  ,nrows)
          fc =     (cbidx-1) * cb+1
          tc = min( cbidx    * cb  ,ncols)
          if ( fr > ubound( mat ,1 ))  exit
          if ( fc > ubound( mat ,2 ))  exit
          SEQ(TRACEX("SeqBlock. Loop cb,rb, ranges",(cbidx,rbidx,fr,tr,fc,tc)))
          if ( cbidx > 1 ) then 
             ! add X to mat
             SEQ(TRACEX("SeqBlock. Subtract X from Mat",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,rbidx*cb+cbidx) 
             mat (fr:tr,fc:tc) = mat(fr:tr,fc:tc) - Xmat(fr:tr,:)
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,rbidx*cb+cbidx) 
             call chol_print_data( mat,rb,cb ) 
          end if
          if ( rbidx > cbidx ) then 
             ! panel update
             SEQ(TRACEX("SeqBlock. Panel update",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
             do c = fc,tc
                if ( c > fc ) then 
                   do r=fr,tr
                      SEQ(TRACEX("SeqBlock. Panel update inner loop ",(r,c)))
                      SEQ(TRACEX("SeqBlock.",("mat(", c,',',fc,':',c-1,")*mat(",r,',',fc,':',c-1,"))")))
                      SEQ(TRACEX("SeqBlock.",(sum(mat( c,fc:c-1)*mat(r,fc:c-1)))))
                      mat(r,c) = mat(r,c) - sum(mat( c,fc:c-1)*mat(r,fc:c-1))
                   end do
                end if
                !call chol_print_data( mat,rb,cb ) 
                mat ( fr:tr,c) =  mat ( fr:tr,c) / mat ( c,c)                   
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,rbidx*cb+cbidx) 
             ! gemm tasks
             fr2 = (cbidx)*rb +1 
             tr2 = min((cbidx+1)*rb , nrows)
             SEQ(TRACEX("SeqBlock. gemm update ",(fr2,tr2)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
             Xmat(fr:tr,:) = Xmat(fr:tr,:) +  matmul(  mat (fr:tr,fc:tc) , transpose(mat (fr2:tr2,fc:tc)) )
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,rbidx*cb+cbidx) 
             call chol_print_data( mat ,rb,cb ) 
             call chol_print_data( Xmat,rb,cb ) 
          end if
          if (rbidx == cbidx) then 
             ! diagonal block 
             SEQ(TRACEX("SeqBlock. Diagonal update",(fc,tc)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             do c=fc,tc
                if ( c >1 ) then 
                   do r = c,tr
                      SEQ(TRACEX("SeqBlock. Diagonal update inner loop ",(r,c)))
                      mat(r,c) = mat (r,c) - sum(  mat(r,fc:c-1)*mat(c,fc:c-1)  )              
                   end do
                end if
                fr = c+1
                mat(c,c) = sqrt ( abs(mat(c,c)))    
                mat(fr:tr,c) = mat(fr:tr,c) / mat(c,c)    
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,rbidx*cb+cbidx) 
             call chol_print_data( mat,rb,cb ) 
          end if 
       end do ! row block idx
    end do ! col block idx


  end subroutine chol_sequential_cache_opt

!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_diag_task(this ) 
    type(dist_chol),intent(inout),pointer :: this
    type(chol_args) :: arg
    real(kind=rfp) , dimension ( :,: ) , pointer :: mat
    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
    integer :: nrows,ncols ! number of columns and rows 
    integer :: rb   ,cb    ! number of rows or columns in block 
    integer :: rbno ,cbno  ! total number of blocks in rows or columns
    integer :: rbidx,cbidx ! index for row and column blocks 
    integer :: fr,tr,fc,tc ! range of rows and column in blocks 
    integer :: fr2,tr2     ! row range for panel updating
    integer :: r,c         ! index for row and columns in blocks
    type(tl_handle )    :: hRead,hWrite,hAdd
    type(tl_handle) , dimension( 2) :: hReads
    type(tl_handle) , dimension( :),pointer :: hReadList,hAddList
    integer :: argidx ,cbidx2,hidx
    type(data_handle) :: Xdobj
    


    mat => this%data1
 
    nrows = this%rows
    ncols = this%cols
    TRACEX("nb",this%nb)
    rb=nrows / this%nb
    cb=ncols / this%nb
    allocate ( Xdobj%mat ( 1:nrows, 1:ncols) ) 
    Xdobj%mat(:,:) = 0  
    allocate ( Xdobj%dmngr )
    call dm_new(Xdobj%dmngr)
    call data_partition_matrix(this%dobj1,this%nb)
    call data_partition_matrix(this%dobj2,this%nb)
    call data_partition_matrix(this%dobj3,this%nb)
    call data_partition_matrix(Xdobj     ,this%nb)

    rbno = this%nb
    cbno = this%nb

    TRACEX("CholTask.Diag BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          hAdd  = chol_get_handle(this,'M',rbidx,cbidx,.true.)
          hAdd  = chol_get_handle(this,'X',rbidx,cbidx,.true.)
       end do
    end do
    arg%rb = rb
    arg%cb = cb 
    arg%this => this
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          arg%bidx = rbidx*cb + cbidx
          TASK(DIAG(TRACEX("CholTask.Diag  Loop cb,rb, ranges",(argidx,cbidx,rbidx))))
          if ( rbidx > cbidx ) then 
             ! panel update
             ! B d2 diag, A d1 
             ! read handle M(cbidx,cbidx) , add handle M(rbidx,cbidx)
             hRead = chol_get_handle(this,'M',cbidx,cbidx)
             hAdd  = chol_get_handle(this,'M',rbidx,cbidx)
             arg%data1 => data_get_mat_part(this%dobj1,rbidx,cbidx)
             arg%data2 => data_get_mat_part(this%dobj1,cbidx,cbidx)
             arg%TName = "PNLU"
             TASK(DIAG(TRACEX("CholTask.Diag. PNLU handle",(hRead,hAdd)) ))
             call tl_add_task_named(TASK_PNLU_NAME//char(0),chol_mat_pnlu_local, arg, sizeof(arg), &
                  hRead,1, hAdd,1, 0,0,5)
             ! gemm tasks
             ! C=C+AB
             ! read handle M(cbidx+1,cbidx), M(rbidx,cbidx), add handle X(rbidx,cbidx+1)
             hReads(1) = chol_get_handle(this,'M',cbidx+1,cbidx  )
             hReads(2) = chol_get_handle(this,'M',rbidx  ,cbidx  )
             hAdd      = chol_get_handle(this,'X',rbidx  ,cbidx+1)
             arg%data1 => data_get_mat_part(this%dobj1,cbidx+1,cbidx   )
             arg%data2 => data_get_mat_part(this%dobj1,rbidx  ,cbidx   )
             arg%data3 => data_get_mat_part(Xdobj     ,rbidx  ,cbidx+1 )
             arg%TName = "MMUL"
             TASK(DIAG(TRACEX("CholTask.Diag. MMUL handle",(hReads,hAdd)) ))
             call tl_add_task_named( TASK_MMUL_NAME , chol_mat_mul_local , arg, sizeof(arg) , &
                  hReads,2, hAdd ,1 , 0,0, 5)


             ! add X to mat
             ! read handle X( rbidx,cbidx) , add handle M(rbidx,cbidx)
             ! MADD: C=B-A
             do cbidx2=cbidx,rbidx-1
                arg%bidx = rbidx*cb + cbidx2
                hRead = chol_get_handle(this,'X',rbidx,cbidx +1  )
                hAdd  = chol_get_handle(this,'M',rbidx,cbidx2+1  )
                arg%data1 => data_get_mat_part(Xdobj     ,rbidx  ,cbidx+1 )
                arg%data2 => data_get_mat_part(this%dobj1,rbidx  ,cbidx2+1 )
                arg%data3 => data_get_mat_part(this%dobj1,rbidx  ,cbidx2+1 )
                arg%TName = "MADD"
                TASK(DIAG(TRACEX("CholTask.Diag. MADD handle",(hRead,hAdd)) ))
                call tl_add_task_named(TASK_MADD_NAME,chol_mat_add_local, arg,sizeof(arg) , &
                     hRead,1 , hAdd,1,0,0 ,5)
!                call sg_madd(SG_ARG(arg) , hRead,1 , hAdd,1,NO_ADD )
             end do
          end if
          if (rbidx == cbidx) then 
             hAdd = chol_get_handle ( this,'M',rbidx,cbidx )
             arg%data1 => data_get_mat_part(this%dobj1,rbidx,cbidx )
             TASK(DIAG(TRACEX("CholTask.Diag. DIAG handle",(hAdd)) ))
             arg%TName = "DIAG"
             call tl_add_task_named ( TASK_DIAG_NAME,chol_diag_upd,arg,sizeof(arg) , 0,0, hAdd,1 , 0,0,5)
!             call sg_diag(arg,sizeof(arg) , NO_READ, hAdd,1 , NO_ADD)
          end if 
       end do ! row block idx
    end do ! col block idx

    hRead = chol_get_handle ( this,'M',rbno,cbno)
    TASK(DIAG(TRACEX("CholTask.Diag. DIAG handle",(hRead)) ))
    call tl_add_task_named("sync"//char(0),chol_task_sync,arg,sizeof(arg), hRead,1, 0,0, 0,0, 5)

  end subroutine chol_diag_task
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
!!$------------------------------------------------------------------------------------------------------------------------
  function to_str4(no) result(str)
    integer          :: no
    character(len=4) :: str

    write (str,"(I4.4)") no

  end function to_str4
!!$------------------------------------------------------------------------------------------------------------------------
  function to_str3(no) result(str)

    integer          :: no
    character(len=2) :: str

    write (str,"(I2.2)") no-48

  end function to_str3

!!$  subroutine sg_madd( arg,hr,hrc,hw,hwc,ha,hwc)
!!$    call tl_add_task_named(TASK_MADD_NAME,chol_mat_add_local, arg,sizeof(arg) , 
!!$         hRead,1 , hAdd,1,0,0 ,5)
!!$  end subroutine sg_madd


  subroutine chol_mat_add_task(this) 
    type(dist_chol) ,intent(inout),pointer :: this
  end subroutine chol_mat_add_task
  subroutine chol_mat_xadd_task(this) 
    type(dist_chol) ,intent(inout),pointer :: this
  end subroutine chol_mat_xadd_task
  subroutine chol_mat_mul_task(this) 
    type(dist_chol) ,intent(inout),pointer :: this
  end subroutine chol_mat_mul_task
  subroutine chol_mat_pnlu_task(this) 
    type(dist_chol) ,intent(inout),pointer :: this
  end subroutine chol_mat_pnlu_task
  subroutine chol_diag_task_cache_opt(this) 
    type(dist_chol) ,intent(inout),pointer :: this
  end subroutine chol_diag_task_cache_opt!
end module dist_cholesky
