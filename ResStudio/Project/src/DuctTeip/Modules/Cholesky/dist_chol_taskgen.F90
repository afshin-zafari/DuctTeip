#include "debug.h"
module cholesky_taskgen

  use task_generation
  use dist_types

  implicit none 
  integer , parameter ::GEMM_WORK = 4
  integer , parameter ::PNL_WORK  = 4
  integer , parameter ::SUB_WORK  = 4
  integer , parameter ::CHOL_WORK = 4


  type(task_gen) ,pointer,private:: tgen
  
 contains
!------------------------------------------------------------------------------------------
   function cholesky_init (N,s1,s2,nodes) result ( M) 
     integer , intent(in ) :: N,s1,s2,nodes
     type(matrix_block ), pointer  :: M 
     type(task_gen) , pointer :: tg
     dtmS = s1
     shmS = s2
     M => new_block ( N ) 
   end function cholesky_init
!-------------------------------------------------------------------------------
   subroutine chol_task_gen(dt_cfg,n,p,q,b)
     type(remote_access) , intent(inout) :: dt_cfg
     integer , intent(in ) :: n ,p,q,b
     integer :: i,j,k
     type(matrix_block ) :: Mii,Mji,Mik,Mjk,Xjk , ro(1:2),wr(1),Mnull(1)

#define  NOTHING Mnull,0
#define InitMat(M) M%w=b ; M%mat_type='MainMatM'  
#define BlockIndex(M,i,j) M%by = i; M%bx = j


     tgen =>taskgen_init(p * q)
     allocate ( tgen%rma )
     tgen%rma = dt_cfg
     tgen%grp_rows = p
     tgen%grp_cols = q
     tgen%node_cnt = p * q
     tgen%block_cnt = n 

     Mnull%by = 0 
     Mnull%bx = 0
     InitMat(Mii)
     InitMat(Mji)
     InitMat(Mik)
     InitMat(Mjk)

     Xjk%w = b ; Xjk%mat_type='MainMatM'


     do i = 1,n
        BlockIndex(Mii,i,i)
        ro(1) = Mii 
        wr(1) = Mii
        call addtask ( tgen , func_key("Diag"),ro,1,wr,1,NOTHING)
        do j = i+1,n
           BlockIndex (Mji,j,i)  
           ro(1) = Mii
           wr(1) = Mji
           call addtask (tgen ,func_key("PnlU"),ro,1 , wr ,1,NOTHING  ) 
        end do
        do j= i+1,n
           do k= 1,i
              BlockIndex (Mjk,j,k)  
              BlockIndex (Mik,i+1,k)  
              BlockIndex (Mji,j,i+1)  
              BlockIndex (Xjk,j,k)  
              ro(1) = Mik ;  ro(2) = Mjk
              wr(1) = Xjk
              call addtask ( tgen,func_key("MMul"),ro,2 , wr,1,NOTHING ) 
              ro(1) = Xjk ;  ro(2) = Mji
              wr(1) = Mji
              call addtask ( tgen,func_key("SubT"),ro,2 , wr,1,NOTHING ) 
           end do
        end do
     end do
     call print_lists(tgen%rma,.true.)
     call print_block_hosts(n)
   end subroutine chol_task_gen

   !------------------------------------------------------------------------------------------
   subroutine print_block_hosts(n)
     integer , intent(in) :: n 
     integer,dimension(n,n)::h
     integer :: i,j
     type(matrix_block) :: M
     write (*,*) "Hosts of blocks:"
     do i = 1,n
        do j=1,i
           M%by = i  ; M%bx = j 
           h(i,j)=block_host(tgen, M)
           write (*,"(I3)",advance='no') h(i,j)
        end do
        write (*,*) ' '
     end do



   end subroutine print_block_hosts

 end module cholesky_taskgen
