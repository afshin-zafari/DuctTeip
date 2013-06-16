# 1 "mainold.F90"
! Copied from the C header file <dlfcn.h>









                                                

                                                

                                                

                                                


  program main

    use common
    use extern
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
!contains
!integer function somef()
  character*20 func_name, lib_name
  integer,pointer :: ihandle            ! handle for dlopen
  integer,pointer :: fptr               ! function pointer returned by dlsym
  logical cont
  type(dt_config) ::conf
  external dtinit

	!somef=0
  !write (*,*) "fPtr from SoLib:"
  cont = .true.
  do while (cont)
   write(*,'("Name of function to execute: ",$)')
   read("m1.add",'(A20)')func_name
   if (func_name(1:1) .NE. ' ') then
     write(*,'("Library this function is in: ",$)')
     read("m1lib.so",'(A20)')lib_name

     ihandle => dlopenf(lib_name, Z'00100'   +Z'00002')
     write (*,*) "Handle of SoLib:",ihandle

     fptr => dlsymf(ihandle,func_name)  ! fptr has the pointer to the
                                        ! function specified by the user
     write (*,*) "fPtr from SoLib:",fptr

! Note that the only way to call a function pointer from Fortran is
! to go through a dummy subroutine
		!call fptr(10)
!     call dumsub(fptr,10,20)            ! call this function with fixed
                                        ! arguments. This can be changed
                  
                      ! to have user supplied args as well
		conf%a=222
		conf.y=35.7
		conf.z =(/1,4,7,10/)
      conf.c="In mainoldf90"
      allocate( conf.ip(1:4) )
		conf.ip = conf.z *2; 
      write(*,*) "before call",conf.a,conf.y,conf.z,conf.c,conf.ip(:)
      conf%a=test(fptr,conf)
      cont=.false.
		write(*,*) "after sub"
   else
     cont = .false.
   endif
  enddo
 write(*,*) "after loop"
end

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

  end 


