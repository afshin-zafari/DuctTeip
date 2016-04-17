
#include "debug.h"

Module dist_cholesky
  Use dm_class
  Implicit None

  Type args
     Character(len=8)	:: read1,read2,Write
     Type(dm) , Pointer :: dmngr
     Real(kind=rfp)::diag_elem
  End Type args

!  Type dist_chol
!  End Type dist_chol


  Subroutine dist_chol_syrk(arg)
!    Target -= x-Block
    Write (*,*) "test"
  End Subroutine dist_chol_syrk
!!$
!!$  Subroutine dist_chol_gemm(arg)
!!$!   block1 *block2-> x-block
!!$  End Subroutine dist_chol_gemm
!!$ 
!!$  Subroutine dist_chol_update_col(arg)
!!$   !arg%diag_elem,data%col_partition
!!$
!!$  End Subroutine dist_chol_update_col
!!$
!!$  Subroutine dist_chol_diag_task(this,dmgr)
!!$  
!!$	! create read blocks
!!$    nb = data_size(dmgr)/num_thrd
!!$    Do i=1,nb
!!$       cols_org  =this%arg%read1//"_"//Achar(i+48)
!!$       xblocks   ="X_"//Achar(i+48)
!!$       cols_final=this%arg%writes//"_"//Achar(i+48)
!!$       Call dm_matrix_add(dmgr,cols_org)
!!$       Call dm_matrix_add(dmgr,xblocks)
!!$       Call dm_matrix_add(dmgr,cols_final)
!!$    End Do
!!$
!!$!    Call tl_add_task_named("diag",dist_chol_diag,arg,sizeof(arg),cols_org,nb,cols_final,nb,0,0)
!!$!    Call tl_add_task_named("diag",dist_chol_update_col,arg,sizeof(arg),cols_org,nb,cols_final,nb,0,0)
!!$!    Call tl_add_task_named("diag",dist_chol_gemm,arg,sizeof(arg),cols_final,nb,xblocks,nb,0,0)
!!$!    Call tl_add_task_named("diag",dist_chol_syrk,arg,sizeof(arg),xblocks,nb,cols_final,nb,0,0)
!!$  End Subroutine dist_chol_diag_task

End Module dist_cholesky
