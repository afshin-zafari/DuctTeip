! TODO: integer subtype för vilken type och caseof för att välja

module op_class

	use dm_class
	use op_mat_new_class
	use op_mat_mul_class
	use op_ass_class
	use problem_class
	use approximation_class

	implicit none
	
	type op
		! private
		type(op_mat_new), pointer	:: op1
		type(op_mat_mul), pointer	:: op2
		type(op_ass), pointer		:: op3
	end type op
	
contains

	! ===== OPERATION CREATION =====

	subroutine op_assemble(this, prob, approx, res)
		type(op), intent(inout)			:: this
		type(problem), intent(in) 		:: prob
		type(approximation), intent(in)	:: approx
		character(len=*), intent(in)	:: res
		
		! nullify
		nullify(this%op1)
		nullify(this%op2)
		nullify(this%op3)
		! etc
		
		allocate(this%op3)
		call op_ass_new(this%op3, prob, approx, res)
		
	end subroutine op_assemble
	
	subroutine op_create_matrix(this, res)
		type(op), intent(inout)			:: this
		character(len=*), intent(in)	:: res
	
		! nullify
		nullify(this%op1)
		nullify(this%op2)
		nullify(this%op3)
		! etc
		
		! init
		allocate(this%op1)
		call op_mat_new_new(this%op1, res)
	
	end subroutine op_create_matrix
	
	subroutine op_mult_matrix(this, fac1, fac2, res)
		type(op), intent(inout)			:: this
		character(len=*), intent(in)	:: fac1, fac2, res
		
		! nullify
		nullify(this%op1)
		nullify(this%op2)
		nullify(this%op3)
		! etc

		! init
		allocate(this%op2)
		call op_mat_mul_new(this%op2, fac1, fac2, res)
		
	end subroutine op_mult_matrix
	
	! ===== /OPERATION CREATION =====	
	
	subroutine op_delete(this)
		type(op), intent(inout) :: this
		
		if (associated(this%op1)) deallocate(this%op1)
		if (associated(this%op2)) deallocate(this%op2)
		if (associated(this%op3)) deallocate(this%op3)
		! etc
		
	end subroutine

	! TODO gör task här istället
	subroutine op_task(this, datamanager)
		type(op), intent(inout) :: this
		type(dm), intent(inout) :: datamanager
		
		if (associated(this%op1)) call op_mat_new_task(this%op1, datamanager)
		if (associated(this%op2)) call op_mat_mul_task(this%op2, datamanager)
		if (associated(this%op3)) call op_ass_task(this%op3, datamanager)
		! etc
		
	end subroutine
	
	subroutine op_print(this)
		type(op), intent(inout) :: this
		
		if (associated(this%op1)) call op_mat_new_print(this%op1)
		if (associated(this%op2)) call op_mat_mul_print(this%op2)
		if (associated(this%op3)) call op_ass_print(this%op3)
		! etc
		
	end subroutine
	
end module op_class
