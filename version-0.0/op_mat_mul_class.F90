# 1 "op_mat_mul_class.F90"
module op_mat_mul_class

	use tl
	use dm_class
	use matrix_class

	implicit none

	type op_mat_mul
		! private
		character(len=NAME_LENGTH), dimension(2) :: reads
		character(len=NAME_LENGTH) :: writes
	end type op_mat_mul
	
	type args
		type(op_mat_mul), pointer	:: this
		type(dm), pointer			:: datamanager
	end type args

contains

	subroutine op_mat_mul_do(arg)
		type(args), intent(inout) :: arg
		
		type(matrix), pointer	:: A, B, C
        integer ::n,k,m
        real :: alpha=1.0,beta=0.0
		
		write(*,*) "Task: ", trim(arg%this%writes), "=", trim(arg%this%reads(1)), "*", trim(arg%this%reads(2))
		
		! call dm_matrix_add(datamanager, this%writes)
		A => dm_matrix_get(arg%datamanager, arg%this%reads(1))
		B => dm_matrix_get(arg%datamanager, arg%this%reads(2))
		C => dm_matrix_get(arg%datamanager, arg%this%writes)
    	m=size(A%grid, 1)
    	n=size(B%grid, 2)
    	k=size(A%grid, 2)
		allocate(C%grid(m, n))
# 44

		C%grid = MATMUL(A%grid, B%grid)	


	end subroutine

	subroutine op_mat_mul_task(this, datamanager)
		type(op_mat_mul), intent(inout), target :: this
		type(dm), intent(inout), target :: datamanager
		
		type(tl_handle), dimension(2) :: reads
		type(tl_handle) :: writes
		type(args) :: arg
		
		! add result matrix
		write(*,*) "Serial: Declaring ", trim(this%writes)
		call dm_matrix_add(datamanager, this%writes)
		
		! get info needed for task creation
		arg%this => this
		arg%datamanager => datamanager
		writes = dm_handle_get(datamanager, this%writes)
		reads(1) = dm_handle_get(datamanager, this%reads(1))
		reads(2) = dm_handle_get(datamanager, this%reads(2))
		
		! schedule task
		call tl_add_task_unsafe(op_mat_mul_do, arg, sizeof(arg), reads, 2, writes, 1, 0, 0)
		
	end subroutine

	subroutine op_mat_mul_new(this, fac1, fac2, res)
		type(op_mat_mul), intent(inout) :: this
		character(len=*), intent(in) :: fac1, fac2, res
		
		this%reads(1) = fac1
		this%reads(2) = fac2
		this%writes = res
		
	end subroutine

	subroutine op_mat_mul_print(this)
		type(op_mat_mul), intent(in) :: this
		write(*,*) "op_mat_mul ", "writes ", this%writes , " reads ", this%reads
	end subroutine


end module op_mat_mul_class
