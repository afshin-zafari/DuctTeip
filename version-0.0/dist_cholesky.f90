
#include "debug.h" 
#define TIMING(a,b,c,d) 


#define KERN(a) 
#define TASK(a) 

#define DIAG(a)  a
#define PNLU(a)  a
#define MMUL(a)  a
#define XADD(a)  
#define MADD(a)  a 

#define HANDLE(a) a
#define DRESULT(a) a
#define DRANGE(a) a

#define TRENTER(a) a 
#define TREXIT(a) a

#define CHOL_NOPRT(a)  a
#define CHOL_PRT(a)  

#define SEQ(a) 

Module dist_cholesky

  Use tl
  Use dm_class
  Use fp
  use timer
  use dist_common

  Implicit None

  Integer , Parameter  :: NAME_LEN  = 17
  Integer , Parameter  :: TASK_MADD = 1
  Integer , Parameter  :: TASK_DIAG = 2
  Integer , Parameter  :: TASK_PNLU = 3
  Integer , Parameter  :: TASK_MMUL = 4
  Integer , Parameter  :: TASK_XADD = 5

  Character(len=NAME_LEN) , Parameter  :: TASK_XADD_NAME = "XADD"
  Character(len=NAME_LEN) , Parameter  :: TASK_MMUL_NAME = "MMUL"
  Character(len=NAME_LEN) , Parameter  :: TASK_PNLU_NAME = "PNLU"
  Character(len=NAME_LEN) , Parameter  :: TASK_DIAG_NAME = "DIAG"
  Character(len=NAME_LEN) , Parameter  :: TASK_MADD_NAME = "MADD"

  type chol_args
     Character(len=NAME_LEN)                                 :: TName
     Type(dist_chol) , Pointer  :: this
     integer                    :: fr,tr,fc,tc,fr2,tr2,fc2,tc2,pnlu_col ,i,j,dealloc=0,rb,cb,mul_overwrite=0
     Real(kind=rfp)          , Dimension ( :,:)    , Pointer :: data1,data2,data3
     type(tl_handle) , dimension(3) :: hList
  end type chol_args

  Type dist_chol
     Character(len=NAME_LEN)                                 :: TName
     Character(len=NAME_LEN) , Dimension ( 1:5)              :: Writes
     Real(kind=rfp)          , Dimension ( :,:)    , Pointer :: data1,data2,data3
     Type(dm)                                      , Pointer :: Dmngr
     Integer                                                 :: nb,np,rows,cols
  End Type dist_chol


contains 
!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_task_sync(arg)
    Type(chol_args) ,Intent(inout):: arg

    
    if ( arg%dealloc /= 0  ) then 
       TRACEX("CHOL_SYNC",ubound(arg%data1,2) )
!       call chol_print_data( arg%data1,arg%rb,arg%cb ) 
!       call chol_print_data( arg%data2,arg%rb,arg%cb ) 
!       call chol_print_data( arg%data3,arg%rb,arg%cb ) 
       deallocate ( arg%data3 )  
    end if
    call dm_set_sync(arg%this%dmngr,1)
    TRACEX("CHOL_SYNC",arg%this%TName)

  End Subroutine chol_task_sync


!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_diag_upd(arg)
    Type(chol_args), Intent(inout)               :: arg
    Integer                                      :: col,r,fr,cb,rb,tmp
    Real(kind=rfp) , Dimension ( :,: ) , Pointer :: A,C,B
    Real(kind=rfp)                               :: div,sm
    Type(tl_handle)                              :: left_col,cur_col,top_row,hnull
    Character (len=NAME_LEN)                     :: lname,tname,cname

    KERN(DIAG(TRACEX("CHOL_KERN,Diag. enter i,colsBlk",(arg%i,arg%fr,arg%tr,arg%fc,arg%tc))))
    


    A => arg%data1
    C => arg%data1
!    B => arg%data3

    If ( .Not. Associated ( A)  ) Then 
       KERN(DIAG(TRACEX("CHOL_KERN,Diag. A  associated",Associated (A))))
       Return 
    End If
    If ( .Not. Associated ( C)  ) Then 
       KERN(DIAG(TRACEX("CHOL_KERN,Diag. C  associated",Associated (C))))
       Return 
    End If

    TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,arg%fr) 

    Do col=arg%fc,arg%tc
       If ( col >1 ) Then 
          if ( arg%fr == 1 ) then 
            fr = col
          else
            fr = arg%fr
          end if 
          
          Do r = fr,arg%tr             
!             sm = 0
             C(r,col) = A(r,col) - Sum( A(r,arg%fc:col-1)*A(col,arg%fc:col-1) ) 
          End Do
!       Else
!          C(col,col)=A(col,col)
       End If
       If ( arg%fr == 1) Then 
          fr = col+1
       Else
          fr = arg%fr
       End If
