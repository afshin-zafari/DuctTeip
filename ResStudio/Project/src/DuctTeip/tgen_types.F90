module tgen_types
  use block 
  use remote_access_class

  integer , parameter :: READ_AXS  = 1
  integer , parameter :: WRITE_AXS = 2
  integer , parameter :: UNKNOWN   = -1
  integer , parameter :: TASK_NAME_LEN=10
  integer , parameter :: TASK_SIZE_LIMIT = 100
  integer , parameter :: DATA_SIZE_LIMIT = 50
  integer , parameter :: ERR_OUT_OF_REGION=-22

   type Data_Object
      type(matrix_block),pointer :: M=>NULL()
      integer            :: Id,host,version
      type(tlink_list),pointer :: tlist
      character(len=7)::name
   end type Data_Object
   type ptr_data_object
      type(data_object),pointer :: dp=>NULL()
   end type ptr_data_object

   type tgData_Access
      type(Data_Object) ,pointer:: dobj=>NULL()	  
      integer           :: Axs,version
   end type TgData_Access

   type Task 
      type(tgData_access),dimension(1:3) :: data_axs
      character(len=TASK_NAME_LEN)     :: Name
      integer                          :: Host,id,work
      logical :: executed
   end type Task

   type data_task_element
      integer :: ver,axs,id
      character(len=7)::name
   end type data_task_element
   
   type dlink_list
      type(dlink_list_node),pointer::first
   end type dlink_list

   type dlink_list_node
      integer :: item,obj
      type(data_object),pointer :: dp
      type(dlink_list_node),pointer::next
   end type dlink_list_node 

   type tlink_list
      type(tlink_list_node),pointer::first
   end type tlink_list

   type tlink_list_node
      type(task),pointer :: tp
      type(tlink_list_node),pointer::next
   end type tlink_list_node 


   type task_gen
      type(task)           , dimension(:)  , pointer :: tlist=>NULL()
      integer                                        :: cur_node,node_cnt,last_task,blk_per_row,my_rank,block_cnt
      integer                                        :: grp_size,grp_cols,grp_rows,data_cnt,sdc=-1,edc,stc=-1,etc,didx,tidx,task_cnt
      logical                                        :: hit_in_region
      type(remote_access)  ,                 pointer :: rma
      type(data_task_element) , dimension(:,:) , pointer :: matDT
      type(dlink_list) , pointer ::dlist
   end type task_gen

end module tgen_types
