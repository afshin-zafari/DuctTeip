#include "debug.h"
module block
  character(len=1)  , parameter ::MAT_TYPE_ORD = 'M'
  type matrix_block
     integer  :: X,Y,W,bx,by
     character(len=9) ::mat_type
  end type matrix_block
  integer :: dtmS,shmS
   


  interface operator(.part.)
     module procedure get_2x2block
  end interface
contains 
!----------------------------------------------------------------------
  subroutine  print_block ( M ) 
    type(matrix_block ) , intent (in ) :: M
    write (*,*) "X",M%x,"Y:",M%y,"W:",M%w
    write (*,*) "bX",M%bx,"bY:",M%by,"Type:",M%mat_type
  end subroutine  print_block
!----------------------------------------------------------------------
  function  new_block(Sz,mtype) result ( M ) 
    type(matrix_block) , pointer :: M
    integer,intent(in) :: Sz
    character(len=1),intent(in),optional :: mtype


    allocate ( M ) 

    if ( .not. present ( mtype) ) then 
       M%mat_type = MAT_TYPE_ORD
    else
       M%mat_type = mtype
    end if
    M%x =1 
    M%y =1
    M%w =Sz
    M%bx = 1
    M%by = 1
  end function  new_block
!----------------------------------------------------------------------
  function get_2x2block(M,p) result(M2)
    
    type(matrix_block) , intent(in) :: M
    type(matrix_block) , pointer  :: M2
    integer , intent(in) :: p
    allocate ( M2 )
    M2%w = M%w/2
    M2%x = M%x
    M2%y = M%y
    
    if ( mod ( p,10 ) ==2 )M2%x = M2%x + M2%w 
    if (     ( p/10 ) ==2 )M2%y = M2%y + M2%w 
    M2%bx = M2%x / dtmS+1
    M2%by = M2%y / dtmS+1
    M2%mat_type = M%mat_type 
  end function get_2x2block
!----------------------------------------------------------------------
end module block