!       KERN(DIAG(TRACEX("CHOL_KERN,Diag. off diag f:t,col",(fr,arg%tr,col))))
       C(col,col) = Sqrt ( abs(C(col,col)))    
       C(fr:arg%tr,col) = A(fr:arg%tr,col) / C(col,col)
       

    End Do
    

    TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,0) 
    !TRACEX("CholData:","CHOL_KERN,Diag")
    call chol_print_data( A, arg%rb,arg%cb)
    KERN(DIAG(TRACEX( "CHOL_KERN,Diag.","Exit")))
  End Subroutine chol_diag_upd

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_diag_upd_task(this)
    Type(dist_chol) , Intent(inout), Target :: this
    Type(chol_args)                         :: arg
    Integer                                 :: rows,cols,i,col,sync_reads,hreads,rb,cb,tmp
    Type(tl_handle)                         :: left_col,cur_col,top_row,hnull
    Type(tl_handle) , dimension(1:2)        :: rhandles
    Character (len=NAME_LEN)                :: lname,tname,cname
    Type(tl_handle), Dimension(:), Pointer  :: handles

    Allocate( handles(1:(this%nb+1)*(this%nb+1) ) ) 
    TASK(DIAG(TRACEX("CHOL_TASK,Diag.  cols,rows,nb",(this%cols,this%rows,this%nb))))
    sync_reads=0
    arg%this => this

    arg%data1=>this%data1
    arg%data2=>this%data2
    arg%data3=>this%data3

    rb=this%rows/this%nb
    cb=this%cols/this%nb
    arg%rb = rb
    arg%cb = cb
    Do col=1,this%cols,cb
       arg%pnlu_col = col
       arg%fc= col
       arg%tc=Min ( col+cb-1, this%cols)
       Do i = 1,this%nb+1
          arg%fr = (i-1)*rb+1
          If (arg%fr >this%rows) Exit
          arg%tr = Min ( (i  )*rb ,this%rows) 
          arg%i  = i
          hreads=0
          top_row = hnull
          left_col= hnull
          rhandles(:)=hnull
          tname="---------------"
          cname="---------------"
          lname="---------------"
          If ( i > 1 ) Then 
             If ((arg%fr+1) <= col .And. col <= arg%tr) Then 
                tname="+++++++++++++++"
             Else
                tmp = (col-1)/rb+1
                TASK(DIAG(TRACEX("CHOL_TASK,Diag.  top_r,txt",(tmp,(col-1)/rb))))
                tname = Trim(arg%this%writes(TASK_DIAG))//"_c_"//to_str2(tmp+48)//"_"//to_str4(col)
                top_row=dm_handle_get(this%dmngr,tname)
                If ( top_row%address /= hnull%address ) Then 
                   hreads=hreads+1
                   rhandles(hreads)=top_row
                   TASK(DIAG(TRACEX("CHOL_TASK,Diag.  top_row,hnd",(tname,top_row))))
                Else
                   tname(1:4)="++++"
                End If
             end if 
          End If
          If ( col >1 ) Then 
                lname = Trim(arg%this%writes(TASK_DIAG))//"_c_"//to_str2(arg%i+48)&
                                                                                //"_"//to_str4(col-cb)
                left_col=dm_handle_get(this%dmngr,lname)
                hreads=hreads+1
                rhandles(hreads)=left_col
                TASK(DIAG(TRACEX("CHOL_TASK,Diag.  left_col,hnd",(lname,left_col))))
          End If

          cname = Trim(arg%this%writes(TASK_DIAG))//"_c_"//to_str2(arg%i+48)//"_"//to_str4(col)
          If (col > arg%tr) Then 
             cname(1:4)="****"
          Else
             Call dm_matrix_add( this%dmngr, cname)
             cur_col=dm_handle_get(this%dmngr,cname)
             TASK(DIAG(TRACEX("CHOL_TASK,Diag.  cur_col,hnd",(cname,cur_col))))
             call tl_add_task_named(TASK_DIAG_NAME,chol_diag_upd,arg,sizeof(arg),rhandles,hreads,cur_col,1,0,0,5)       
          End If
          TASK(DIAG(TRACEX( "CHOL_TASK,Diag.TaskAddName",(rhandles,hreads))))
          TASK(DIAG(TRACEX( "CHOL_TASK,Diag.Dependencies",("               ",tname              ))))
          TASK(DIAG(TRACEX( "CHOL_TASK,Diag.Dependencies",(lname            ,cname,arg%fr,arg%tr))))
       End Do
       If ( col >= this%cols-cb) Then 
          sync_reads = sync_reads +1
          handles(sync_reads) = cur_col
       End If
    End Do

    If ( sync_reads /=0 ) Then 
       call tl_add_task_named("sync"//char(0),chol_task_sync, arg, sizeof(arg), handles, sync_reads , 0, 0, 0, 0,5)
       TASK(DIAG(TRACEX("CHOL_TASK,Diag.TaskAddName,#sync",(sync_reads,handles))))
    End If

    Deallocate ( handles ) 
    TASK(DIAG(TRACEX("CHOL_TASK,Diag.","Exit")))

  End Subroutine chol_diag_upd_task

!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_pnlu_local(arg)
    Type(chol_args), Intent(inout)               :: arg
    Real(kind=rfp) , Dimension ( :,: ) , Pointer :: A,B
    Integer                                      :: col,r,fr,cb
    Character (len=NAME_LEN)                     :: name
    Type(tl_handle)                              :: left_col,cur_col


!!$     x
!!$ B   x x
!!$     x x x   ----
!!$                 -> A
!!$     x x x   ----
!!$ A   x x x
!!$     x x x
!!$

    KERN(PNLU(TRACEX( "CHOL_KERN,Pnlu.  enter",(arg%fr,arg%tr,arg%fc,arg%tc))))
    
    A => arg%data1
    B => arg%data2 

    If ( .Not. Associated (A)  ) Return 
    If ( .Not. Associated (B)  ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,0) 
    Do col=arg%fc,arg%tc
       
       !KERN(PNLU(TRACEX("CHOL_KERN,Pnlu.  c,f:t",(col,",",arg%fr,arg%tr,"x",arg%fc,arg%tc))))
       If ( col >arg%fc ) Then 
          Do r = arg%fr,arg%tr             
             A(r,col) = A(r,col) - Sum( A(r,arg%fc:col-1)*B(col,arg%fc:col-1) ) 
          End Do
       End If
       
       A(arg%fr:arg%tr,col) = A(arg%fr:arg%tr,col)/ B(col,col)

    End Do
    TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,0) 
    call chol_print_data( A, arg%rb,arg%cb)
!    call chol_print_data( B, arg%rb,arg%cb)
    KERN(PNLU(TRACEX( "CHOL_KERN,Pnlu.",("Exit",arg%fr,arg%tr,"x",arg%fc,arg%tc))))

  end subroutine chol_mat_pnlu_local

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_pnlu(arg)
    Type(chol_args), Intent(inout)               :: arg
    real(kind=rfp) , dimension ( :,: ) , pointer :: A,B,Y
    Integer                                      :: col,r,fr,cb
    Character (len=NAME_LEN)                     :: name
    Type(tl_handle)                              :: left_col,cur_col


!!$     x
!!$ B   x x
!!$     x x x   ----
!!$                 -> A
!!$     x x x   ----
!!$ A   x x x
!!$     x x x
!!$

    KERN(PNLU(TRACEX( "CHOL_KERN,Pnlu.  enter",(arg%fr,arg%tr,arg%fc,arg%tc))))
    
    A => arg%data1
    B => arg%data2 
    Y => arg%data3

    If ( .Not. Associated (A)  ) Return 
    If ( .Not. Associated (B)  ) Return 
    If ( .Not. Associated (Y)  ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,0) 
    Do col=arg%fc,arg%tc
       
       !KERN(PNLU(TRACEX("CHOL_KERN,Pnlu.  c,f:t",(col,",",arg%fr,arg%tr,"x",arg%fc,arg%tc,Y(:,1)))))
       If ( col >1 ) Then 
          Do r = arg%fr,arg%tr             
             Y(r,1) = Y(r,1)+  ( A(r,col-1)*B(col,col-1) ) 
             A(r,col) = A(r,col) -Y(r,1)
          End Do
       End If
       
       A(arg%fr:arg%tr,col) = A(arg%fr:arg%tr,col)/ B(col,col)

    End Do
    TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,0) 
