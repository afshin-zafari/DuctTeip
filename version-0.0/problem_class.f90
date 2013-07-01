module problem_class

	use geometry_class
	use expression_class

	implicit none
	type problem
		type(expression)	:: expr
		type(geometry)		:: geom
	end type problem

contains

	subroutine problem_new(this, geom, expr)
		type(problem), intent(inout) :: this
		type(expression), intent(in) :: expr
		type(geometry), intent(in) :: geom
		
		this%expr = expr
		this%geom = geom
		
	end subroutine problem_new

	function problem_get_expression(this) result(res)
		type(problem), intent(in) :: this
		type(expression) :: res
		res = this%expr
	end function problem_get_expression
	
	function problem_get_geometry(this) result(res)
		type(problem), intent(in) :: this
		type(geometry) :: res
		res = this%geom
	end function problem_get_geometry
	
end module problem_class