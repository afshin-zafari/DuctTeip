! TODO: should be able to handle 32 and 64 bit memory addresses
module tl_handle_class
#ifdef LINUX
#define INTEROP_BIND(a) ,BIND(a)
#define INTEROP_TYPE(a) (a)
  use iso_c_binding
#else
#define INTEROP_BIND(a) 
#define INTEROP_TYPE(a) 
#endif

  implicit none
  
  type, public  INTEROP_BIND(C)   :: tl_handle
     integer INTEROP_TYPE(C_LONG) :: address = 1337
  end type tl_handle
  
end module tl_handle_class
