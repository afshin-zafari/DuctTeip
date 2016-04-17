module pthreads

	implicit none

	type mutex
		integer, pointer :: address
		! integer :: address = 1337
	end type mutex

contains

	subroutine pthreads_mutex_init(this)
		type(mutex), intent(inout) :: this
		call mutex_init(this)
	end subroutine pthreads_mutex_init
	
	subroutine pthreads_mutex_destroy(this)
		type(mutex), intent(inout) :: this
		call mutex_init(this)
	end subroutine pthreads_mutex_destroy
		
	subroutine pthreads_mutex_lock(this)
		type(mutex), intent(inout) :: this
		call mutex_lock(this)
	end subroutine pthreads_mutex_lock
		
	subroutine pthreads_mutex_unlock(this)
		type(mutex), intent(inout) :: this
		call mutex_unlock(this)
	end subroutine pthreads_mutex_unlock
	
end module pthreads