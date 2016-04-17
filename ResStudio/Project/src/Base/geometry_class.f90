module geometry_class

	use class_point_set

	implicit none
	
	type geometry
		integer, dimension(:), pointer :: eqs
	end type geometry

contains

	integer function geometry_eq(this, i)
		type(geometry), intent(in) :: this
		integer, intent(in) :: i
		
		geometry_eq = this%eqs(i)
		
	end function geometry_eq
	
	integer function geometry_size(this)
		type(geometry), intent(in) :: this
		
		geometry_size = size(this%eqs)
		
	end function geometry_size

	subroutine geometry_new(this, types)
		type(geometry), intent(inout)		:: this
		integer, dimension(0:), intent(in)	:: types
		
		integer :: i

		! TODO make this general
		! write(*,*) types
		allocate(this%eqs(0:size(types)-1))

		do i=0,size(this%eqs)-1
			this%eqs(i) = types(i)
		end do
		
		! write(*,*) this%eqs
		
	end subroutine geometry_new
	
	subroutine geometry_delete(this)
		type(geometry), intent(inout) :: this
		
		deallocate(this%eqs)
		
	end subroutine geometry_delete
	
	subroutine geometry_print(this)
		type(geometry), intent(in) :: this
		
		write(*,*) this%eqs
		
	end subroutine geometry_print
	
end module geometry_class
