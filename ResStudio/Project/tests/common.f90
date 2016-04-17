module common
    type dt_config
         integer :: a
         real :: y
         integer,dimension(1:4)::z
         character(len=20) :: c
         integer,dimension(:),pointer::ip
    end type
	 type dt_task
		integer ::id
		character(len=20)::name
	 end type
	 interface
		function create_task2(name) result (taskid)
			integer ::taskid
			character(len=*),intent(in)::name
		end function
    end interface
contains 
		function create_task(conf,name) result (taskid)
			type(dt_config) , intent(inout),pointer :: conf
			integer ::taskid
			character(len=*),intent(in)::name
			taskid=10;
			write (*,*) "Task Name:",name
         if ( associated(conf.ip) ) then 
 			   write (*,*) "Conf2 in Create Task",conf%a,conf%y,conf%z,conf%c,conf%ip(:)
			else
				write (*,*) "Conf1 in Create Task",conf%a,conf%y,conf%z,conf%c

    	 	end if 
		end function
end module 
