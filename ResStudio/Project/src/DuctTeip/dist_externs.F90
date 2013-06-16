
#include "debug.h"

! Copied from the C header file <dlfcn.h>

#define RTLD_NOLOAD             Z'0004'       

#define RTLD_LOCAL              Z'0000'       

                                                
#define RTLD_PARENT             Z'0200'       
                                                
#define RTLD_GROUP              Z'0400'       
                                                
#define RTLD_WORLD              Z'0800'       
                                                
#define RTLD_NODELETE           Z'1000'       


module dist_externs
  use dist_types
  implicit none
  integer *8 , parameter :: RTLD_LAZY             =  Z'0001'       
  integer *8 , parameter ::  RTLD_NOW             =  Z'0002'       
  integer *8 , parameter :: RTLD_GLOBAL           =  Z'0100'       
  private ::  get_ex_func


  interface dl_funcs
    function dlopenf(name,i)
      integer,pointer :: dlopenf        ! returns a pointer to an integer
      character*(*) name
      integer*8 i
      intent (in) :: name,i
    end function dlopenf

    function dlsymf(handle,name)
      integer,pointer :: dlsymf         ! returns a pointer to an integer
      character*(*) name
      integer :: handle
      intent(in) :: name
    end function dlsymf
  end interface

contains
!_______________________________________________________________
  subroutine extern_event_notify(this,event,obj)
    use dist_types

    type(remote_access) , intent(inout) :: this
    integer , intent(in) :: event,obj

    integer,pointer ::fptr
    external extern_dt_event

    fptr =>get_ex_func("cholesky.so","cholesky","dt_event")
    call extern_dt_event(fptr,this,event,obj)
  end subroutine extern_event_notify

!_______________________________________________________________
  subroutine extern_run_task(this,task)
    use dist_types
    type(remote_access) , intent(inout) :: this
    type(task_info) , intent(inout) :: task

    integer,pointer ::fptr
    external extern_dt_event

    fptr =>get_ex_func("cholesky.so","cholesky","dt_event")
    call extern_dt_event(fptr,this,EVENT_TASK_STARTED,task%id)
    
  end subroutine extern_run_task
!_______________________________________________________________
  subroutine extern_init(this)
    type(remote_access) , intent(inout) :: this

    integer,pointer ::fptr
    external extern_dt_init

    fptr =>get_ex_func("cholesky.so","cholesky","dt_init")
    call extern_dt_init(this,fptr)
  end subroutine extern_init
!_______________________________________________________________
   function get_ex_func(fname,mname,fn_name) result(fptr)
     character(len=*),intent (in) ::fname,mname,fn_name
     character(len=25) ::func_name,ff
     integer ,pointer ::ihandle , fptr
     ff=fname//char(0)
     TRACEX( "libname ",ff)
     ihandle => dlopenf(ff, RTLD_GLOBAL+RTLD_NOW)
     TRACEX("Lib handle",ihandle)
#ifdef LINUX
     func_name="__"//mname//"_MOD_"//fn_name//char(0)
#else
     func_name=mname//"."//fn_name//"_"//char(0)
#endif
     fptr => dlsymf(ihandle,func_name)  
     TRACEX("func name,fptr:",(func_name,fptr))

   end function get_ex_func

end module dist_externs

!_______________________________________________________________
  subroutine extern_dt_init(this,fptr)
    use dist_types
    type(remote_access) , intent(inout) :: this
    integer  :: fptr
    integer k
    external fptr
    TRACEX( "before fptr call:",k)
    k=fptr(this)
    
  end subroutine extern_dt_init
!_______________________________________________________________
  subroutine extern_dt_event(fptr,this,event,objid)
    use dist_types
    type(remote_access) , intent(inout) :: this
    integer, intent(in)::event,objid
    integer  :: fptr
    integer k
    external fptr
    TRACEX( "before Event Notify call:",(event,objid))
    k=fptr(this,event,objid)
  end subroutine extern_dt_event

