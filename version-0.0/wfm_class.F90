# 1 "wfm_class.F90"

# 4

# 1 "./debug.h" 1 



# 11










# 23




                          



# 35


# 41






# 51






# 61






# 71






# 81






# 91





# 6 "wfm_class.F90" 2 

!TODO store op in list?

module wfm_class
	
	use dm_class
	use op_class

	implicit none
	
	type wfm
		! private
		type(dm), pointer :: datamanager
		type(op), pointer, dimension(:) :: ops
		integer :: num_ops
		integer :: cur_op
	end type

contains

  function wfm_get_op_asm(this,idx) result (ops)
    type(wfm), intent(inout) :: this
    type(op_ass) , pointer :: ops
    integer , intent(in) :: idx
    
    ops => this%ops(idx)%op3  

  end  function wfm_get_op_asm

        subroutine wfm_set_new_pts(this,pts)
    		type(wfm), intent(inout) :: this
            type(point_set)::pts
            type(epsilon)::eps
            character(len=20)::phi
            integer :: i 
            do i=1,this%num_ops
                if ( associated(this%ops(i)%op3) ) then 
                    eps = approximation_get_eps(this%ops(i)%op3%approx)
                    phi = approximation_get_phi(this%ops(i)%op3%approx)
                    CALL approximation_new2(this%ops(i)%op3%approx,phi,eps,pts,pts)
                end if 
            end do

        end subroutine wfm_set_new_pts


	subroutine wfm_new(this, num_ops)
		type(wfm), intent(inout) :: this
		integer, intent(in) :: num_ops
		
		integer :: i
		
		! init datamanger
		allocate(this%datamanager)
		call dm_new(this%datamanager)
		
		! init nums
		this%num_ops = num_ops
		this%cur_op = 1
		
		! init ops
		allocate(this%ops(1:this%num_ops))
		do i=1,this%num_ops
			! nullify ops?
		end do
		
	end subroutine wfm_new
	
	subroutine wfm_delete(this)
		type(wfm), intent(inout) :: this
		
		integer :: i
		
		! delete objects
		call dm_delete(this%datamanager)
		do i=1,this%cur_op-1
			call op_delete(this%ops(i))
		end do
	
		! deallocate
		deallocate(this%datamanager)
		deallocate(this%ops)
		this%num_ops = 0
		this%cur_op = 0
	
	end subroutine wfm_delete
	
	subroutine wfm_add(this, new_op)
		type(wfm), intent(inout) :: this
		type(op), intent(in) :: new_op
		
		if (this%cur_op > this%num_ops) then
			write(*,*) "Error: Maximum number of ops already added"
			return
		end if
		this%ops(this%cur_op) = new_op
		this%cur_op = this%cur_op + 1
		
	end subroutine wfm_add
	
	subroutine wfm_execute(this, savs)
		type(wfm), intent(inout) :: this
		character(len=*), dimension(:), intent(in) :: savs
		
		integer :: i
		
		! TODO
		! write(*,*) "<Executing>"
		
		! make each op into a task
		do i=1,this%num_ops
			call op_task(this%ops(i), this%datamanager)
		end do
		
		! 
		
		! add clean up tasks
		call dm_cleanup(this%datamanager, savs)
		
		! TODO
		! write(*,*) "</Executing>"
		
	end subroutine wfm_execute

	subroutine wfm_print(this)
		type(wfm), intent(inout) :: this
		
		integer :: i
		
		write(*,*) "num_ops", this%num_ops
		write(*,*) "cur_op", this%cur_op
		do i=1,this%num_ops
			call op_print(this%ops(i))
		end do
		
	end subroutine wfm_print
	
	subroutine wfm_print_dm(this)
		type(wfm), intent(in) :: this
		
		call dm_print(this%datamanager)
		
	end subroutine wfm_print_dm
	
end module wfm_class