!    TRACEX("CholData:","CHOL_KERN,Pnlu")
    call chol_print_data( A, arg%rb,arg%cb)
    KERN(PNLU(TRACEX( "CHOL_KERN,Pnlu.",("Exit",arg%fr,arg%tr,"x",arg%fc,arg%tc))))

  End Subroutine chol_mat_pnlu

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_pnlu_task(this)
    Type(dist_chol) , Intent(inout), Target  :: this
    Type(tl_handle) , Dimension(:) , Pointer :: handles
    Type(chol_args)                          :: arg
    Integer                                  :: rows,cols,i,col,sync_reads,rb,cb
    Type(tl_handle)                          :: left_col,cur_col
    Character (len=NAME_LEN)                 :: name


    Allocate( handles(1:(this%nb+1)*(this%nb+1) ) ) 

    TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.  cols,rows,nb",(this%cols,this%rows,this%nb))))

    sync_reads=0
    arg%this => this
    arg%data1=>this%data1
    arg%data2=>this%data2
    allocate ( arg%data3(this%rows,1) ) 
    arg%data3(:,:)=0

    rb=this%rows/this%nb 
    cb=this%cols/this%nb
    arg%rb = rb 
    arg%cb = cb
    Do col=1,this%cols,cb
       arg%fc = col
       arg%tc = Min( col+ cb-1,this%cols)
       Do i = 1,this%nb+1
          arg%fr = (i-1)*rb+1
          If (arg%fr >this%rows) Exit
          arg%tr = Min ( (i  )*rb , this%rows) 
          arg%i = i
          If ( col >1 ) Then 

             name = Trim(arg%this%writes(TASK_PNLU))//"_c_"//to_str2(arg%i+48)&
                                                                            //"_"//to_str4(col-cb)
             left_col=dm_handle_get(this%dmngr,name)
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.  left_col",(name,left_col))))

             name = Trim(arg%this%writes(TASK_PNLU))//"_c_"//to_str2(arg%i+48)&
                                                                //"_"//to_str4(col)
             Call dm_matrix_add( this%dmngr, name)
             cur_col=dm_handle_get(this%dmngr,name)
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.  cur_col",(name,cur_col))))

             call tl_add_task_named(TASK_PNLU_NAME,chol_mat_pnlu,arg,sizeof(arg),left_col,1,cur_col,1,0,0,5)       
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.TaskAddName",(arg%i,col,arg%fr,arg%tr,arg%fc,arg%tc))))

          Else
             name = Trim(arg%this%writes(TASK_PNLU))//"_c_"//to_str2(arg%i+48)//"_"//to_str4(col)
             Call dm_matrix_add( this%dmngr, name)
             cur_col=dm_handle_get(this%dmngr,name)
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.  cur_col",(name,cur_col))))

             call tl_add_task_named(TASK_PNLU_NAME,chol_mat_pnlu,arg,sizeof(arg),0,0,cur_col,1,0,0,5)                    
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.TaskAddName",(arg%i,col,arg%fr,arg%tr,arg%fc,arg%tc))))
          End If
          If ( col >= this%cols -cb) Then 
             sync_reads=sync_reads+1
             TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.Handles#",sync_reads)))
             If ( sync_reads <= Ubound(handles,1) ) Then 
                handles(sync_reads) = cur_col
             Else
                TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.Handles limit exceeded",sync_reads)))
             End If
          End If
       End Do
    End Do

    arg%dealloc = 1
    call tl_add_task_named("sync"//char(0),chol_task_sync, arg, sizeof(arg), handles,sync_reads, 0, 0, 0, 0,5)
    TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.Sync Task",(sync_reads,handles))))
    Deallocate ( handles ) 
    TASK(PNLU(TRACEX("CHOL_TASK,Pnlu.","Exit")))

  End Subroutine chol_mat_pnlu_task

!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_mul_local(arg)
    Type(chol_args), Intent(inout)             :: arg
    Real(kind=rfp) , Dimension(:,:) , Pointer  :: A,B,C
    Integer                                    :: fr,tr,fc,tc,fr2,tr2,fc2,tc2

    fr = arg%fr
    tr = arg%tr
    fc = arg%fc
    tc = arg%tc    

    fr2 = arg%fr2
    tr2 = arg%tr2
    fc2 = arg%fc2
    tc2 = arg%tc2   
    KERN(MMUL(TRACEX("CHOL_KERN,MMul. enter (1) f:t,f:t",(fr ,tr ,fc ,tc ))))
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.  (2) f:t,f:t",(fr2,tr2,fc2,tc2))))
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.  mul overwrt",arg%mul_overwrite)))

    A => arg%data1
    B => arg%data2
    C => arg%data3

    If ( .Not. Associated (A) ) Return 
    if ( .not. associated (B) ) return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,0) 


    if ( arg%mul_overwrite == 1 ) then 
       C(fr:tr,fc2:tc2)= matmul(A(fr:tr,fc:tc) ,transpose(B(fr2:tr2,fc:tc)) )
    else
       C(fr:tr,fc2:tc2)= C(fr:tr,fc2:tc2) + matmul(A(fr:tr,fc:tc) ,transpose(B(fr2:tr2,fc:tc)) )
    end if

    TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,0) 
!    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.",(C(arg%fr:arg%tr,arg%fc2:arg%tc2)))))
    CHOL_PRT(TRACEX("CholData:","CHOL_KERN,MMul_local"))
    call chol_print_data( C, arg%rb,arg%cb)
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.","Exit")))

  end subroutine chol_mat_mul_local
