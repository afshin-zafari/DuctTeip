#include "debug.h"
module matasm
   use dist_common
   use ductteip
   implicit none
contains
!---------------------------------------------------------------------------------------------------------------------------
  function dt_init(opt,wf,pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt) result (rma) 

    Integer         , Intent(in) :: pid,part_cnt,part_size,node_cnt,dim_cnt,np,nt
    Type(wfm)       , Intent(in) :: wf
    Type(options)   , Intent(in) :: opt
    Type(remote_access)          :: rma
    Integer 		         :: cyc = 0,tc,err,event=EVENT_DONT_CARE,ids_element,lnp,lnd,lnt
    Type(data_handle)            :: Data
    Type ( point_set)            :: pts
    Character(len=5)             :: env
    Character(len=MAX_DATA_NAME) :: dname
    Integer                      :: part_idx = 1 ,time_out,node_owns_data,i,j=1,bcast_mtd,seq,import_tasks,lbm,tasks_node,cholesky
    Integer(kind=8)              :: task_dst
    Integer(kind=8)              :: time,drdy,dlag,dnow
    Real(kind=rfp)::start
    Type(task_info)::task
    Type(listener)::lsnr
    Logical :: populated = .False.
    character(len=STR_MAX_LEN)::tasks_file



    
!TODO: Options     
    seq = opt_get_option(rma%opt,OPT_SEQUENTIAL)
    if ( seq /= 0 .and. import_tasks ==0) then 
!TODO: timings     
       TIMING(EVENT_DTLIB_EXEC,0,0,0)
       call rma_mat_assemble_seq(rma)
       TIMING(EVENT_DTLIB_EXEC,0,0, 0)
       return
    end if
    node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
    TIMING(EVENT_DTLIB_DIST,0,0,0)
    if ( pid == 0  ) then 
       task_dst = opt_get_option(rma%opt,OPT_TASK_DISTRIBUTION)
       if ( task_dst == TASK_DIST_ONCE ) then 
          tc = rma_generate_tasks_part(rma)
       else
          Call rma_generate_1task_4all(rma)
       end if
       if (node_owns_data ==0 )  call set_pts_zero(pts)
    end if
  end function dt_init
!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_mat_assemble_seq(this)
    Type(remote_access) , Intent(inout)	           :: this
    Real(kind=rfp)      , Dimension(:,:), Pointer  :: d,r
    Real(kind=rfp)      , Dimension(:,:), Pointer  :: x
    Integer                                        :: i,j,np,nd,nb,seqchunk,l,k
    Character(len=80)			           :: phi
    Character				           :: nprime
    Real(kind=rfp)			           :: eps

    np = opt_get_option(this%opt,OPT_CHUNK_PTS)
    seqchunk = opt_get_option(this%opt,OPT_SEQ_CHUNK)
    nb = Max(np/seqchunk,1)
    If ( nb == 1 ) seqchunk = np
    write(*,*)  "SEQ_CHUNK,nb"," : ",(seqchunk,nb) 
    Do l=1,nb
       Do k=1,nb
          Allocate(x(seqchunk,1:2),d(seqchunk,seqchunk),r(seqchunk,seqchunk))
          Do i=1,Ubound(x,1)
             Do j=1,Ubound(x,1)
                d(i,j)= Sqrt((x(j,1)-x(i,1))**2+(x(j,2)-x(i,2))**2)
             End Do
          End Do
          eps = 2.0
          phi="gauss"
          nprime='0'
          nd = 2

          Call dphi(phi, nprime, nd, eps, d, r)
          Deallocate(x,d,r)
       End Do
    End Do


  End Subroutine rma_mat_assemble_seq

