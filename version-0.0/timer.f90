! $$$ timer.f90 $$$
!
module timer
contains
!
  ! function getime()
!
  ! real :: getime
!
  ! integer(kind=8) timing
!
    ! call highrestimer(timing)
    ! getime = 0.000000001*timing
  ! end function getime
  
	function getime()
		integer(kind=8) :: getime
		integer(kind=8) timing
		call highrestimer(timing)
		getime = timing
	end function getime
  
end module timer