!-----------------------------------------------------------------------------------------------------

  Subroutine chol_mat_mul(arg)
    Type(chol_args), Intent(inout)             :: arg
    Real(kind=rfp) , Dimension(:,:) , Pointer  :: A,B,C
    Integer                                    :: fr,tr,fc,tc,fr2,tr2,fc2,tc2

    fr = arg%fr
    tr = arg%tr
    fc = arg%fc
    tc = arg%tc    

    fr2 = arg%fr2
    tr2 = arg%tr2
    fc2 = arg%fc2
    tc2 = arg%tc2   
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.  (1) f:t,f:t",(fr ,tr ,fc ,tc ))))
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.  (2) f:t,f:t",(fr2,tr2,fc2,tc2))))

    A => arg%data1
    B => arg%data2
    C => arg%data3

    If ( .Not. Associated (A) ) Return 
    If ( .Not. Associated (B) ) Return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,0) 


    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.-",(C(arg%fr:arg%tr,arg%fc2:arg%tc2)))))
    C(fr:tr,fc2:tc2)= C(fr:tr,fc2:tc2) + matmul(transpose(B(fr2:tr2,fc2:tc2)),A(fr:tr,fc:tc)  ) 


    TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,0) 
    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.",(A(arg%fr:arg%tr,arg%fc:arg%tc)))))
    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.",(B(arg%fr2:arg%tr2,arg%fc2:arg%tc2)))))
    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.",(C(arg%fr:arg%tr,arg%fc2:arg%tc2)))))
    KERN(MMUL(TRACEX("CHOL_KERN,Mmul.",(arg%fr,arg%tr,arg%fc2,arg%tc2))))
 !   TRACEX("CholData:","CHOL_KERN,MMul")
    call chol_print_data( C, arg%rb,arg%cb)
    KERN(MMUL(TRACEX("CHOL_KERN,MMul.","Exit")))

  End Subroutine chol_mat_mul

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_mul_task(this)
    Type(dist_chol) , Intent(inout), Target  :: this
    Type(tl_handle) , Dimension(:) , Pointer :: handles
    Type(chol_args)                          :: arg
    integer                                  :: rows,cols,i,j,k,sync_reads,rb,cb,hcnt,rbno,cbno
    Character (len=NAME_LEN)                 :: name
    Type(tl_handle) ,dimension(1:1)          :: blks

    rows = this%rows
    cols = this%cols
    hcnt=(this%nb+1)*(this%nb+1)
    Allocate(handles(1:hcnt))
    TASK(MMUL(TRACEX("CHOL_TASK,MMul.  cols,rows,nb,hndl#",(this%cols,this%rows,this%nb,hcnt ))))
    
    sync_reads=0
    arg%this => this

    arg%data1=>this%data1
    arg%data2=>this%data2
    arg%data3=>this%data3

    rb=this%rows/this%nb
    cb=this%cols/this%nb
    arg%rb = rb 
    arg%cb = cb
    rbno = this%rows/rb
    if ( mod (this%rows,this%nb) /= 0 ) rbno= rbno+1
    cbno = this%cols/cb
    if ( mod (this%cols,this%nb) /= 0 ) cbno= cbno+1
    hcnt = 0
    TASK(MMUL(TRACEX("CHOL_TASK,MMul.  rb,cb,rbno,cbno",(rb,cb,rbno,cbno))))
    Do i=1,this%nb+1
       arg%fr = (i-1)*rb+1
       If (arg%fr >this%rows) Exit
       arg%tr = Min ( (i  )*rb , rows) 
       arg%i=i
       Do j=1,this%nb+1
          arg%j=j
          arg%fc2 = (j-1)*cb+1
          If (arg%fc2 >this%cols) Exit
          arg%tc2 = Min ( (j  )*cb , cols)                
          name = Trim(this%writes(TASK_MMUL))//"_blk_"//to_str2(i+48)&
                                                                   //"_"//to_str2(j+48)
          Call dm_matrix_add(this%dmngr,name)
          blks(1) = dm_handle_get(this%dmngr,name)
          hcnt = hcnt + 1
          handles(hcnt)=blks(1)
          TASK(MMUL(TRACEX("CHOL_TASK,MMul.  blk,hnd",(name,blks(1),hcnt))))
          ! Cij= Sum ( Aik * Bkj ; for k = 1,nb)
          Do k=1,this%nb+1
             arg%fc  =       (k-1)*cb+1
             arg%tc  = Min ( (k  )*cb  , cols) 
             arg%fr2 =       (k-1)*rb+1
             arg%tr2 = Min ( (k  )*rb  , rows)                
             If (arg%fc  >this%cols) Exit
             If (arg%fr2 >this%rows) Exit
             TASK(MMUL(TRACEX("CHOL_TASK,MMul.  [1] f:t,f:t",(arg%fr ,arg%tr ,arg%fc ,arg%tc ))))
             TASK(MMUL(TRACEX("CHOL_TASK,MMul.  [2] f:t,f:t",(arg%fr2,arg%tr2,arg%fc2,arg%tc2))))
             TASK(MMUL(TRACEX("CHOL_TASK,MMul.TaskAddName. i,j,k, RdWr handle",(i,j,k,blks))))
             call tl_add_task_named(TASK_MMUL_NAME,chol_mat_mul,arg,sizeof(arg),0,0,blks,1,0,0,5)       
          End Do
       End Do
    End Do

    TASK(MMUL(TRACEX("CHOL_TASK,MMul.TaskAddName",Size(handles))))
    call tl_add_task_named("sync"//char(0),chol_task_sync, arg, sizeof(arg), handles, hcnt, 0, 0, 0, 0,5)

    TASK(MMUL(TRACEX("CHOL_TASK,MMul. sync-handles",(handles,hcnt))))

    Deallocate ( handles ) 
    TASK(MMUL(TRACEX("CHOL_TASK,MMul.","Exit")))

  End Subroutine chol_mat_mul_task

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_add(arg)
    Type(chol_args), Intent(inout)             :: arg
    Real(kind=rfp) , Dimension(:,:) , Pointer  :: A,B,C
    Integer                                    :: fr,tr,fc,tc

    fr = arg%fr
    tr = arg%tr
    fc = arg%fc
    tc = arg%tc    
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.  [2] f:t,f:t",(fr,tr,fc,tc))))

    A => arg%data1
    B => arg%data2
    C => arg%data3
    
    If ( .Not. Associated (A) ) Return 
    If ( .Not. Associated (B) ) Return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,0) 
    
    C(fr:tr,fc:tc) = B(fr:tr,fc:tc) - A(fr:tr,fc:tc) 


    TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,0) 
!    KERN(MADD(TRACEX("CHOL_KERN,MAdd.",(C(arg%fr:arg%tr,arg%fc:arg%tc)))))
!    TRACEX("CholData:","CHOL_KERN,MAdd")
    call chol_print_data( C, arg%rb,arg%cb)
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.","Exit")))


  End Subroutine chol_mat_add
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_mat_add_local(arg)
    Type(chol_args), Intent(inout)             :: arg
    Real(kind=rfp) , Dimension(:,:) , Pointer  :: A,B,C
    integer                                    :: fr,tr,fc,tc,fc2,tc2

    fr = arg%fr
    tr = arg%tr
    fc = arg%fc
    tc = arg%tc    
    fc2 = arg%fc2
    tc2 = arg%tc2    
    KERN(MADD(TRACEX("CHOL_KERN,MAdd. enter  [1] f:t,f:t",(fr,tr,fc,tc  ))))
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.  [2] f:t,f:t",(fr,tr,fc2,tc2))))

    A => arg%data1
    B => arg%data2
    C => arg%data3
    
    If ( .Not. Associated (A) ) Return 
    If ( .Not. Associated (B) ) Return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,0) 

    CHOL_PRT(TRACEX("CholData:","CHOL_KERN,MAdd_local B"))
    call chol_print_data( B, arg%rb,arg%cb)
    
    C(fr:tr,fc:tc) = B(fr:tr,fc:tc) - A(fr:tr,fc2:tc2) 


    TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,0) 
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.pSum",(sum(C(arg%fr:arg%tr,arg%fc:arg%tc)) , fr,tr,fc,tc ) )))
!    KERN(MADD(TRACEX("CHOL_KERN,MAdd.pSum",(sum(A(arg%fr:arg%tr,arg%fc2:arg%tc2)) , fr,tr,fc2,tc2 ) )))
    CHOL_PRT(TRACEX("CholData:","CHOL_KERN,MAdd_local A"))
    call chol_print_data( A, arg%rb,arg%cb)
    CHOL_PRT(TRACEX("CholData:","CHOL_KERN,MAdd_local C"))
    call chol_print_data( C, arg%rb,arg%cb)
    KERN(MADD(TRACEX("CHOL_KERN,MAdd.","Exit")))


  end subroutine chol_mat_add_local


