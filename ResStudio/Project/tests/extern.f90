module extern 
implicit none
contains
  function test(f,a) result (i)
    use common
	type(dt_config), intent(in) ::a
	integer , pointer,intent(in)::f
   integer ::i
	call dtinit(f,a)
	end function test
end module 
subroutine dtinit(func,conf)
    use common
  integer ::func
  type(dt_config),intent(in) :: conf
  integer ::b
   
  external func
   write(*,*) "in sub",conf.a,conf.y,conf.z,conf.c,conf.ip(:)
   b=  func(conf)

  write(*,*) "output",b,conf.a,conf.y,conf.z,conf.c,conf.ip(:)

  write(*,*) "exit sub"

  end 
