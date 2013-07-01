# 1 "dist_types.F90"


!!$ ---------------------------Data types for Task Distribution functionality over the nodes-----------------------------
!!$ Data_Handle Type: 
!!$    Name               Unique name of the Data 
!!$    ID                 Unique ID of the Data
!!$    Proc_ID            Processor ID (rank in MPI) that hosts the Data
!!$    nRows,nCols        Number of rows and columns of the Data (which is genarally seen as a matrix)
!!$    Version            Current Version of the Data according to the Task Executions on this Data
!!$    sv_r               Scheduler Version for Read Access Type
!!$    sv_w               Scheduler Version for Write Access Type
!!$    sv_m               Scheduler Version for Modify/Add Access Type
!!$    nt                 Number of Types in the Point Set in the Data
!!$    np                 Number of Points in the Point Set in the Data
!!$    nd                 Number of Dimension for every point in the data Point Set
!!$    status             Last/Current status of the Data object
!!$    has_lsnr           Flag that shows whether any listener is defined for the data or no
!!$    pts                Ponint Set data structure containing  the coordinates of the Data Points
!!$    buf                Pointer to the contigous memory holding all the meta-data and Data Contents.
!!$-----------------------------------------------------------------------------------------------------------------------
!!$ Data_Access Type:
!!$    Data               Data Handle structure to which any type of Access is needed
!!$    dproc_id           The host (processor) id of the data owner 
!!$    access_types       What kinds of accesses
!!$    match_status       Last status of matching the Access with the Data version
!!$    min_ver            The required Minimum version of Data 
!!$-----------------------------------------------------------------------------------------------------------------------
!!$ Task_Info  Type:
!!$    axs_list           List of Data the task needs 
!!$    name               Name of the Task,used also for calling real executable Tasks understandable by Task Library
!!$    id                 Unique ID of the Task
!!$    status             The Last ststaus of the task in the communications and/or executions
!!$    proc_id            The processor ID of Task host
!!$    type               Is it a Sync task or not, and what kind of Sync
!!$-----------------------------------------------------------------------------------------------------------------------
!!$ Listener Type:
!!$    dname              The Data to which it listens, used for remote Data
!!$    min_ver            The minimum version of the requested Data, used for remote Data.
!!$    req_data           The actual Data pointer used locally.
!!$    id                 Unique ID of the listener
!!$    status             The last status of Listener in communications
!!$    proc_id            The processor ID of the node that the listener resides
!!$-----------------------------------------------------------------------------------------------------------------------
!!$ Scheduler
!!$    event              What event happened for any Task
!!$    task               Which task gets new status
!!$-----------------------------------------------------------------------------------------------------------------------
!!$ Remote_Access Type:
!!$    USE_MPI_RMA        Whether to use MPI-2 RMA or not. (not used)
!!$    USE_MPI_COMM       Whether to use MPI-1 Communications or not. 
!!$    mpi_comm           MPI Communication Context
!!$    proc_id            The porcessor ID of the nodes that hosts the program
!!$    sync_stage         Which synchronization stage is the program in
!!$    event              What is the last event
!!$    notify             Which object(MailBox,Task,Listener..) has to be notified 
!!$    data_buf_size      Size of the Data send/receive buffer
!!$    nd                 Number of Dimensions in Point Set of Data
!!$    np                 Number of Points in every Data object
!!$    nt                 Number of Point Types in every Data object
!!$    pc                 Partition Count of the whole Point Sets
!!$    schd               Scheduler object
!!$    task_list          List of Tasks objects(dynamically allocated)
!!$    data_list          List of Data  objects(dynamically allocated)
!!$    lsnr_list          List of Listeners(dynamically allocated)
!!$    task_req           List of MPI REQUEST objects used for Task communications(fixed)
!!$    data_req           ''                ''                 Data      '' 
!!$    lsnr_req           ''                ''                 Listener  '' 
!!$    in_task            Task object used as a receiving buffer 
!!$    in_data            Data       ''   ''     ''            ''
!!$    in_lsnr            Listener   ''   ''     ''            ''
!!$    data_buf           Buffer of contigous area of memory used for sending/receiving the Data contents 
!!$    send_list          List of ready to send objects,used for checking the Outbox
!!$    tlist_ready_sts    List of ready tasks for execution
!!$-----------------------------------------------------------------------------------------------------------------------


Module dist_types

  Use fp
  Use wfm_class
  Use dist_const
  Use options_class
  Use class_point_set
  use link_list_class

  Implicit None

	Type data_handle
    	  Character(len=MAX_DATA_NAME) 		    :: name
       	  Integer 				    :: id,proc_id,nrows,ncols,version,sv_r,sv_w,sv_m,nt,np,nd
    	  Integer 				    :: status,has_lsnr
	  Type(point_set)			    :: pts
    	  Character(len=1) , Dimension(:),Pointer   :: buf
          Real(kind=rfp) , dimension(:,:) , Pointer :: mat=>NULL()
    	  Character(len=1) , Dimension(:),Pointer   :: sendbuf=>NULL()
          Type(dm)         ,Pointer                 :: dmngr
	End Type data_handle

	Type data_access
   	  Integer 	                :: access_types , match_status , min_ver,dproc_id
   	  Character(len=MAX_DATA_NAME)	:: dname
          Type(data_handle),Pointer     :: Data=>NULL()
 	End Type data_access

	Type task_info 
    	  Integer 			 			:: id,status,proc_id,Type,task_idx,weight
          Type(data_access),Dimension(1:MAX_DATA_AXS_IN_TASK)   :: axs_list
   	  Character(len=MAX_TASK_NAME) 	 			:: name
          Type(dm)         ,Pointer                             :: dmngr
	End Type task_info

	Type listener
   	  Character(len=MAX_DATA_NAME)	:: dname
          Type(data_handle)   , Pointer :: req_data=>NULL()
	  Integer                       :: id,status, proc_id,min_ver
	End Type listener

 	Type scheduler
     	  Type(task_info) :: task
       	  Integer         :: event
  	End Type scheduler


	Type remote_access

  	  Logical :: USE_MPI_RMA,USE_MPI_COMM,single_load_sent
          Integer :: mpi_comm,proc_id,sync_stage,event,notify,node_capacity,group_size
          Integer :: data_buf_size,nd,np,nc,pc,task_cnt,node_cnt,part_size,task_steal_dest
          Type(wfm ) :: wf
          Type(options) :: opt

    	  Type(task_info)  , Dimension(:), Pointer 	:: task_list
     	  Type(data_handle), Dimension(:), Pointer 	:: data_list
     	  Type(listener)   , Dimension(:), Pointer 	:: lsnr_list

     	  Integer          , Dimension(:,:), Pointer 	:: work_load_list


     	  Integer          , Dimension(:),Pointer 	:: data_req
     	  Integer 	   , Dimension(:),Pointer 	:: task_req
     	  Integer          , Dimension(:),Pointer 	:: lsnr_req

          Character(len=1) , Dimension (:)     ,Pointer :: data_buf,data_buf2
          Type(link_list)  , Pointer                    :: send_list,tlist_ready_sts
	End Type remote_access

End Module dist_types