!------------------------------------------------------------------------------------------------------
  Function rma_generate_tasks_part(this) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: this
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,k,l,t_cnt ,owner,node_owns_data,bcast_mtd,d_cnt,di_pid,dj_pid
    Character :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii


    axs(:)%dproc_id = -1
    axs(:)%dname    = ""
    t_name = TNAME_ASM_MAT
    t_cnt  = 0


    bcast_mtd = opt_get_option(this%opt,OPT_BROADCAST_MTD)
    node_owns_data = opt_get_option(this%opt,OPT_NODE_AUTONOMY)
    

    Do i = 0 , this%node_cnt-1       
       pid = Mod(i,this%node_cnt)
       If ( pid < this%proc_id ) Cycle
       If ( pid > this%proc_id + this%group_size-1) Cycle

       Do j = 0 , this%node_cnt-1
          Do k = 1,this%pc
             Do l = 1, this%pc
                pi_name  = "P"//Trim(to_str(i))//"_"//Trim(to_str(k))
                pj_name  = "P"//Trim(to_str(j))//"_"//Trim(to_str(l))
                
                di_pid = COORDINATOR_PID
                dj_pid = COORDINATOR_PID
                
                If ( bcast_mtd == BCASTMTD_PIPE .And. pid > 0 ) Then 
                   di_pid = pid -1
                   dj_pid = pid -1
                End If
                If (node_owns_data /= 0 ) Then 
                   di_pid=i
                   dj_pid=j
                End If
                pi = rma_create_data(this,pi_name,this%part_size,this%nd,v,di_pid)
                 
                pj = rma_create_data(this,pj_name,this%part_size,this%nd,v,dj_pid)
                 

                dij_name = "D"//to_str(i)//"_"//to_str(k)//"--"//to_str(j)//"_"//to_str(l)
                dij = rma_create_data(this,dij_name,this%part_size,this%part_size,v,pid)
                 

                axs(1) = rma_create_access ( this ,  pi , AXS_TYPE_READ  )
                axs(2) = rma_create_access ( this ,  pj , AXS_TYPE_READ  )
                axs(3) = rma_create_access ( this , dij , AXS_TYPE_WRITE )
                Do ii = 4,Ubound(axs,1)
                   nullify(axs(ii)%data)
                End Do

                 
                t = rma_create_task ( this , t_name , pid , axs ,1)
                t_cnt = t_cnt + 1 
             End Do
          End Do
       End Do
    End Do
    Call print_lists(this)


  End Function rma_generate_tasks_part
!!$------------------------------------------------------------------------------------------------------------------
  Subroutine rma_generate_1task_4all(this)
    Type(remote_access) , Intent(inout)  :: this
    Integer                              :: i,j

    Do i = 0 , this%pc-1
       Call rma_generate_single_task(this,i)
    End Do


  End Subroutine rma_generate_1task_4all
!!$------------------------------------------------------------------------------------------------------------------

  Subroutine rma_generate_single_task(this,pid,idx)
    Integer             , Intent(in)  , Optional   :: idx
    Type(remote_access) , Intent(inout)  :: this
    Integer             , Intent(in)     :: pid
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,d
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,t_cnt ,owner,node_owns_data,bcast_mtd,i,j
    Character                            :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer 		    	  	 :: ii

    If ( Present(idx)  ) Then 
       write(*,*)  "Gen Single Task pid,idx"," : ",(pid,idx)
       If ( this%task_list(idx)%id == TASK_INVALID_ID )  then 
          Return 
       end if 

       Read (this%task_list(idx)%axs_list(1)%data%name,"(A1 I2)") dummy,i
       Read (this%task_list(idx)%axs_list(2)%data%name,"(A1 I2)") dummy,j
       j= j+1
       If ( j >= this%node_cnt ) Return 
    Else
       write(*,*)  "Gen Single Task pid,w/out idx"," : ",pid
       i = pid
       j = pid 
    End If
    write(*,*)  "Task for pid, Di,Dj"," : ",(pid,i,j)


    t_name = TNAME_ASM_MAT

    pi_name  = "P"//Trim(to_str(i))
    pj_name  = "P"//Trim(to_str(j))
    dij_name = "D"//to_str(i)//"_"//to_str(j)

    
    d = data_find_by_name_dbg(this,pi_name,"remote_access_class.F90",1375)
    If ( d%id == DATA_INVALID_ID ) Then 
       pi = rma_create_data(this,pi_name,this%part_size,this%nd,v,COORDINATOR_PID)
    End If
    d = data_find_by_name_dbg(this,pj_name,"remote_access_class.F90",1379)
    If ( d%id == DATA_INVALID_ID ) Then 
       pj = rma_create_data(this,pj_name,this%part_size,this%nd,v,COORDINATOR_PID)
    End If
    
    d = data_find_by_name_dbg(this,dij_name,"remote_access_class.F90",1384)
    If ( d%id == DATA_INVALID_ID ) Then 
       d = rma_create_data(this,dij_name,this%part_size,this%part_size,v,pid)
    End If
    
    axs(1) = rma_create_access ( this ,  pi , AXS_TYPE_READ  )
    axs(2) = rma_create_access ( this ,  pj , AXS_TYPE_READ  )
    axs(3) = rma_create_access ( this ,  d  , AXS_TYPE_WRITE )
    Do ii = 4,Ubound(axs,1)
       nullify(axs(ii)%data)
    End Do

    bcast_mtd = opt_get_option(this%opt,OPT_BROADCAST_MTD)
    If (bcast_mtd == BCASTMTD_PIPE) Then 
       If ( pid > 0 ) Then 
          axs(1)%data%proc_id = pid -1
          axs(2)%data%proc_id = pid -1
       End If
    End If
    node_owns_data = opt_get_option(this%opt,OPT_NODE_AUTONOMY)
    If (node_owns_data /= 0 ) Then 
       Read (axs(1)%data%name,"(A1 I2)") dummy,owner
       
       axs(1)%data%proc_id=owner
       Read (axs(2)%data%name,"(A1 I2)") dummy,owner
       
       axs(2)%data%proc_id=owner
    End If
    t = rma_create_task ( this , t_name , pid , axs ,1)


  End Subroutine rma_generate_single_task


