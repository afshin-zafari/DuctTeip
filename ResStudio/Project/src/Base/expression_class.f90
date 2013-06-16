! TODO: Change this dummy module, use Danhua's expression class
module expression_class

	use class_point_set

	type expression
		character :: dummy
	end type expression

contains

	subroutine expression_new(this)
		type(expression), intent(inout) :: this
		! Do nothing in dummy
	end subroutine expression_new

 Function expression_get_op(this, Type) Result(op)
   Character (len=1)::op
   Type(expression), Intent(in) :: this
   Integer, Intent(in) :: Type
   
   If(Type == 1) Then
      op = "L"
   Else If(Type == 0) Then
      op = "0"
   Else
      Write(*,*) "ERROR: Dummy expression class called with '", Type, "'"
   End If
   
 End Function expression_get_op
	
end module expression_class
