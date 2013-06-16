
#ifdef PARTS_MNGR_DLEVEL 
#define DEBUG_LEVEL PARTS_MNGR_DLEVEL
#endif

#include "debug.h"

!  Module Header
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ External Dependencies
!!$      Point Set Class     Point Set functionalities
!!$      Workflow Class      For executing the RBF Matrix Assembly after receving the partitions
!!$      MPI                 MPI calls/structures 
!!$      FP                  floating point precesion definition
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ List of Types 
!!$     pts_partition
!!$     	structure for handling communications of a single Points Set 
!!$     pratition_mngr
!!$     	structure for managing all partitions of the point sets in send/receive operations
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ List of procedures
!!$
!!$        				Public 
!!$
!!$        Function   pmgr_new                  new the Partition Manager class
!!$        Subroutine pmgr_destruct		deletes the Partition Manager class
!!$        Subroutine pmgr_recv			called by Distribution object to receive Point Sets
!!$        Subroutine pmgr_compute		called after the receiving partitions are finished
!!$        Subroutine pmgr_send			called by Distribution object to send Point Sets
!!$        Subroutine pmgr_wait_for_recv	called by Distribution object to be sure that all incomplete sends are complete
!!$
!!$        				Private
!!$
!!$        Subroutine local_compute		the local computations in every process of the MPI world
!!$        Subroutine mat_name_gen		generates the names of the Point Set matrices for current process
!!$        Function   write_parts		prints out the partition passed in as input
!!$        Subroutine init_parts		initialization of the Partition Manager class
!!$        Function   get_next_pno		get next Partition No.
!!$        Function   get_pno			get current Partition No.
!!$        Subroutine set_pno			set next Partition No.
!!$        Function   get_pidx			get current free Partition Index
!!$        Subroutine set_pidx			set free Partition Index
!!$        Function   wait_for_any		waits for any of the receives to complete
!!$        Function   get_next_pidx		get next free Partition Index
!!$        Function   free_parts		deallocates all freed Partitions
!!$        Function   get_ridx			get current Request Index
!!$        Subroutine set_ridx			set current Request Index
!!$        Subroutine set_part_ridx		set corresponding request of the partition
!!$        Function   get_next_ridx		get next free Request Index
!!$        Subroutine set_sub_ids		set sub ID's of received point sets
!!$        Subroutine recv_all_data		receives Point Sets of neighbor 
!!$        Subroutine send_all_data		sends Point Sets to neighbor 
!!$        Subroutine send_ids			sends the ID component of Point Set
!!$        Subroutine send_pts			sends the Point Data of the Point Set
!!$        Function   generate_pts		merges all the received partitions and generate a single Point Set 
!!$------------------------------------------------------------------------------------------------------------------------------
!!$	   				Algorithm for Partitioning
!!$  	Phase I:
!!$		1) Root process : 
!!$		    - Reads Point Sets chunk by chunk and cyclically sends them to other processes 
!!$		      including itself by just copying them internally.
!!$		    - Saves the sent buffer and the 'requests' in an array of partitions.
!!$		    - When the array filled, waits for receive ack. and frees corresponding element.
!!$		    - Uses the freed element for next sends.
!!$		    - When EOF, sends a FINISH message and goes to Phase II.
!!$		2) Other Processes:
!!$		    - Receive the partitions from Root.
!!$		    - Save them in their array of partitions in the order of receive.
!!$		    - Repeat until FINISH message received and then go to Phase II.
!!$	Phase II:
!!$		1) Every Process:
!!$		    - Merges its received partitions into a single Point Set
!!$		    - Performs its 'local computations' on its own data
!!$		    - Sends its own partitions to its immediate right neighbor
!!$		    - Receives its left neighbor partitions
!!$		    - Performs its 'local computations' on its own partitions and its neighbor's
!!$		    - Repeats communications with its neighbors for N-Proc times (number of processes)
!!$------------------------------------------------------------------------------------------------------------------------------
!!$
!!$
!!$
!!$
!!$





Module parts_mngr_class

    Use mpi
    Use class_point_set
    Use wfm_class
    Use fp
    
    Implicit None 
    Private
    Public :: partition_mngr    ,pts_partition  
    Public :: pmgr_new          ,pmgr_destruct  ,pmgr_recv      ,pmgr_wait_for_recv, & 
              pmgr_compute      ,pmgr_send
    
     
    Integer,Parameter,Public  :: MAX_PARTS       = 20
    Integer,Parameter,Public  :: TAG_IDS_BLOCK   = 0
    Integer,Parameter,Public  :: TAG_PTS_BLOCK   = 1
    Integer,Parameter,Public  :: TAG_IDS_CYCLE   = 2
    Integer,Parameter,Public  :: TAG_PTS_CYCLE   = 3
    Integer,Parameter,Private :: ROOT            = 0
    Integer,Parameter,Private :: MAP_COL_NEWTYPE = 0

    Type pts_partition
        Type(point_set)         :: pts			! Point Set structure
        Logical                 :: filled       	! Is it filled or free
        Integer                 :: sent_to_rank 	! to which process it has been sent
        Integer,Dimension(1:2)  :: req_pair 		! what are the requests used for sending,
							!  one for ID's and one for point data
    End Type pts_partition

