module const
!
use fp
!
implicit none
!
public
!
! These are constants that can be used in the program. pi is pi and
! im is i the imaginary unit.
!
real(kind=rfp), parameter :: &
     pi=3.1415926535897932384626433832795028841971_rfp
complex(kind=cfp), parameter :: im=(0.0_rfp,1.0_rfp)
!
end module const