!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_add_task(this)
    Type(dist_chol) , Intent(inout), Target  :: this
    Type(tl_handle) , Dimension(:) , Pointer :: handles
    Type(chol_args)                          :: arg
    Integer                                  :: rows,cols,i,j,rb,cb,hcnt
    Character (len=NAME_LEN)                 :: name
    Type(tl_handle)                          :: blks


    rows = this%rows
    cols = this%cols

    TASK(MADD(TRACEX("CHOL_TASK,MAdd.  cols,rows,nb",(this%cols,this%rows,this%nb))))

    Allocate(handles(1:(this%nb+1)*(this%nb+1) ) ) 
    
    arg%this => this

    arg%data1=>this%data1
    arg%data2=>this%data2
    arg%data3=>this%data3

    rb=this%rows/this%nb
    cb=this%cols/this%nb
    arg%rb = rb 
    arg%cb = cb
    hcnt =0
    Do i=1,this%nb+1
       arg%fr = (i-1)*rb+1
       If (arg%fr >this%rows) Exit
       arg%tr = Min ( (i  )*rb , rows) 
       Do j=1,this%nb+1
          arg%fc = (j-1)*cb+1
          If (arg%fc >this%cols) Exit
          arg%tc = Min ( (j  )*cb , cols)
          name = Trim(this%writes(TASK_MADD))//"_blk_"//to_str2(i+48)&
                                                                   //"_"//to_str2(j+48)
          Call dm_matrix_add(this%dmngr,name)
          blks = dm_handle_get(this%dmngr,name)
          hcnt = hcnt + 1
          handles(hcnt)=blks
          TASK(MADD(TRACEX("CHOL_TASK,MAdd.  blk,hnd",(name,blks))))
          TASK(MADD(TRACEX("CHOL_TASK,MAdd.  f:t,f:t",(arg%fr,arg%tr,arg%fc,arg%tc))))
          call tl_add_task_named(TASK_MADD_NAME,chol_mat_add,arg,sizeof(arg),0,0,blks,1,0,0,5)       
          TASK(MADD(TRACEX("CHOL_TASK,MAdd.TaskAddName",TASK_MADD_NAME)))
       end do
    End Do

    call tl_add_task_named("sync"//char(0),chol_task_sync, arg, sizeof(arg), handles, hcnt, 0, 0, 0, 0,5)
    TASK(MADD(TRACEX("CHOL_TASK,MAdd.TaskAddName",hcnt)))

    TASK(MADD(TRACEX("CHOL_TASK,MAdd. sync-handles",handles)))

    Deallocate ( handles ) 
    TASK(MADD(TRACEX("CHOL_TASK,MAdd.","Exit")))

  End Subroutine chol_mat_add_task

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_xadd(arg)
    Type(chol_args), Intent(inout)             :: arg
    Real(kind=rfp) , Dimension (:,:) , Pointer :: A,C
    Integer                                    :: fr,tr,fc,tc


    fr = arg%fr
    tr = arg%tr
    fc = arg%fc
    tc = arg%tc    
    KERN(XADD(TRACEX("CHOL_KERN,XAdd.  f:t,f:t",(fr,tr,fc,tc))))

    A => arg%data1
    C => arg%data3
    
    If ( .Not. Associated (A) ) Return 
    If ( .Not. Associated (C) ) Return 

    TIMING(EVENT_SUBTASK_STARTED , TASK_XADD_NAME,5,0) 

    
    C(fr:tr,fc:tc) = C(fr:tr,fc:tc) + A(fr:tr,fc:tc) 


    TIMING(EVENT_SUBTASK_FINISHED , TASK_XADD_NAME,5,0) 

!    KERN(XADD(TRACEX("CHOL_KERN,XAdd.",(C(arg%fr:arg%tr,arg%fc:arg%tc)))))
!    TRACEX("CholData:","CHOL_KERN,XAdd")
    call chol_print_data(C, arg%rb,arg%cb)
    KERN(XADD(TRACEX("CHOL_KERN,XAdd.","Exit")))


  End Subroutine chol_mat_xadd

!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_mat_xadd_task(this)
    Type(dist_chol) , Intent(inout), Target  :: this
    Type(tl_handle) , Dimension(:) , Pointer :: handles
    Type(chol_args)                          :: arg
    Integer                                  :: rows,cols,i,j,rb,cb,hcnt
    Character (len=NAME_LEN)                 :: name
    Type(tl_handle)                          :: blks

    rows = this%rows
    cols = this%cols
    Allocate(handles(1:(this%nb+1)*(this%nb+1) ))
    TASK(XADD(TRACEX( "CHOL_TASK,XAdd.  cols,rows,nb",(this%cols,this%rows,this%nb,Size(handles)))))
    
    
    arg%this => this

    arg%data1=>this%data1
    arg%data2=>this%data2
    arg%data3=>this%data3

    rb=this%rows/this%nb
    cb=this%cols/this%nb
    arg%rb = rb 
    arg%cb = cb

    hcnt = 0
    Do i=1,this%nb+1
       arg%fr = (i-1)*rb+1
       If (arg%fr >this%rows) Exit
       arg%tr = Min ( (i  )*rb , rows) 
       Do j=1,this%nb+1
          arg%fc = (j-1)*cb+1
          If (arg%fc >this%cols) Exit
          arg%tc = Min ( (j  )*cb , cols)
          name = Trim(this%writes(TASK_XADD))//"_blk_"//to_str2(i+48)&
                                                                   //"_"//to_str2(j+48)
          Call dm_matrix_add(this%dmngr,name)
          blks = dm_handle_get(this%dmngr,name)
          hcnt = hcnt +1 
          handles(hcnt)=blks
          TASK(XADD(TRACEX("CHOL_TASK,XAdd.  blk",(name,blks))))
          TASK(XADD(TRACEX("CHOL_TASK,XAdd.  f:t,f:t",(arg%fr,arg%tr,arg%fc,arg%tc))))
          call tl_add_task_named(TASK_XADD_NAME,chol_mat_xadd,arg,sizeof(arg),0,0,blks,1,0,0,5)       
          TASK(XADD(TRACEX("CHOL_TASK,XAdd.TaskAddName",TASK_XADD_NAME)))
       end do
    End Do

    call tl_add_task_named("sync"//char(0),chol_task_sync, arg, sizeof(arg), handles, hcnt, 0, 0, 0, 0,5)
    TASK(XADD(TRACEX("CHOL_TASK,XAdd.TaskAddName",hcnt)))

    TASK(XADD(TRACEX("CHOL_TASK,XAdd.  sync-handles",handles)))

    Deallocate ( handles ) 

    TASK(XADD(TRACEX("CHOL_TASK,XAdd.","Exit")))
    
  End Subroutine chol_mat_xadd_task

!!$------------------------------------------------------------------------------------------------------------------------
  Function  dist_chol_new(nb,tname) Result (this)
    Type(dist_chol)         , Pointer    :: this
    Integer                 , Intent(in) :: nb
    Character(len=*) , Intent(in) :: tname
    
    Allocate(this ) 
    this%nb = nb
    this%tname = tname
    this%writes(TASK_XADD) = TASK_XADD_NAME
    this%writes(TASK_MADD) = TASK_MADD_NAME
    this%writes(TASK_DIAG) = TASK_DIAG_NAME
    this%writes(TASK_PNLU) = TASK_PNLU_NAME
    this%writes(TASK_MMUL) = TASK_MMUL_NAME
    

  End Function dist_chol_new



!!$------------------------------------------------------------------------------------------------------------------------
  Subroutine chol_sequential(mat)
!    Type(dist_chol) , Intent(inout), Target  :: this
    Real(kind=rfp) , Dimension ( :,: ) , Pointer ,intent(inout):: mat
    integer                                      :: col,row,fr,tr,tc,fc
    Real(kind=rfp) , Dimension ( :,: ) , Pointer :: A,C,B
    Real(kind=rfp)                               :: div,sm
    
    A => mat
    C => mat
    If ( .Not. Associated ( mat)  ) Then 
       TRACEX("CHOL_SEQ,Diag. mat  associated",Associated (mat))
       Return 
    End If
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
   


  End Subroutine chol_sequential
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
          SEQ(TRACEX("SeqBlock. Loop cb,rb, ranges",(cbidx,rbidx,fr,tr,fc,tc)))
!!$          if ( cbidx > 1 ) then 
!!$             ! add X to mat
!!$             SEQ(TRACEX("SeqBlock. Subtract X from Mat",(fr,tr)))
!!$             TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,0) 
!!$             mat (fr:tr,fc:tc) = mat(fr:tr,fc:tc) - Xmat(fr:tr,:)
!!$             TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,0) 
!!$             call chol_print_data( mat,rb,cb ) 
!!$          end if
          if ( rbidx > cbidx ) then 
             ! panel update
             SEQ(TRACEX("SeqBlock. Panel update",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,0) 
             do c = fc,tc
                if ( c > fc ) then 
                   do r=fr,tr
                      SEQ(TRACEX("SeqBlock. Panel update inner loop ",(r,c)))
                      SEQ(TRACEX("SeqBlock.",("mat(", c,',',fc,':',c-1,")*mat(",r,',',fc,':',c-1,"))")))
                      SEQ(TRACEX("SeqBlock.",(sum(mat( c,fc:c-1)*mat(r,fc:c-1)))))
                      mat(r,c) = mat(r,c) - sum(mat( c,fc:c-1)*mat(r,fc:c-1))
                   end do
                end if
                call chol_print_data( mat,rb,cb ) 
                mat ( fr:tr,c) =  mat ( fr:tr,c) / mat ( c,c)                   
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,0) 
             TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,0) 
             ! gemm tasks
             fr2 = (cbidx)*rb +1 
             tr2 = min((cbidx+1)*rb , nrows)
             SEQ(TRACEX("SeqBlock. gemm update ",(fr2,tr2)))
             Xmat(fr:tr,fc:tc) = matmul(  mat (fr:tr,fc:tc) , transpose(mat (fr2:tr2,fc:tc)) )
             do cbidx2=cbidx,rbidx
                fc2 =  cbidx2   *cb + 1
                tc2 = (cbidx2+1)*cb
                mat(fr:tr,fc2:tc2) = mat(fr:tr,fc2:tc2) - Xmat(fr:tr,fc:tc)
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,0) 
             call chol_print_data( mat ,rb,cb ) 
             call chol_print_data( Xmat,rb,cb ) 
          end if
          if (rbidx == cbidx) then 
             ! diagonal block 
             SEQ(TRACEX("SeqBlock. Diagonal update",(fc,tc)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,fr) 
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
             TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,0) 
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
          SEQ(TRACEX("SeqBlock. Loop cb,rb, ranges",(cbidx,rbidx,fr,tr,fc,tc)))
          if ( cbidx > 1 ) then 
             ! add X to mat
             SEQ(TRACEX("SeqBlock. Subtract X from Mat",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_MADD_NAME,4,0) 
             mat (fr:tr,fc:tc) = mat(fr:tr,fc:tc) - Xmat(fr:tr,:)
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MADD_NAME,4,0) 
             call chol_print_data( mat,rb,cb ) 
          end if
          if ( rbidx > cbidx ) then 
             ! panel update
             SEQ(TRACEX("SeqBlock. Panel update",(fr,tr)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_PNLU_NAME,2,0) 
             do c = fc,tc
                if ( c > fc ) then 
                   do r=fr,tr
                      SEQ(TRACEX("SeqBlock. Panel update inner loop ",(r,c)))
                      SEQ(TRACEX("SeqBlock.",("mat(", c,',',fc,':',c-1,")*mat(",r,',',fc,':',c-1,"))")))
                      SEQ(TRACEX("SeqBlock.",(sum(mat( c,fc:c-1)*mat(r,fc:c-1)))))
                      mat(r,c) = mat(r,c) - sum(mat( c,fc:c-1)*mat(r,fc:c-1))
                   end do
                end if
                call chol_print_data( mat,rb,cb ) 
                mat ( fr:tr,c) =  mat ( fr:tr,c) / mat ( c,c)                   
             end do
             TIMING(EVENT_SUBTASK_FINISHED , TASK_PNLU_NAME,2,0) 
             TIMING(EVENT_SUBTASK_STARTED , TASK_MMUL_NAME,3,0) 
             ! gemm tasks
             fr2 = (cbidx)*rb +1 
             tr2 = min((cbidx+1)*rb , nrows)
             SEQ(TRACEX("SeqBlock. gemm update ",(fr2,tr2)))
             Xmat(fr:tr,:) = Xmat(fr:tr,:) +  matmul(  mat (fr:tr,fc:tc) , transpose(mat (fr2:tr2,fc:tc)) )
             TIMING(EVENT_SUBTASK_FINISHED , TASK_MMUL_NAME,3,0) 
             call chol_print_data( mat ,rb,cb ) 
             call chol_print_data( Xmat,rb,cb ) 
          end if
          if (rbidx == cbidx) then 
             ! diagonal block 
             SEQ(TRACEX("SeqBlock. Diagonal update",(fc,tc)))
             TIMING(EVENT_SUBTASK_STARTED , TASK_DIAG_NAME,1,fr) 
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
             TIMING(EVENT_SUBTASK_FINISHED , TASK_DIAG_NAME,1,0) 
             call chol_print_data( mat,rb,cb ) 
          end if 
       end do ! row block idx
    end do ! col block idx


  end subroutine chol_sequential_cache_opt

!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_print_data ( mat,rb,cb )
    real(kind=rfp) , dimension ( :,: ) , pointer ,intent(inout):: mat
    integer , intent(in) :: rb,cb
    integer :: r,c
    
    !CHOL_NOPRT(return ) 
    
    if ( cb > 0 ) then 
       if ( ubound(mat,1) > 13 ) return 
       if ( ubound(mat,2) > 13 ) return 
    end if

    !TRACEX("CHOL_PRINT",(ubound(mat,1),ubound(mat,2),rb,cb))
    do r = 1,ubound(mat,1)
       write ( *,"(A)",advance='no') "CholData:"
       do c = 1,ubound(mat,2)
          !if ( c>r ) exit
          write ( *,"(F5)",advance='no') mat(r,c)
          if (mod(c,cb ) ==0 )  write ( *,"(A)",advance='no') "|" 
       end do
       write (*,*) ""
       if ( mod ( r,rb) == 0 ) then 
          write (*,*) "CholData:----------------------------------------------------------------------|"
       end if
    end do
    
  end subroutine chol_print_data
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_diag_task_cache_opt(this ) 
    type(dist_chol),intent(inout),pointer :: this
    type(chol_args) :: arg
    real(kind=rfp) , dimension ( :,: ) , pointer :: mat
    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
    type(chol_args) :: arg
    integer :: nrows,ncols ! number of columns and rows 
    integer :: rb   ,cb    ! number of rows or columns in block 
    integer :: rbno ,cbno  ! total number of blocks in rows or columns
    integer :: rbidx,cbidx ! index for row and column blocks 
    integer :: fr,tr,fc,tc ! range of rows and column in blocks 
    integer :: fr2,tr2     ! row range for panel updating
    integer :: r,c         ! index for row and columns in blocks
    type(tl_handle )    :: hRead,hWrite,hAdd
    type(tl_handle) , dimension( 2) :: hReads
    


    mat => this%data1
 
    nrows = this%rows
    ncols = this%cols
    TRACEX("nb",this%nb)
    rb=nrows / this%nb
    cb=ncols / this%nb
    if ( ncols < 32 ) then 
       rbno = 4 
       cbno = 4
       rb = nrows / rbno
       cb = ncols / cbno
    end if

    allocate ( Xmat ( 1:nrows, 1:cb) ) 
    Xmat(:,:) = 0  
    this%nb = rb

    rbno = nrows / rb + 1 
    cbno = ncols / cb + 1 



    TRACEX("CholTask.Diag BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          hAdd  = chol_get_handle(this,'M',rbidx,cbidx,.true.)
          hAdd  = chol_get_handle(this,'X',rbidx,cbidx,.true.)
       end do
    end do
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          arg%rb=rb
          arg%cb=cb
          arg%this => this
          arg%fr = (rbidx-1) * rb+1
          arg%tr = min(rbidx*rb,nrows)
          arg%fc = (cbidx-1)*cb+1
          arg%tc = min(cbidx*cb,ncols)
          TASK(DIAG(TRACEX("CholTask.Diag  Loop cb,rb, ranges",(cbidx,rbidx, & 
               arg%fr,arg%tr,arg%fc,arg%tc))))
          if ( cbidx > 1 ) then 
             ! add X to mat
             ! read handle X( rbidx,cbidx) , add handle M(rbidx,cbidx)
             ! MADD: C=B-A
             hRead = chol_get_handle(this,'X',rbidx,cbidx)
             hAdd  = chol_get_handle(this,'M',rbidx,cbidx)
             arg%data1 => Xmat
             arg%data2 => mat
             arg%data3 => mat
             arg%fc2= 1
             arg%tc2= ubound(Xmat,2)
             TASK(DIAG(TRACEX("CholTask.Diag. MADD handle",(hRead,hAdd)) ))
             call tl_add_task_named(TASK_MADD_NAME,chol_mat_add_local, arg,sizeof(arg) , hRead,1 , hAdd,1,0,0, 5)
             !mat (fr:tr,fc:tc) = mat(fr:tr,fc:tc) - Xmat(fr:tr,:)
          end if
          if ( rbidx > cbidx ) then 
             ! panel update
             ! B d2 diag, A d1 
             ! read handle M(cbidx,cbidx) , add handle M(rbidx,cbidx)
             hRead = chol_get_handle(this,'M',cbidx,cbidx)
             hAdd  = chol_get_handle(this,'M',rbidx,cbidx)
             arg%data1 => mat
             arg%data2 => mat

             TASK(DIAG(TRACEX("CholTask.Diag. PNLU handle",(hRead,hAdd)) ))
             call tl_add_task_named(TASK_PNLU_NAME//char(0),chol_mat_pnlu_local, arg, sizeof(arg), hRead,1,  hAdd,1,0,0,5)
             ! gemm tasks
             ! C=C+AB
             ! read handle M(cbidx+1,cbidx), M(rbidx,cbidx), add handle X(rbidx,cbidx+1)
             hReads(1) = chol_get_handle(this,'M',cbidx+1,cbidx  )
             hReads(2) = chol_get_handle(this,'M',rbidx  ,cbidx  )
             hAdd      = chol_get_handle(this,'X',rbidx  ,cbidx+1)
             arg%fr2 =     (cbidx  )*rb+1 
             arg%tr2 = min((cbidx+1)*rb  , nrows)
             TASK(DIAG(TRACEX("CholTask.Diag. gemm update ",(arg%fr2,arg%tr2))))
             arg%fc2= 1
             arg%tc2= ubound(Xmat,2)
             arg%data1 => mat
             arg%data2 => mat
             arg%data3 => Xmat

             TASK(DIAG(TRACEX("CholTask.Diag. MMUL handle",(hReads,hAdd)) ))
             call tl_add_task_named( TASK_MMUL_NAME , chol_mat_mul_local , arg, sizeof(arg) , hReads,2,  hAdd,1 ,0,0, 5)
             !Xmat(fr:tr,:) = Xmat(fr:tr,:) +  matmul(  mat (fr:tr,fc:tc) , mat (fr2:tr2,fc:tc) )

          end if
          if (rbidx == cbidx) then 
             hAdd = chol_get_handle ( this,'M',rbidx,cbidx )
             arg%data1 => mat
             arg%data2 => mat
             TASK(DIAG(TRACEX("CholTask.Diag. DIAG handle",(hAdd)) ))
             call tl_add_task_named ( TASK_DIAG_NAME,chol_diag_upd,arg,sizeof(arg) , 0,0, hAdd,1, 0,0, 5)
          end if 
       end do ! row block idx
    end do ! col block idx
    arg%data1 => mat
    arg%data2 => mat
    arg%data3 => Xmat
    arg%dealloc = 1 
    hRead = chol_get_handle ( this,'M',rbno,cbno)
    call tl_add_task_named("sync"//char(0),chol_task_sync,arg,sizeof(arg), hRead,1, 0,0, 0,0, 5)

  end subroutine chol_diag_task_cache_opt
!!$------------------------------------------------------------------------------------------------------------------------
  subroutine chol_diag_task(this ) 
    type(dist_chol),intent(inout),pointer :: this
    type(chol_args) :: arg
    real(kind=rfp) , dimension ( :,: ) , pointer :: mat
    real(kind=rfp) , dimension ( :,: ) , pointer :: Xmat
    type(chol_args) :: arg
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
    

    TRACEX("FRT SIZEOF CHOL_ARG",sizeof(arg))

    mat => this%data1
 
    nrows = this%rows
    ncols = this%cols
    TRACEX("nb",this%nb)
    rb=nrows / this%nb
    cb=ncols / this%nb
    if ( ncols < 32 ) then 
       rbno = 4 
       cbno = 4
       rb = nrows / rbno
       cb = ncols / cbno
    end if

    allocate ( Xmat ( 1:nrows, 1:ncols) ) 
    Xmat(:,:) = 0  
    this%nb = rb

    rbno = nrows / rb + 1 
    cbno = ncols / cb + 1 

    TRACEX("CholTask.Diag BlkSize,BlkNo,Element No", (rb,cb,rbno,cbno,nrows,ncols) )
    argidx = 0 
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          hAdd  = chol_get_handle(this,'M',rbidx,cbidx,.true.)
          hAdd  = chol_get_handle(this,'X',rbidx,cbidx,.true.)
       end do
    end do
    do cbidx=1,cbno
       do rbidx = cbidx,rbno
          argidx = argidx+1
          arg%rb=rb
          arg%cb=cb
          arg%this => this
          arg%fr = (rbidx-1) * rb+1
          arg%tr = min(rbidx*rb,nrows)
          arg%fc = (cbidx-1)*cb+1
          arg%tc = min(cbidx*cb,ncols)
          if ( arg%fr > nrows ) exit
          if ( arg%fc > ncols ) exit
          TASK(DIAG(TRACEX("CholTask.Diag  Loop cb,rb, ranges",(argidx,cbidx,rbidx, & 
               arg%fr,arg%tr,arg%fc,arg%tc))))
          if ( rbidx > cbidx ) then 
             ! panel update
             ! B d2 diag, A d1 
             ! read handle M(cbidx,cbidx) , add handle M(rbidx,cbidx)
             hRead = chol_get_handle(this,'M',cbidx,cbidx)
             hAdd  = chol_get_handle(this,'M',rbidx,cbidx)
             hReads(1) = hRead
             hReads(2) = hAdd
             arg%data1 => mat
             arg%data2 => mat
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
             arg%fr2 =     (cbidx  )*rb+1 
             arg%tr2 = min((cbidx+1)*rb  , nrows)
             arg%fc2 =     (cbidx  )*cb+1
             arg%tc2 = min((cbidx+1)*cb  , ncols)
             arg%data1 => mat
             arg%data2 => mat
             arg%data3 => Xmat

             arg%mul_overwrite=1
             arg%TName = "MMUL"
             TASK(DIAG(TRACEX("CholTask.Diag. MMUL handle",(hReads,hAdd)) ))
             call tl_add_task_named( TASK_MMUL_NAME , chol_mat_mul_local , arg, sizeof(arg) , &
                  hReads,2, hAdd ,1 , 0,0, 5)


             ! add X to mat
             ! read handle X( rbidx,cbidx) , add handle M(rbidx,cbidx)
             ! MADD: C=B-A
             do cbidx2=cbidx,rbidx-1
                hRead = chol_get_handle(this,'X',rbidx,cbidx +1  )
                hAdd  = chol_get_handle(this,'M',rbidx,cbidx2+1  )
                arg%data1 => Xmat
                arg%data2 => mat
                arg%data3 => mat
                arg%fc=  cbidx2     *cb + 1
                arg%tc= (cbidx2 + 1)*cb 
                arg%fc2=  (cbidx  )*cb + 1
                arg%tc2=  (cbidx+1)*cb 
                if ( arg%fc  > ncols ) exit
                if ( arg%fc2 > ncols ) exit
                arg%TName = "MADD"
                TASK(DIAG(TRACEX("CholTask.Diag. MADD handle",(hRead,hAdd)) ))
                call tl_add_task_named(TASK_MADD_NAME,chol_mat_add_local, arg,sizeof(arg) , &
                     hRead,1 , hAdd,1,0,0 ,5)
             end do
          end if
          if (rbidx == cbidx) then 
             hAdd = chol_get_handle ( this,'M',rbidx,cbidx )
             arg%data1 => mat
             arg%data2 => mat
             TASK(DIAG(TRACEX("CholTask.Diag. DIAG handle",(hAdd)) ))
             arg%TName = "DIAG"
             call tl_add_task_named ( TASK_DIAG_NAME,chol_diag_upd,arg,sizeof(arg) , 0,0, hAdd,1 , 0,0,5)
          end if 
       end do ! row block idx
    end do ! col block idx
    arg%data1 => mat
    arg%data2 => mat
    arg%data3 => Xmat
    arg%dealloc = 1 
    hRead = chol_get_handle ( this,'M',rbno-1,cbno-1)
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
#ifdef BLOCK_ALLOC 
    type(matrix) :: mat
#endif

    write ( hname,"(A4 A1 A1 A1 I4.4 A1 I4.4)")  this%writes(TASK_DIAG),'_',n,'_',r,'_',c
    fr = this%nb*(r-1)+1
    tr = this%nb*(r  )
    fc = this%nb*(c-1)+1
    tc = this%nb*(c )
    if ( present(create) .and. create  ) then 
       call  dm_matrix_add ( this%dmngr, hname)
       hnd = dm_handle_get ( this%dmngr, hname)
#ifdef BLOCK_ALLOC 
       mat = dm_matrix_get ( this%dmngr, hname)
       call chol_block_init( this,mat%grid,r,c)
#endif
       TASK(HANDLE(TRACEX(" Handle Name +", (hname ,hnd,fr,':',tr,'-',fc,':',tc)) ))
       return 
    end if
    hnd = dm_handle_get(this%dmngr,hname)
    TASK(HANDLE(TRACEX(" Handle Name  ", (hname ,hnd,fr,':',tr,'-',fc,':',tc)) ))
    
  end function chol_get_handle
!!$------------------------------------------------------------------------------------------------------------------------
#ifdef BLOCK_ALLOC 
 subroutine chol_block_init(this,blk,r,c)
    type(dist_chol) , intent(inout) , pointer  :: this
    integer         , intent(in)               :: r,c
    real(kind=rbf) , dimension(:,:) , pointer:: blk

    allocate ( blk ( this%rows,this%cols ) )

 end subroutine chol_block_init
#endif


  Function to_str4(no) Result(str)
    Integer          :: no
    Character(len=4) :: str

    Write (str,"(I4.4)") no

  End Function to_str4
!!$------------------------------------------------------------------------------------------------------------------------
  function to_str3(no) result(str)

    Integer          :: no
    Character(len=2) :: str

    Write (str,"(I2.2)") no-48

  end function to_str3

End Module dist_cholesky