!!$----------------PARTITION MANAGER data structure------------------------------------------------------
!!$  wf			workflow manager variable
!!$  parts		array of partitions
!!$  root_parts		array of prtitions for root process, since root process only copies the point set 
!!$ 			and no need to send them 
!!$  part_map 		mapping of original ID's and new ID's which are generated after merging
!!$  req		array of request structures for controlling the non-blocking communications
!!$  sid_max		keeping track of maximum sub-ID's assigned to every original ID's
!!$  part_idx		index of current free element in partition array
!!$  root_idx		index of current free element in partition array for root process
!!$  part_no		last sent partition's number 
!!$  req_idx		index of current free element in request array
!!$  n_proc		no. of processes in MPI world
!!$  rank		rank/process Id of current running program
!!$  last_sent_rank	which process is the last to whom a partition is sent
!!$  chunk_pts		no. of points in every partition
!!$  nd			no. of dimensions of Point Sets
!!$  nt			no of types in Point Sets
!!$------------------------------------------------------------------------------------------------------
    
    Type partition_mngr
        Type(wfm)                                   :: wf
        Type(pts_partition),Dimension(1:MAX_PARTS)  :: parts , &
                                                       root_parts
        Integer            ,Dimension(:,:), Pointer :: part_map
        Integer            ,Dimension(1:MAX_PARTS*2):: req
        Integer            ,Dimension(:,:),Pointer  :: sid_max
        Integer                                     :: part_idx , root_idx , part_no , req_idx ,  & 
                                                       n_proc   , rank     , last_sent_rank , & 
                                                       chunk_pts , nd , nt
    End Type partition_mngr


    Contains 

        Function pmgr_new(wf,rank,n_proc,chunk_pts,nd,nt) Result (this)
        
            Type(wfm)            ,Intent(in)    :: wf
            Type(partition_mngr)                :: this
            Integer              ,Intent(in)    :: n_proc,rank,chunk_pts,nd,nt
            
            TRACE1("PMNGR_NEW","ENTER")
            
            this%rank      = rank
            this%n_proc    = n_proc
            this%chunk_pts = chunk_pts
            this%nd        = nd
            this%nt        = nt
            this%wf        = wf

            Allocate( this%sid_max(1:nt,1:2) )
            
            Call init_parts(this)
            
            TRACE1("PMNGR_NEW","EXIT")
            
        End Function pmgr_new

        Subroutine pmgr_destruct(this)

            Type(partition_mngr)    ,Intent(inout)  :: this
            Integer                                 :: pidx 
            
            TRACE1("PMGR_DESTRUCT","ENTER")
            
            Do pidx=1,Ubound(this%parts,1)
                If ( this%parts(pidx)%filled ) Then 
                    Call destruct_pts( this%parts(pidx)%pts ) 
                End If 
            End Do 
            Deallocate ( this%sid_max ) 
            
            TRACE1("PMGR_DESTRUCT","EXIT")
            
        End Subroutine pmgr_destruct
            
        Subroutine init_parts(this)
        
            Type(partition_mngr),Intent(inout)  :: this
            Integer                             :: i 

            TRACE1("INIT_PARTS","ENTER")
            
            Do i=1,this%nt
                this%sid_max(i,1) = i-1
            End Do 
            
            this%sid_max(:,2)           = -1
            this%req                    = MPI_REQUEST_NULL            
            this%parts(:)%req_pair(1)   = MPI_REQUEST_NULL
            this%parts(:)%req_pair(2)   = MPI_REQUEST_NULL
            
            this%last_sent_rank         = -1
            this%part_idx               = 0
            this%req_idx                = 0
            this%part_no                = -1
            this%root_idx               = 1
            this%root_parts(:)%filled   = .False.
            this%parts(:)%filled        = .False.
            
            TRACE1("INIT_PARTS","EXIT")

        End Subroutine init_parts

        Subroutine pmgr_recv(this)

            Type(partition_mngr)                        , Intent(inout) :: this
            Type(point_set)                                             :: pts
            Real(kind=rfp)  , Dimension(:,:)            , Pointer       :: ptr        
            Integer         , Dimension(:,:)            , Pointer       :: ids
            Integer         , Dimension(MPI_STATUS_SIZE)                :: sts
            Integer                                                     :: err,pcnt,buf_len,pidx,l

            TRACE1("PMGR_RECV","ENTER")
            
            Do 
                Allocate( ids(1:this%nt,1:ID_COL_MAX) )
                buf_len = this%nt * Ubound(ids,2) * sizeof( ids(1,1) )
                
                Call MPI_RECV(ids,buf_len,MPI_BYTE,ROOT,TAG_IDS_BLOCK,MPI_COMM_WORLD,sts,err)
                
                TRACE2("Recv Data from ROOT,err=",err)
                
                pidx = get_next_pidx(this,.True.)
                pcnt = Maxval(ids(:,ID_COL_TO))
                
                TRACE2("recvd IDs",(ids(l,:),l=1,Ubound(ids,1)))
                
                If ( pcnt == 0 ) Then   ! FINISHED Msg received 
                    TRACE2("FINISH msg recvd",pidx)
                    Exit
                End If 
                
                pcnt = this%chunk_pts
                Allocate( ptr(1:pcnt,1:this%nd) )
                buf_len  = pcnt * this%nd * sizeof(ptr(1,1))
                
                Call MPI_RECV(ptr(:,:),buf_len,MPI_BYTE,ROOT,TAG_PTS_BLOCK,MPI_COMM_WORLD,sts,err)
                
                this%parts(pidx)%pts    = new_pts(ids,ptr)
                this%parts(pidx)%filled = .True.
                
                ids=>get_ids_ptr(this%parts(pidx)%pts)
                
                TRACE2("put in parts()%ids",pidx)
                TRACE2("recvd IDs,after set in parts",(ids(l,:),l=1,Ubound(ids,1)))
               
                Call set_pidx(this,pidx)
            End Do   ! All partitions are received

            Call pmgr_compute(this)
            
            TRACE1("PMGR_RECV","EXIT")            

        End Subroutine pmgr_recv

        Subroutine local_compute(this,p1)

            Type(pts_partition )     ,Dimension(1:MAX_PARTS) ,Intent(in)     :: p1
            Type(partition_mngr)                             ,Intent(inout)  :: this            
            Type(point_set     )                                             :: all_pts

            TRACE1("local_compute","ENTER")
            
            TEST_ONLY ( call mat_name_gen(this,p1) )  

            all_pts = generate_pts (this,p1)
            Call wfm_set_new_pts( this%wf,all_pts )
            Call wfm_execute( this%wf , (/ "Result" /) )    
            Call destruct_pts(all_pts)

            If (Associated (this%part_map) ) Deallocate(this%part_map)
            
            TRACE1("local_compute","EXIT")
            
        End Subroutine local_compute

        Subroutine mat_name_gen(this,p1)

            Type(pts_partition)     ,Dimension(1:MAX_PARTS) ,Intent(in)     :: p1
            Type(pts_partition)     ,Dimension(1:MAX_PARTS)                 :: p2
            Type(partition_mngr)                            ,Intent(inout)  :: this
            Integer                 ,Dimension(:,:)         ,Pointer        :: i_ids,j_ids            
            Type(point_set)                                                 :: all_pts
            Integer                                                         :: i,j,ii,jj,id1,id2,sid1,sid2,pno1,pno2,l

