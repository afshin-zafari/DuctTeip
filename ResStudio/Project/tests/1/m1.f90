module m1 
  use common
contains
  function dt_init(conf1) result(res)
	type(dt_config) , intent(inout),target :: conf1
	type(dt_config) , pointer :: conf
	integer ::res
	integer ::a
	allocate ( conf ) 
	conf.a = 10
	conf.y=110.220
	conf.z=(/4,5,6,7/)
   conf.c="Now it is ok."
	write (*,*) "Before ip"
   if ( .not. associated (conf.ip) ) then 
	   write (*,*) "m1.Conf a,b:",conf.a,conf.y,conf.z,conf.c(1:12)
      allocate( conf.ip(1:4) )
	end if
   write (*,*) "m1.Conf a,b:",conf.a,conf.y,conf.z,conf.c(1:12),conf.ip(1:4)
	conf.a= create_task(conf,"Afshin")
   res=7
	conf.z(3:4)=33
	conf.c = "In Module M1"
   conf.ip = conf.z *3;
   write (*,*) "m1.Conf.3 a,b:",conf.a,conf.y,conf.z,conf.c(1:12),conf.ip(1:4)
  end function
  function test(s) result(k)
    integer ::k
    type(dt_config) ,intent(inout):: s
    s.a=22
    k=23
  end function 
  function add(p1, p2) result(a)
  integer ,intent(in)::p1, p2
  integer ::a

  a = p1+p2
  end function 

  function sub(p1,p2) result (a)
  integer ::p1,p2,a

  a = p1-p2
  end function
  
end module
