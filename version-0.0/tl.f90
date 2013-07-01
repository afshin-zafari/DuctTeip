!TODO: 32/64 bit compability
!TODO: Type safety between C and Fortran. I.e. int and integer same # of bits?

module tl

	use tl_handle_class
	
	implicit none

	interface
	
		subroutine tl_init(num_threads,node_id)
			integer, intent(in) :: num_threads,node_id
		end subroutine tl_init
	
		subroutine tl_destroy()
		end subroutine tl_destroy
		
		subroutine tl_barrier()
		end subroutine tl_barrier
		
		! TODO: Possible to define here with arbitrary function and arg?
		!subroutine tl_add_task_safe(...)
		!end subroutine tl_add_task_safe(...)
		
		!subroutine tl_add_task_unsafe(...)
		!end subroutine tl_add_task_unsafe(...)
		
		subroutine tl_create_handle(handle)
			use tl_handle_class
			type(tl_handle), intent(out)	:: handle
		end subroutine tl_create_handle
		
		! TODO: array normaly sent as pointer to pointer when using this interface
		!		however when part of a derived type it is sent as pointer
		!		this causes problems since we can't be sure which type was sent
		subroutine tl_create_handles(num_handles, handles)
			use tl_handle_class
			integer, intent(in)							:: num_handles
			type(tl_handle), dimension(:), intent(out)	:: handles
		end subroutine tl_create_handles
		
		subroutine tl_destroy_handles(handles, num_handles)
			use tl_handle_class
			integer, intent(in)							:: num_handles
			type(tl_handle), dimension(:), intent(out)	:: handles
		end subroutine tl_destroy_handles
	
	end interface

end module tl