#           define TRACE TRACE5
#           define SEP SEP5

            p2 = this%parts
            TRACE("p1_u",Ubound(p1,1))
            Do i = 1,Ubound(p1,1)
                TRACE("i",i)
                If (.Not. p1(i)%filled) Cycle
                i_ids => get_ids_ptr(p1(i)%pts)
                If (.Not. Associated(i_ids)) Cycle 
                TRACE("i_ids_l",Lbound(i_ids,1)  ) SEP TRACE("i_ids_u",Ubound(i_ids,1)  ) SEP
                TRACE("i_ids",(i_ids(l,:),l=Lbound(i_ids,1),Ubound(i_ids,1) ) )
                Do ii = Lbound(i_ids,1), Ubound(i_ids,1)
                    TRACE("ii",ii) SEP TRACE("from",i_ids(ii,ID_COL_FROM)) SEP TRACE("to",i_ids(ii,ID_COL_TO)) SEP TRACE("ii",ii) 
                    If (i_ids(ii,ID_COL_TO) == 0 .Or. i_ids(ii,ID_COL_FROM)== 0 ) Cycle
                    id1  = i_ids(ii,ID_COL_TYPE) SEP TRACE(" id1", id1)
                    sid1 = i_ids(ii,ID_COL_SID ) SEP TRACE("sid1",sid1)
                    pno1 = i_ids(ii,ID_COL_PNO ) SEP TRACE("pno1",pno1)
                    Do j=1,Ubound(p2,1)
                        TRACE("j",j)            
                        If (.Not. p2(j)%filled) Cycle
                        j_ids => get_ids_ptr(p2(j)%pts)
                        If (.Not. Associated(j_ids)) Cycle             
                        TRACE("j_ids",(j_ids(l,:),l=1,Ubound(j_ids,1) ) )
                        Do jj = Lbound(j_ids,1), Ubound(j_ids,1)
                            TRACE("jj",jj) SEP TRACE("from",j_ids(jj,ID_COL_FROM)) SEP TRACE("to",j_ids(jj,ID_COL_TO))
                            If (j_ids(jj,ID_COL_TO) == 0 .Or. j_ids(jj,ID_COL_FROM) == 0 ) Cycle
                            id2  = j_ids(jj,ID_COL_TYPE) SEP TRACE(" id2", id2)
                            sid2 = j_ids(jj,ID_COL_SID ) SEP TRACE("sid2",sid2)
                            pno2 = j_ids(jj,ID_COL_PNO ) SEP TRACE("pno2",pno2)
                            If ( id1 <= id2 ) Then 
                                Write(*,*) "(",this%rank,"):matrix", &
                                "_P"//Achar(pno1+48)//"_I"//Achar(id1+48)//"_S"//Achar(sid1+48)// & 
                                "_P"//Achar(pno2+48)//"_J"//Achar(id2+48)//"_S"//Achar(sid2+48)
				Write(*,*) "MAT",this%rank,pno1,id1,sid1,pno2,id2,sid2
                            End If 
                        End Do ! jj
                    End Do ! j 
                End Do ! ii
            End Do ! i 
        End Subroutine mat_name_gen

        Function write_parts(parts) Result(ii)

            Type(pts_partition) ,Dimension(1:MAX_PARTS) ,Intent(in) :: parts            
            Integer             ,Dimension(:,:)         ,Pointer    :: ids
            Integer                                                 :: i,ii

            Write(*,*) "WRITE_PARTS ","ENTER"
            Do i = 1,Ubound(parts,1)

                If (.Not. parts(i)%filled) Cycle

                ids => get_ids_ptr(parts(i)%pts)

     		    If (.Not. Associated(ids)) Cycle

                Write(*,*) "(",i,"):",( ids(ii,:),ii=1,Ubound(ids,1) )
                Write(*,*) "(",i,"):",parts(i)%sent_to_rank,parts(i)%req_pair        

                ii=i

            End Do

            Write(*,*) "WRITE_PARTS ","EXIT"
            
        End Function write_parts
        
        Subroutine pmgr_compute(this,root)

            Integer             ,Dimension(MPI_STATUS_SIZE,1:MAX_PARTS*2+1) :: sts
            Type(pts_partition) ,Dimension(1:MAX_PARTS)                     :: send_data,recv_data
            Type(partition_mngr),Intent(inout)                              :: this
            Integer             ,Dimension(1:MAX_PARTS*2+1)                 :: send_req            
            Integer                                                         :: i,right,left,buf_len,err
            Logical             ,Optional                                   :: root
			
