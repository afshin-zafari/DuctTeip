module op_mat_new_class

	use tl
	use dm_class
	use matrix_class

	implicit none

	type op_mat_new
		! private
		character(len=NAME_LENGTH) :: writes
	end type op_mat_new

	type args
		type(op_mat_new), pointer	:: this
		type(dm), pointer			:: datamanager
	end type args
	
contains

	subroutine op_mat_new_do(arg)
		type(args), intent(inout) :: arg
	
		type(matrix), pointer :: mat
		
		write(*,*) "Task: Creating ", trim(arg%this%writes)
		
		mat => dm_matrix_get(arg%datamanager, arg%this%writes)
		allocate(mat%grid(2,2))
		mat%grid(1,1) = 1.0
		mat%grid(2,1) = 2.0
		mat%grid(1,2) = 3.0
		mat%grid(2,2) = 4.0
	
	end subroutine
	
	subroutine op_mat_new_task(this, datamanager)
		type(op_mat_new), intent(inout), target :: this
		type(dm), intent(inout), target :: datamanager
		
		type(tl_handle) :: handle
		type(args) :: arg
		
		! declare new matrix
		write(*,*) "Serial: Declaring ", trim(this%writes)
		call dm_matrix_add(datamanager, this%writes)
		
		! get info needed for task creation
		arg%this => this
		arg%datamanager => datamanager
		handle = dm_handle_get(datamanager, this%writes)
		
		! schedule task to set it to something
		call tl_add_task_unsafe(op_mat_new_do, arg, sizeof(arg), 0, 0, handle, 1, 0, 0)
		
		! write(*,*) "TODO: Add create matrix ", this%writes, " task"
		
	end subroutine

	subroutine op_mat_new_new(this, res)
		type(op_mat_new), intent(inout) :: this
		character(len=*), intent(in) :: res
		
		this%writes = res
		
	end subroutine

	subroutine op_mat_new_print(this)
		type(op_mat_new), intent(in) :: this
		write(*,*) "op_mat_new ", "writes ", this%writes
	end subroutine

end module op_mat_new_class