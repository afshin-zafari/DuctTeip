module fp
!
! The parameters rfp, cfp govern the precision in the computations.
! Always recompile (everything) when the precision is changed 
! 
implicit none
!
public
!
! To use single precision (not likely, but just in case)
!
!!$! integer,parameter :: rfp=kind(0.0), cfp=kind((0.0,0.0)) 
!
! Use this statement to get double precision
!
!  integer, parameter :: rfp=kind(0d0), cfp=kind((0d0,0d0))
!
! Use these two statements to get quad precision (4.1860e-34)
! 

!  integer, parameter :: rfp=selected_real_kind(32,600)
!  integer, parameter :: cfp=kind((0.0_rfp,0.0_rfp))

  integer, parameter :: rfp=selected_real_kind(12)
 integer, parameter :: cfp=kind((0d0,0d0))
 
end module fp
!
! Trillian does not support quad
!
