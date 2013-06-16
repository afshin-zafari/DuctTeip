#include "debug.h"
module ductteip_lib
    use dist_common
    use dist_types
    use dist_data_class
    use dist_task_class
    implicit none  
contains
    Function rma_create_data(this,name,nr,nc,version,pid) Result(dh)

    Type(remote_access) ,Intent(inout) 					:: this
    Integer		,Dimension(AXS_TYPE_LBOUND:AXS_TYPE_UBOUND)     :: vers
    Type(data_handle)   ,Pointer 					:: dh
    Type(data_handle)                 					:: data
    Integer 		              					:: pid,nr,nc,version
    Character(len=*) 	              					:: name

    

    Data = data_find_by_name_dbg(this,name,"remote_access_class.F90",674)
    If ( data%id /= DATA_INVALID_ID ) Then 
       dh => this%data_list(data%id)
    Else
       dh =>  data_create(this,name,nr,nc,version,pid)
    End If

  End Function rma_create_data
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_create_task(this,name,proc,axs_list,wght,sync) Result(task)

    Type(remote_access),Intent(inout)   	:: this
    Type(task_info)    ,Pointer 		:: task
    Type(data_access)  ,Dimension(:),Intent(in) :: axs_list
    Integer            ,Intent(in),Optional     :: sync
    Integer            ,Intent(in) 	        :: proc,wght
    Character(len=*) 				:: name
    Integer                                     :: i

    
    
    Allocate(task)
    task%name     = name
    task%proc_id  = proc
    task%axs_list = axs_list
    task%status   = TASK_STS_INITIALIZED
    task%weight   = wght
    Nullify(task%dmngr)
    If (Present(sync)) Then 
       task%type = sync
    Else
       task%type = SYNC_TYPE_NONE 
    End If

       
    Call task_add_to_list(this,task)
    If ( task%type /= SYNC_TYPE_NONE) Then 
       Return
    End If
    Do i = Lbound(task%axs_list,1),Ubound(task%axs_list,1)
       If ( .Not. Associated(task%axs_list(i)%data) ) Then 
          Cycle
       End If
          
       If (task%axs_list(i)%data%id /= DATA_INVALID_ID) Then 
          If ( task%axs_list(i)%access_types == AXS_TYPE_READ ) Then 
             VIZIT4(EVENT_DATA_DEP,task%id,task%axs_list(i)%data%id,0)
          Else
             VIZIT4(EVENT_DATA_DEP,task%id,task%axs_list(i)%data%id,1)
          End If
       Else
       End If
    End Do
    

  End Function rma_create_task
!!$------------------------------------------------------------------------------------------------------------------
  
  Function rma_create_access(this,data,rwm) Result(daxs)

    Type(remote_access) , Intent(inout) :: this
    Integer 		, Intent(in)    :: rwm    
    Type(data_access)  			:: daxs
    Type(data_handle)   , Intent(in)   	:: Data

    daxs%access_types = rwm
    daxs%dname=data%name
    daxs%data => this%data_list(Data%id)
    daxs%dproc_id = data%proc_id
    daxs%min_ver=data%version

  End Function rma_create_access

end module ductteip_lib