!!$------------------------------------------------------------------------------------------------------------------
!!$  Task_ij = {NodeID,'AssembleMatrix', Read(P_i),Read(P_j), Write(D_ij)}
!!$      The Task_xj's are sent to node x for computing the D_xj.
!!$      All the P_i and P_j Data has a CORDINATOR_PID as host ID which is the root node.
!!$      Therefore all the tasks requests for the P_i Data from the Root.
!!$      That is, all nodes send listeners to Root node for P_i.
!!$
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$      Read P_i   Read P_i    Read P_i   Read P_i  
!!$      Read P_j   Read P_j    Read P_j   Read P_j
!!$      Write D_0j Write D_1j  Write D_2j Write D_3j
!!$      
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$       P_i  ---> P_i 
!!$       P_i  ---------------> P_i 
!!$       P_i  --------------------------> P_i
!!$------------------------------------------------------------------------------------------------------------------
  Function rma_generate_tasks(rma,part_cnt,part_size,node_cnt,dim_cnt) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: rma
    Integer             , Intent(in)     :: part_cnt,part_size,node_cnt,dim_cnt
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,t_cnt ,owner,node_owns_data
    Character :: dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii


    t_name = TNAME_ASM_MAT
    t_cnt  = 0

    Do i = 0 , part_cnt-1

       pi_name  = "P"//Trim(to_str(i))
       pid = Mod(i,node_cnt)

       Do j = 0 , part_cnt-1
          pj_name  = "P"//Trim(to_str(j))
          pi = rma_create_data(rma,pi_name,part_size,dim_cnt,v,COORDINATOR_PID)
          pj = rma_create_data(rma,pj_name,part_size,dim_cnt,v,COORDINATOR_PID)
          
          dij_name = "D"//to_str(i)//"_"//to_str(j)
          dij = rma_create_data(rma,dij_name,part_size,part_size,v,pid)

          axs(1) = rma_create_access ( rma ,  pi , AXS_TYPE_READ  )
          axs(2) = rma_create_access ( rma ,  pj , AXS_TYPE_READ  )
          axs(3) = rma_create_access ( rma , dij , AXS_TYPE_WRITE )
          Do ii = 4,Ubound(axs,1)
             nullify(axs(ii)%data)
          End Do

          node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
          If (node_owns_data /= 0 ) Then 
             Read (axs(1)%data%name,"(A1 I2)") dummy,owner
             axs(1)%data%proc_id=owner
             Read (axs(2)%data%name,"(A1 I2)") dummy,owner
             axs(2)%data%proc_id=owner
          End If
          t = rma_create_task ( rma , t_name , pid , axs ,1)
          t_cnt = t_cnt + 1
       End Do
    End Do

    write(*,*)  "DataObject#"," : ",t_cnt*3
    write(*,*)  "TaskObject#"," : ",t_cnt 

  End Function rma_generate_tasks
