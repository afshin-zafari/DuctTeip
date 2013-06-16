! Copied from the C header file <dlfcn.h>

#define RTLD_LAZY               Z'00001'       
#define RTLD_NOW                Z'00002'       

#define RTLD_NOLOAD             Z'00004'       

#define RTLD_GLOBAL             Z'00100'       
#define RTLD_LOCAL              Z'00000'       

                                                
#define RTLD_PARENT             Z'00200'       
                                                
#define RTLD_GROUP              Z'00400'       
                                                
#define RTLD_WORLD              Z'00800'       
                                                
#define RTLD_NODELETE           Z'01000'       

  program 

    use common
  implicit none

! Interface blocks for dlopen and dlsym

  interface
    function dlopenf(name,i)
      integer,pointer :: dlopenf        ! returns a pointer to an integer
      character*(*) name
      integer i
      intent (in) :: name,i
    end function dlopenf

    function dlsymf(handle,name)
      integer,pointer :: dlsymf         ! returns a pointer to an integer
      character*(*) name
      integer :: handle
      intent(in) :: name
    end function dlsymf
  end interface



  character*20 func_name, lib_name
  integer,pointer :: fptr               ! function pointer returned by dlsym
  logical cont


  cont = .true.
  do while (cont)
   write(*,'("Name of function to execute: ",$)')
   read(*,'(A20)')func_name
   if (func_name(1:1) .NE. ' ') then
     write(*,'("Library this function is in: ",$)')
     read(*,'(A20)')lib_name

       ! fptr has the pointer to the
     fptr => dlsymf(ihandle,func_name)
                                        ! function specified by the user
     write (*,*) "fPtr from SoLib:",fptr

! Note that the only way to call a function pointer from Fortran is
! to go through a dummy subroutine

     call dumsub(fptr,10,20)            ! call this function with fixed
                                        ! arguments. This can be changed
                                        ! to have user supplied args as well

   else
     cont = .false.
   endif
  enddo
contains 
  function get_func_ptr(libname,module_name,func_name) result (fptr)
    integer,pointer::fptr
	 character(len=20),intent(in)::libname,module_name,func_name
    integer,pointer :: ihandle            ! handle for dlopen
	 character (len=100) :: full_name
	
     ihandle => dlopenf(libname, RTLD_NOW)
     write (*,*) "Handle of SoLib:",ihandle

	  full_name=module_name//"."//func_name
     fptr => dlsymf(ihandle,full_name)
  end function

  subroutine dumsub(func,a1,a2)
    use common
  integer ::func
  integer ,intent(in) :: a1,a2
  integer ::b
  type(dt_config) :: s
  external func
    b = func(s)
  write(*,*) "inputs",a1,a2
  !b=func(a1,a2)
  write(*,*) "output",b,s.a
!  print *,"Result :",func(a1,a2)

  end subroutine


 

  end program


