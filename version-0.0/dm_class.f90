module dm_class

	use pthreads
	use tl
	use list_matrix_class
	use matrix_class
	use constants
	
	implicit none
	
	type dm
		private
		type(mutex)			:: mats_mutex
		type(list_matrix)	:: mats
                integer::sync,taskid
	end type dm

	type dm_remove_arg
		type(dm), pointer				:: this
		character(len=NAME_LENGTH)	:: nam
	end type dm_remove_arg
	
contains
  
        function dm_get_taskid(this)result (val)
          type(dm), intent(in) :: this
          integer  :: val
          val = this%taskid 
        end function dm_get_taskid
        subroutine dm_set_taskid(this,val)
          type(dm), intent(inout) :: this
          integer , intent(in)    :: val
          this%taskid = val 
        end subroutine dm_set_taskid
        function dm_get_sync(this) result(val)
          type(dm), intent(in) :: this
          integer  :: val
          val = this%sync 
        end function dm_get_sync
        subroutine dm_set_sync(this,val)
          type(dm), intent(inout) :: this
          integer , intent(in) :: val
          this%sync = val
        end subroutine dm_set_sync
        function dm_get_mat_length(this) result(len)
          type(dm), intent(in) :: this
          integer :: len
          len = list_matrix_length(this%mats)
        end function dm_get_mat_length

	subroutine dm_new(this)
         type(dm), intent(inout) :: this
  
         call pthreads_mutex_init(this%mats_mutex)
         call list_matrix_new(this%mats)
         this%sync = 0
  
	end subroutine dm_new
	
	subroutine dm_delete(this)
		type(dm), intent(inout) :: this
	
		call pthreads_mutex_destroy(this%mats_mutex)
		call list_matrix_delete(this%mats)
	
	end subroutine dm_delete
	
	subroutine dm_matrix_add(this, nam)
		type(dm), intent(inout) :: this
		character(len=*), intent(in)	:: nam
		
		call pthreads_mutex_lock(this%mats_mutex)
		! write(*,*) "critical"
		call list_matrix_add(this%mats, nam)
		! write(*,*) "/critical"
		call pthreads_mutex_unlock(this%mats_mutex)
		
	end subroutine dm_matrix_add
	
	subroutine dm_matrix_remove_task(arg)
		type(dm_remove_arg), intent(inout) :: arg
		
		! write(*,*) "Task: Removing ", trim(arg%nam)
		call dm_matrix_remove(arg%this, arg%nam)
		
	end subroutine dm_matrix_remove_task
	
	subroutine dm_matrix_remove(this, nam)
		type(dm), intent(inout) :: this
		character(len=*), intent(in)	:: nam
		
		call pthreads_mutex_lock(this%mats_mutex)
		! write(*,*) "critical"
		call list_matrix_remove(this%mats, nam)
		! write(*,*) "/critical"
		call pthreads_mutex_unlock(this%mats_mutex)
		
	end subroutine dm_matrix_remove
	
	function dm_matrix_get(this, nam) result(res)
		type(dm), intent(in)			:: this
		character(len=*), intent(in)	:: nam
		! real, dimension(:,:), pointer	:: res
		type(matrix), pointer			:: res
		
		res => list_matrix_get_matrix(this%mats, nam)
	
	end function dm_matrix_get
	
	function dm_handle_get(this, nam) result(res)
		type(dm), intent(in)			:: this
		character(len=*), intent(in)	:: nam
		type(tl_handle)					:: res
		
		res = list_matrix_get_handle(this%mats, nam)
		
	end function dm_handle_get

	subroutine dm_cleanup(this, savs)
		type(dm), intent(inout), target :: this
		character(len=*), dimension(:), intent(in) :: savs
		
		character(len=NAME_LENGTH), dimension(:), pointer :: vars
		integer :: length, i, j
		logical :: sav
		type(tl_handle) :: handle
		type(dm_remove_arg) :: arg
		
		! get arrays of all existing vars
		length = list_matrix_length(this%mats)
		allocate(vars(1:length))
		call list_matrix_get_vars(this%mats, vars)
		
		! create tasks to remove unwanted vars
		do i=1,length
		
			sav = .false.
		
			do j=1,size(savs)
				if(vars(i) == savs(j)) sav = .true.
			end do
		
			! is this var unwanted?
			if (.not. sav) then
				
				arg%this => this
				arg%nam = vars(i)
				handle = list_matrix_get_handle(this%mats, vars(i))
				call tl_add_task_unsafe(dm_matrix_remove_task, arg, sizeof(arg), 0, 0, handle, 1, 0, 0)
				
			end if
		
		end do
		
		! clean up
		deallocate(vars)
		
		! wait for everything to finish
		call tl_barrier()
		
	end subroutine dm_cleanup
	
	subroutine dm_print(this)
		type(dm), intent(in)	:: this
		
		call list_matrix_print(this%mats)
		
	end subroutine dm_print
	
end module dm_class
