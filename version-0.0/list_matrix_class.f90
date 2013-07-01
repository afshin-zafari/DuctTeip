! TODO make this more efficient
! TODO type(tl_handle), dimension(1)
	! because there is no functioning version for a single var
	! single var is passed as pointer, array as pointer to pointer
	! how do I solve this?

module list_matrix_class

	use matrix_class
	use tl
	use constants

	implicit none

	private
	
	public :: list_matrix, list_matrix_new, list_matrix_delete, list_matrix_print
	public :: list_matrix_add, list_matrix_remove, list_matrix_length !, list_matrix_cleanup
	public :: list_matrix_get_matrix, list_matrix_get_handle, list_matrix_get_vars
	
	type list_matrix
		private
		integer :: length
		type(list_matrix_node), pointer :: first
        end type list_matrix
	
	type list_matrix_node
		type(matrix)					:: mat
		type(tl_handle)					:: handle
		character(len=NAME_LENGTH)	:: nam
		type(list_matrix_node), pointer	:: next 
	end type list_matrix_node
	
contains

	subroutine list_matrix_get_vars(this, vars)
		type(list_matrix), intent(in) :: this
		character(len=*), dimension(:) :: vars
		
		integer :: i
		type(list_matrix_node), pointer :: cur
		cur => this%first
		i = 1
		do
			if (.not. associated(cur)) exit
			
			vars(i) = cur%nam
			
			cur => cur%next
			i = i + 1
			
		end do
		
	end subroutine

	integer function list_matrix_length(this)
		type(list_matrix), intent(in) :: this
		list_matrix_length = this%length
	end function list_matrix_length

	function list_matrix_get_handle(this, nam) result(res)
		type(list_matrix), intent(in) :: this
		character(len=*), intent(in) :: nam
		type(tl_handle) :: res
	
		type(list_matrix_node), pointer :: cur
		cur => this%first
		do
			if (.not. associated(cur)) then
				!write(*,*) "Error: Could not return handle, matrix named '", nam, "' not found."
				exit
			end if
		
			if (cur%nam == nam) then
				res = cur%handle
				exit
			else
				cur => cur%next
			end if
		
		end do
	
	end function list_matrix_get_handle
	
	function list_matrix_get_matrix(this, nam) result(res)
		type(list_matrix), intent(in) :: this
		character(len=*), intent(in) :: nam
		! real, pointer, dimension(:,:) :: res
		type(matrix), pointer :: res
		
		type(list_matrix_node), pointer :: cur
		cur => this%first
		do
			if (.not. associated(cur)) then
				write(*,*) "Error: Could not return matrix, matrix named '", nam, "' not found."
                                call list_matrix_print(this)
				exit
			end if
		
                        !write (*,*) "Matrix scan:",cur%nam,(cur%nam == nam) 
			if (cur%nam == nam) then
				res => cur%mat
				exit
			else
				cur => cur%next
			end if
		end do
		
	end function list_matrix_get_matrix
	
	subroutine list_matrix_new(this)
		type(list_matrix), intent(inout) :: this
		
		this%length = 0
		nullify(this%first)
		
	end subroutine list_matrix_new

	subroutine list_matrix_delete(this)
		type(list_matrix), intent(inout) :: this
		
		type(list_matrix_node), pointer :: old
		
		do
			if(.not. associated(this%first)) exit
		
			old => this%first
			this%first => this%first%next
			
			deallocate(old%mat%grid)
			call tl_destroy_handle(old%handle)
			deallocate(old)
		end do
		
		this%length = 0
		
	end subroutine list_matrix_delete
	
	subroutine list_matrix_add(this, nam)
		type(list_matrix), intent(inout) :: this
		character(len=*), intent(in) :: nam
		
		type(list_matrix_node), pointer :: new
		
		! create new node
		allocate(new)
		new%next => this%first
		
		nullify(new%mat%grid)
		! allocate(new%mat%grid(2,2))
		! new%mat%grid(1,1) = 1.0
		! new%mat%grid(2,1) = 2.0
		! new%mat%grid(1,2) = 3.0
		! new%mat%grid(2,2) = 4.0
		
		call tl_create_handle(new%handle)
		
		new%nam = nam
		! new%sav = .false.		
		
		! relink
		this%length = this%length + 1
		this%first => new
		
	end subroutine list_matrix_add
	
	subroutine list_matrix_remove(this, nam)
		type(list_matrix), intent(inout) :: this
		character(len=*), intent(in) :: nam
		
		type(list_matrix_node), pointer :: cur, prev, old
		cur => this%first
		do
			if (.not. associated(cur)) then
				! write(*,*) "end"
				exit
			end if
			
			! write(*,*) "checking ", cur%nam
		
			if (cur%nam == nam) then
			
				! write(*,*) "removing"
			
				if (associated(cur,this%first)) then
					this%first => cur%next
				else
					prev%next => cur%next
				end if
			
				old => cur
				cur => cur%next
			
				if(associated(old%mat%grid)) then
					deallocate(old%mat%grid)
				else
					! write(*,*) "Warning: Removing unallocated matrix ", trim(nam), ". Tasks handles correct?"
				end if
				! TODO We leak handles, this should be destroyed somehow
				! call tl_destroy_handle(old%handle)
				deallocate(old)
				this%length = this%length - 1
				
			else

				prev => cur
				cur => cur%next
				
			end if
			
		end do
		
	end subroutine
	
	subroutine list_matrix_print(this)
		type(list_matrix), intent(in) :: this
		
		type(list_matrix_node), pointer :: cur
		cur => this%first
		write(*,*) "<list length =", this%length, ">"
		do
			if (.not. associated(cur)) exit

			write(*,*) "<name,nogrid>",cur%nam,"'aaa'"

			if (associated(cur%mat%grid)) then 

			     write(*,*) "<name,grid>",cur%nam,"bbb"          !, ",'",cur%mat%grid,"'" 
			
			end if 

			cur=> cur%next
		
		end do
		
		write(*,*) "</list>"
		
	end subroutine list_matrix_print
	
end module list_matrix_class