#           define TRACE TRACE5
#           define SEP SEP5

            TRACE1("PMGR_COMPUTE","ENTER")

			send_req = 0 

            If (Present(root) .And. root) Then
                TRACE2("Root Parts",write_parts(this%root_parts))
                this%parts = this%root_parts 
            End If
            
            right   = Mod(this%rank +1,this%n_proc)
            left    = Mod(this%rank -1,this%n_proc)
            buf_len = sizeof(send_data)

            send_data = this%parts
            
            TRACE2("This Parts",write_parts(this%parts))
            TRACE2("Send Parts",write_parts(send_data ))

            Call local_compute ( this, send_data)

            Do i= 1,this%n_proc -1
            
                Call send_all_data(this,send_data,right,send_req) 
                TRACE2("send req"  ,send_req)
                Call recv_all_data(this,recv_data,left          ) 
                TRACE2("Recv Parts",write_parts(recv_data))
                Call local_compute(this,recv_data)
                
                Call MPI_WAITALL(Ubound(send_req,1),send_req,sts,err)
                
                !TODO:return the result ( save / send)
                send_data = recv_data
                
		TRACE2("New Send Parts",write_parts(send_data))
                !TODO:destruct recv data
            End Do
            if ( this%rank == 0 )  Write(*,*) "MAT",-1, 0,0,0, 0,0,0  

            TRACE1("PMGR_COMPUTE","EXIT")

        End Subroutine pmgr_compute
        
        Subroutine pmgr_send(this,pts,flag)
        
            Type(partition_mngr)    ,Intent(inout)  :: this
            Type(point_set)         ,Intent(in)     :: pts
            Integer                 ,Intent(in)     :: flag

            TRACE1("PMGR_SEND","ENTER")
            TRACE2("REQ",this%req(:10))
            TRACE2("PIDX",this%part_idx)
            TRACE2("RIDX",this%req_idx)

            Call send_ids(this,pts)
            Call send_pts(this)

            TRACE1("PMGR_SEND","EXIT")
            
        End Subroutine pmgr_send

       
       Function get_next_pno(this) Result(pno)
           
           Type(partition_mngr),Intent(inout) :: this
           Integer :: pno
            
           pno = this%part_no + 1
           TRACE4("Next Pno",pno)
           
       End Function get_next_pno

       Function get_pno(this) Result(pno)

           Type(partition_mngr),Intent(inout) :: this
           Integer :: pno 
           
           pno = this%part_no 
           TRACE4("Cur Pno",pno)
           
       End Function get_pno       
       
       Subroutine set_pno(this,pno) 
       
           Type(partition_mngr),Intent(inout) :: this
           Integer :: pno 
           
           this%part_no = pno
           TRACE4("Pno<--",pno)
           
       End Subroutine set_pno

       Function get_pidx(this) Result(pidx)
       
           Type(partition_mngr),Intent(inout) :: this
           Integer :: pidx 

           pidx = this%part_idx 
           TRACE3("Pidx ",pidx)

       End Function get_pidx     
       
       Subroutine set_pidx(this,pidx) 

           Type(partition_mngr),Intent(inout) :: this
           Integer :: pidx 

           this%part_idx = pidx
           TRACE3("Pidx <--",pidx)

       End Subroutine set_pidx
       

       Function wait_for_any(this) Result(ridx)
       
            Type(partition_mngr),Intent(inout)  :: this
            Integer                             :: ridx,cnt,sts(MPI_STATUS_SIZE),err
            
            TRACE3("WAIT_FOR_ANY","ENTER")            
            Call MPI_WAITANY(cnt,this%req,ridx,sts,err)            
            TRACE3("WAIT_FOR_ANY","EXIT")
            
       End Function wait_for_any
       
       Subroutine pmgr_wait_for_recv(this)

            Integer,Dimension(MPI_STATUS_SIZE,1:MAX_PARTS*2) :: sts
            Type(partition_mngr)    ,Intent(inout)           :: this
            Integer                                          :: err,l

            TRACE3("PMGR_WAIT_FOR_RECV","ENTER")            

            TRACE3("this%req",this%req(:))
            TRACE3("this%req->size",Ubound(this%req,1))

            Call MPI_WAITALL( Ubound(this%req,1) , this%req , sts , err )            

            TRACE3("this%req",this%req(:))
            TRACE3("err",err) SEP TRACE3("sxs code",MPI_SUCCESS) 
            TRACE3("sts",(sts(l,:),l=1,MPI_STATUS_SIZE) )

            TRACE3("PMGR_WAIT_FOR_RECV","EXIT")            

       End Subroutine pmgr_wait_for_recv
       
       Function get_next_pidx(this,for_recv) Result(pidx)
       
           Type(partition_mngr) ,Intent(inout)          ::  this
           Logical              ,Intent(in)   ,Optional ::  for_recv
           Integer                                      ::  pidx,i,rq_idx

           TRACE1("GET_NEXT_PIDX","ENTER")
           
           If (Present(for_recv) .And. for_recv) Then 
               pidx = this%part_idx+1
               TRACE1("GET_NEXT_PIDX","EXIT")
               Return 
           End If

           pidx = 0 

           Do i=1,Ubound(this%parts,1)
                If (this%parts(i)%req_pair(1) == MPI_REQUEST_NULL .And. & 
                    this%parts(i)%req_pair(2) == MPI_REQUEST_NULL ) Then 
                    pidx = i
                    Exit
                End If 
           End Do
           
           Do While  (pidx == 0 ) 

                rq_idx = wait_for_any(this)
                pidx   = free_parts(this,rq_idx)

           End Do
           
           TRACE1("GET_NEXT_PIDX","EXIT")
           
       End Function get_next_pidx

       Function free_parts(this,rq_idx) Result(pidx)
       
            Type(partition_mngr),Intent(inout)  :: this
            Integer                             :: rq_idx,pidx,i
            
            TRACE1("FREE_PARTS","ENTER")
            
            pidx = 0 
            Do i = 1 , Ubound(this%parts,1)
                If (this%parts(i)%req_pair(1) == rq_idx ) Then 
                    this%parts(i)%req_pair(1) = MPI_REQUEST_NULL
                End If 
                If (this%parts(i)%req_pair(2) == rq_idx ) Then 
                    this%parts(i)%req_pair(2) = MPI_REQUEST_NULL
                End If 
                If (this%parts(i)%req_pair(1) == MPI_REQUEST_NULL .And. &
                    this%parts(i)%req_pair(2) == MPI_REQUEST_NULL) Then 
                    pidx = i                    
                    Call destruct_pts(this%parts(pidx)%pts)
                End If 
            End Do
            
            TRACE1("FREE_PARTS","EXIT")
            
       End Function free_parts    

       Function get_ridx(this) Result(ridx)
       
           Type(partition_mngr),Intent(inout) :: this
           Integer :: ridx 

           ridx = this%req_idx 

           TRACE4("Req Idx ",ridx)

       End Function get_ridx     
       
       Subroutine set_ridx(this,ridx) 

           Type(partition_mngr),Intent(inout) :: this
           Integer :: ridx 
           
           this%req_idx  = ridx
           TRACE4("Req Idx<-- ",ridx)
           
       End Subroutine set_ridx

       Subroutine set_part_ridx(this,ridx) 
       
           Type(partition_mngr),Intent(inout)   :: this
           Integer                              :: ridx ,pidx
           
           TRACE3("SET_PART_IDX","ENTER")
       
           pidx = get_pidx(this)
           
           If ( this%parts(pidx)%req_pair(1) == MPI_REQUEST_NULL) Then
                this%parts(pidx)%req_pair(1) =  ridx
           Else
                this%parts(pidx)%req_pair(2) =  ridx
           End If 
           
           this%req_idx  = ridx
           
           TRACE3("SET_PART_IDX","EXIT")
           
       End Subroutine set_part_ridx

       
       Function get_next_ridx(this) Result(ridx)
       
            Type(partition_mngr),Intent(inout)  :: this
            Integer                             :: ridx,i

            TRACE2("GET_NEXT_RIDX","ENTER")
            
            ridx = 0
            Do i=1,Ubound(this%req,1)
                If (this%req(i) == MPI_REQUEST_NULL) Then 
                    ridx = i
                    Exit
                End If 
            End Do        
            
            If ( ridx == 0 ) Then 
                ridx = wait_for_any(this)
                   i = free_parts(this,ridx)
            End If     
            
            TRACE2("GET_NEXT_RIDX","EXIT")
            
       End Function get_next_ridx
       

       Subroutine set_sub_ids(this,ids)

            Type(partition_mngr)    ,Intent(inout)      :: this
            Integer, Dimension(:,:) ,Intent(inout)      :: ids
            Integer                                     :: i,j,l
            
            TRACE2("SET_SUB_IDS","ENTER")
            
            Do i = 1,Ubound(ids,1)
            
                If (ids(i,ID_COL_TO) == 0 .Or.  ids(i,ID_COL_FROM) == 0 ) Cycle
                
                Do j=1,Ubound(this%sid_max,1)
                
                    If ( ids(i,ID_COL_TYPE) == this%sid_max(j,1) ) Then 
                    
                         ids(i,ID_COL_SID ) =  this%sid_max(j,2)+1
                         this%sid_max(j,2)  =  this%sid_max(j,2)+1
                        
                        Exit
                        
                    End If 
                End Do
            End Do 
            
            TRACE2("MAX SID",( this%sid_max(:,:)))! ,l=1,ubound(this%sid_max,1) ) )
            
            TRACE2("SET_SUB_IDS","EXIT")
            
       End Subroutine set_sub_ids

       Subroutine recv_all_data(this,recv_data,src)       
            Type(pts_partition) ,Dimension(1:MAX_PARTS) ,Intent(inout)  :: recv_data
            Type(partition_mngr)                        ,Intent(inout)  :: this
            Type(point_set)                                             :: pts
            Real(kind=rfp)      ,Dimension(:,:)         ,Pointer        :: ptr        
            Integer                                     ,Intent(in)     :: src
            Integer             ,Dimension(:,:)         ,Pointer        :: ids
            Integer             ,Dimension(MPI_STATUS_SIZE)             :: sts
            Integer                                                     :: err,pcnt,buf_len,pidx,ridx,max_ridx,l

            TRACE1("RECV_ALL_DATA","ENTER")
            
            max_ridx = Ubound(recv_data,1)*2
            ridx = 0

            ! clear recv_data
            Do pidx = 1,Ubound(recv_data,1)
            
			    recv_data(pidx)%filled = .False.
			    					 
                TRACE2("pidx",pidx) SEP2 TRACE2("ridx",ridx)
                
                If (ridx >max_ridx) Then 
                    TRACE2("Ridx reached max_ridx",max_ridx) SEP2 TRACE2("                 ridx",ridx)
                    Exit
                End If 
                
                Allocate(ids(1:this%nt,1:ID_COL_MAX))
                buf_len = this%nt * Ubound(ids,2) * sizeof(ids(1,1))
                
                TRACE2("ids buf_len",(buf_len,"") )
                Call MPI_RECV(ids(:,:),buf_len,MPI_BYTE,src,TAG_IDS_CYCLE,MPI_COMM_WORLD,sts,err)
                
                TRACE2("ids",( ids(l,:),l=1,this%nt) )
                
                If (Maxval(ids(:,ID_COL_TO)) == 0 ) Then 
                
                    max_ridx = ids(1,ID_COL_TYPE)
                    
                    TRACE2("FINISH CYCLE recvd",max_ridx) SEP2 TRACE2("fin msg",(ids(l,:),l=1,this%nt) )
                    
                    If (ridx >=max_ridx) Then 
                        Exit
                    End If 
                    
                    TRACE2("Ridx not reached to max","")
                    
                    Cycle
                    
                End If 

                ridx = ridx+1           SEP2 TRACE2("ridx"  ,ridx)                
                pcnt = this%chunk_pts   SEP2 TRACE2("ptr ##",pcnt)

                Allocate(ptr(1:pcnt,1:this%nd))
                buf_len  = pcnt * this%nd * sizeof(ptr(1,1))
                
                TRACE2("ptr buf_len",buf_len)

                Call MPI_RECV(ptr(:,:),buf_len,MPI_BYTE,src,TAG_PTS_CYCLE,MPI_COMM_WORLD,sts,err)
                
                ridx = ridx+1           SEP2 TRACE2("ridx",ridx)
                
                recv_data(pidx)%pts    = new_pts(ids,ptr)
				recv_data(pidx)%filled = .True.

            End Do
            
            TRACE1("RECV_ALL_DATA","EXIT")
            
       End Subroutine recv_all_data
       
       Subroutine send_all_data(this,send_data,dest,send_req)

            Type(pts_partition) ,Dimension(1:MAX_PARTS)     ,Intent(in)     :: send_data
            Type(partition_mngr)                            ,Intent(inout)  :: this
            Type(point_set)                                                 :: pts
            Integer             ,Dimension(1:MAX_PARTS*2+1) ,Intent(out)    :: send_req
            Integer             ,Dimension(MPI_STATUS_SIZE)                 :: sts
            Integer             ,Dimension(:,:)             ,Pointer        :: ids
            Integer             ,Dimension(:,:)             ,Pointer        :: finish_msg
            Real(kind=rfp)      ,Dimension(:,:)             ,Pointer        :: ptr        
            Integer                                                         :: dest,pidx,ridx,err,buf_len,icnt,pno,pcnt,l
            
            TRACE1("SEND_ALL_DATA","ENTER")
            
            !clear parts 
            Do pidx = 1,MAX_PARTS

                If (.Not. send_data(pidx)%filled) Then 
                    TRACE2("Not filled and skipped",pidx)
                    Cycle
                End If 
                
                pts =  send_data(pidx)%pts
                ids => get_ids_ptr(pts)
                
                If (.Not. Associated(ids)) Then 
                    TRACE2("Not allocated and skipped",pidx)
                    Cycle
                End If 
                
                icnt    = Ubound(ids,1)
                buf_len = icnt * Ubound(ids,2) * sizeof(ids(1,1))
                ridx    = pidx *2-1
                
                TRACE2("ids ub",Ubound(ids,1)) SEP2 TRACE2("ids buf_len",buf_len)
                
                Call MPI_ISEND(ids(:,:),buf_len,MPI_BYTE,dest,TAG_IDS_CYCLE,MPI_COMM_WORLD,send_req(ridx),err)
                
                pcnt = this%chunk_pts
                ptr => get_pts_ptr(pts)

                buf_len = pcnt * Ubound(ptr,2) * sizeof(ptr(1,1))
                
                TRACE2("ptr ub",Ubound(ptr,1)) SEP2 TRACE2("ptr ##",pcnt) SEP2 TRACE2("ptr buf_len",buf_len)
                TRACE2("ptr SZ",Ubound(ptr,1) * Ubound(ptr,2) * sizeof(ptr(1,1)))
                
                Call MPI_ISEND(ptr(:,:),buf_len,MPI_BYTE,dest,TAG_PTS_CYCLE,MPI_COMM_WORLD,send_req(ridx+1),err)
                
				TRACE2("send_req",(send_req) )
				
            End Do 

            Allocate(finish_msg(1:this%nt,1:ID_COL_MAX) )
            buf_len = this%nt * Ubound(finish_msg,2) * sizeof(finish_msg(1,1))
            
            finish_msg(:,:          ) = 0
            finish_msg(1,ID_COL_TYPE) = ridx+1

            TRACE2("FINISH CYCLE sent,ridx",ridx+1) SEP2 TRACE2("                 ,buf_len  ",buf_len)
            
            Call MPI_ISEND(finish_msg(:,:),buf_len,MPI_BYTE,dest,TAG_IDS_CYCLE,MPI_COMM_WORLD,send_req(Ubound(send_req,1)),err)
            
            TRACE2("fin msg sent",(finish_msg(l,:),l=1,ID_COL_MAX) )
            
            TRACE1("SEND_ALL_DATA","EXIT")
            
       End Subroutine send_all_data

       Subroutine send_ids(this,pts)

            Type(partition_mngr)            ,Intent(inout)  :: this
            Type(point_set)                 ,Intent(in)     :: pts
            Real(kind=rfp)  ,Dimension(:,:) ,Pointer        :: ptr        
            Integer         ,Dimension(:,:) ,Pointer        :: ids
            Integer                                         :: dest,pidx,ridx,err,buf_len,icnt,pno,req,l

            TRACE1("SEND_IDS","ENTER")
            
            dest = Mod( this%last_sent_rank +1,this%n_proc)
            
            this%last_sent_rank  = dest
            
            If (dest == ROOT ) Then 
            
                ids => get_ids_ptr(pts)
                ptr => get_pts_ptr(pts)
                
                this%root_parts(this%root_idx)%pts    = new_pts(ids,ptr)
                this%root_parts(this%root_idx)%filled = .True.
                
                ids => get_ids_ptr(this%root_parts(this%root_idx)%pts)
                this%root_idx = this%root_idx +1 
                
                Call set_sub_ids(this,ids)
                
                TRACE2("root Idx",this%root_idx -1 ) SEP2 TRACE2("root IDs",(ids(l,:),l=1,Ubound(ids,1) ) )
                
                TRACE1("SEND_IDS","EXIT")
                
                Return 
            End If 
            
            pidx = get_next_pidx(this)
            ridx = get_next_ridx(this)
            pno  = get_next_pno (this)
            
            this%parts(pidx)%sent_to_rank   = dest
            this%parts(pidx)%pts            = pts
            
            ids => get_ids_ptr(this%parts(pidx)%pts)
            
            icnt = Ubound(ids,1)
            ids(:,ID_COL_PNO) = pno
            
            Call set_sub_ids(this,ids)
            
            buf_len = icnt * Ubound(ids,2) * sizeof(ids(1,1))
            
            Call MPI_ISEND(ids(:,:),buf_len,MPI_BYTE,dest,TAG_IDS_BLOCK,MPI_COMM_WORLD,req,err)
            
            this%req(ridx)=req
            
            TRACE2("sent IDs",(ids(l,:),l=1,Ubound(ids,1) ) )
            
            Call set_pidx     (this,pidx)
            Call set_part_ridx(this,ridx)
            Call set_ridx     (this,ridx)
            Call set_pno      (this,pno )
            
            TRACE1("SEND_IDS","EXIT")
            
       End Subroutine send_ids
       
       Subroutine send_pts(this)

            Real(kind=rfp)  ,Dimension(:,:) ,Pointer        :: ptr        
            Type(partition_mngr)            ,Intent(inout)  :: this
            Type(point_set)                                 :: pts
            Integer                                         :: pcnt,ridx,pidx,dest,err,buf_len,req

            TRACE1("SEND_PTS","ENTER")

            dest =  this%last_sent_rank 

            If (dest == ROOT ) Then 
                TRACE1("SEND_PTS","EXIT")
                Return 
            End If 

            pidx =  get_pidx(this)
            pts  =  this%parts(pidx)%pts
            ridx =  get_next_ridx(this)
            pcnt =  this%chunk_pts
            ptr  => get_pts_ptr(pts)
            
            buf_len = pcnt * Ubound(ptr,2) * sizeof(ptr(1,1))
            
            Call MPI_ISEND(ptr(:,:),buf_len,MPI_BYTE,dest,TAG_PTS_BLOCK,MPI_COMM_WORLD,req,err)
            
            this%req(ridx)= req
            
            TRACE2("REQ",this%req(:10))
            
            Call set_part_ridx(this,ridx)
            Call set_ridx(this,ridx)
            
            TRACE1("SEND_PTS","EXIT")
            
       End Subroutine send_pts
       
       
       Function generate_pts(this,in_parts) Result(pts)
       
            Type(partition_mngr)                            , Intent(inout) :: this
            Type(pts_partition)     ,Dimension(1:MAX_PARTS)                 :: in_parts
            Type(point_set)         ,Dimension(:)           , Pointer       :: pts_list
            Integer                 ,Dimension(:,:)         , Pointer       :: ids            
            Logical                                                         :: ok
            Type(point_set)                                                 :: pts
            Integer                                                         :: pcnt,pcnt1,pcnt2,i,l,midx,j,np,nt

            TRACE1("GENERATE_PTS","ENTER")
            
            pcnt1 = Count( this%parts(:)%filled ) 
            pcnt2 = Count(   in_parts(:)%filled )  
            pcnt  = pcnt1 + pcnt2
            
            Allocate ( pts_list(1:pcnt) , this%part_map(1:pcnt*this%nt,0:ID_COL_MAX)) 
            
            this%part_map = 0
            
            pts_list(1      :pcnt1) = Pack(this%parts(:)%pts , this%parts(:)%filled )
            pts_list(pcnt1+1:pcnt ) = Pack(  in_parts(:)%pts ,   in_parts(:)%filled )

            TRACE2("Pcnt1",pcnt1) SEP2 TRACE2("Pcnt2",pcnt2) SEP2 TRACE2("Pcnt",pcnt)

            midx = 1
            np   = 0
            
            Do i = 1,pcnt
                TRACE2("i",i)
                ids => get_ids_ptr(pts_list(i))
                
                If ( .Not. Associated(ids) ) Cycle
                
                np = np + Maxval(ids(:,ID_COL_TO) ) 
                
                TRACE2("NO.Pts",np) SEP2 TRACE2("original ids",(ids(l,:),l=1,this%nt) )                
                
                Do j= 1,this%nt
                
                    If ( ids(j,ID_COL_FROM) == 0 .Or. ids(j,ID_COL_FROM) == 0) Cycle
                
                    this%part_map(midx,1:ID_COL_MAX) = ids(j,:)
                
                    ids(j,ID_COL_TYPE) = midx -1 
                    this%part_map(midx,MAP_COL_NEWTYPE) = midx -1 
                    
                    midx = midx + 1
                    
                End Do
                
                TEST_ONLY( ids => get_ids_ptr(pts_list(i)) )
                
                TRACE2("generated ids,new",(ids(l,:),l=1,this%nt) )                

            End Do
            
            TRACE2("part map",(this%part_map(l,:),l=1,Ubound(this%part_map,1) ) )            

            nt  = midx
            pts = new_pts(np,this%nd,nt)
            
            TRACE2("np,nd,nt",(np,this%nd,nt) ) 
            
            Do i = 1,pcnt
            
                ok = add_pts(pts,pts_list(i) )
                
                If (.Not. ok) Then 
                    TRACE2("add pts failed ,pidx",i)
                    Exit
                End If 
            End Do 
            
            TEST_ONLY(  ids => get_ids_ptr(pts)  )
            TRACE2("generate ids,org",(ids(l,:),l=1,Ubound(ids,1) ) )                
            
            TRACE1("GENERATE_PTS","EXIT")
            
       End Function generate_pts
End Module parts_mngr_class