!!$------------------------------------------------------------------------------------------------------------------
!!$  Task_ij = {NodeID,'AssembleMatrix', Read(P_i),Read(P_j), Write(D_ij)}
!!$      The Task_xj's are sent to node x for computing the D_xj.
!!$      All the P_i for Task_xj are read from node 'x-1' for x>0. Root node does not request any Data.
!!$      Therefore all the tasks requests for the P_i Data from their left neighbor.(in line topology)
!!$
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$      Read P_i   Read P_i    Read P_i   Read P_i  
!!$      Read P_j   Read P_j    Read P_j   Read P_j
!!$      Write D_0j Write D_1j  Write D_2j Write D_3j
!!$      
!!$      Node0      Node1       Node2      Node3
!!$      ---------  ---------   ---------- ----------
!!$       P_i ----> P_i  ------> P_i -----> P_i
!!$------------------------------------------------------------------------------------------------------------------

  Function rma_generate_tasks_pipeline(rma,part_cnt,part_size,node_cnt,dim_cnt) Result(t_cnt)

    Type(remote_access) , Intent(inout)  :: rma
    Integer             , Intent(in)     :: part_cnt,part_size,node_cnt,dim_cnt
    Character (len=MAX_DATA_NAME)        :: pi_name,pj_name,dij_name,t_name
    Type(data_handle)		 	 :: pi,pj,dij
    Type(task_info)		  	 :: t 
    Integer 		    	  	 :: v= 0,pid,i,j,t_cnt ,owner,node_owns_data
    Character::dummy
    Type(data_access)   , &
       Dimension(1:MAX_DATA_AXS_IN_TASK) :: axs
    Integer                             :: ii

    t_name = TNAME_ASM_MAT
    t_cnt  = 0

    Do i = 0 , part_cnt-1

       pi_name  = "P"//Trim(to_str(i))
       pid = Mod(i,node_cnt)

       Do j = 0 , part_cnt-1
          pj_name  = "P"//Trim(to_str(j))
          pi = rma_create_data(rma,pi_name,part_size,dim_cnt,v,COORDINATOR_PID)
          pj = rma_create_data(rma,pj_name,part_size,dim_cnt,v,COORDINATOR_PID)
          
          dij_name = "D"//to_str(i)//"_"//to_str(j)
          dij = rma_create_data(rma,dij_name,part_size,part_size,v,pid)

          axs(1) = rma_create_access ( rma ,  pi , AXS_TYPE_READ  )
          axs(2) = rma_create_access ( rma ,  pj , AXS_TYPE_READ  )
          axs(3) = rma_create_access ( rma , dij , AXS_TYPE_WRITE )
          Do ii = 3,Ubound(axs,1)
             nullify(axs(ii)%data)
          End Do

          If ( pid > 0 ) Then 
             axs(1)%data%proc_id = pid -1
             axs(2)%data%proc_id = pid -1
          End If
          node_owns_data = opt_get_option(rma%opt,OPT_NODE_AUTONOMY)
          If (node_owns_data /= 0 ) Then 
             Read (axs(1)%data%name,"(A1 I2)") dummy,owner
             
             axs(1)%data%proc_id=owner
             Read (axs(2)%data%name,"(A1 I2)") dummy,owner
             
             axs(2)%data%proc_id=owner
          End If
          t = rma_create_task ( rma , t_name , pid , axs ,1)
          t_cnt = t_cnt + 1

       End Do
    End Do
    write(*,*)  "DataObject#"," : ",t_cnt*3
    write(*,*)  "TaskObject#"," : ",t_cnt 

  End Function rma_generate_tasks_pipeline

  subroutine dt_event()

  end subroutine dt_event

end module matasm
