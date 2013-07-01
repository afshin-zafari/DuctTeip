# 1 "dist_const.F90"
Module dist_const

  Implicit None

  Integer,Parameter :: FROM_IDX= 1
  Integer,Parameter :: TO_IDX= 2
  Integer,Parameter :: SYNC_TYPE_NONE	   = 0

  Integer,Parameter :: WL_NODE_ID = 1
  Integer,Parameter :: WL_WEIGHT  = 2

  Integer,Parameter :: MPI_TAG_START= 0

  Integer,Parameter :: MPI_TAG_DATA                = MPI_TAG_START + 1
  Integer,Parameter :: MPI_TAG_LSNR                = MPI_TAG_START + 2
  Integer,Parameter :: MPI_TAG_TASK                = MPI_TAG_START + 3

  Integer,Parameter :: SYNC_TYPE_SENDING_TASKS     = MPI_TAG_START + 4
  Integer,Parameter :: SYNC_TYPE_LAST_TASK         = MPI_TAG_START + 5
  Integer,Parameter :: SYNC_TYPE_DATA_FREE         = MPI_TAG_START + 6
  Integer,Parameter :: SYNC_TYPE_TERM_OK           = MPI_TAG_START + 7


  Integer,Parameter :: MPI_TAG_TASK_STEAL          = MPI_TAG_START + 8
  Integer,Parameter :: MPI_TAG_TASK_STEAL_RESPONSE = MPI_TAG_START + 9
  Integer,Parameter :: MPI_TAG_LSNR_REDIRECT       = MPI_TAG_START + 10
  Integer,Parameter :: MPI_TAG_WORK_LOAD           = MPI_TAG_START + 11
  Integer,Parameter :: MPI_TAG_TRANSFER_TASK       = MPI_TAG_START + 12

  Integer,Parameter :: MPI_TAG_SYNC_LAST_TASK      = SYNC_TYPE_LAST_TASK
  Integer,Parameter :: MPI_TAG_SYNC_DATA_FREE      = SYNC_TYPE_DATA_FREE
  Integer,Parameter :: MPI_TAG_SYNC_TERM_OK        = SYNC_TYPE_TERM_OK

  Integer,Parameter :: TTYPE_TASK_STEAL_ACCEPTED    = 1 
  Integer,Parameter :: TTYPE_TASK_STEAL_REJECTED    = 2


  Integer,Parameter :: MAX_TASK_NAME = 15
  Integer,Parameter :: MAX_DATA_NAME = 25

  Integer,Parameter :: AXS_TYPE_READ   = 1
  Integer,Parameter :: AXS_TYPE_WRITE  = 10
  Integer,Parameter :: AXS_TYPE_MODIFY = 100
  Integer,Parameter :: AXS_TYPE_LBOUND = 1
  Integer,Parameter :: AXS_TYPE_UBOUND = 3

  Integer,Parameter :: MIN_MSG_SIZE=1024
  Integer,Parameter :: MAX_MSG_SIZE=10240
  Logical,Parameter :: SPLIT_MSG=.False.
  Logical,Parameter :: PACK_MSG=.False.

  Integer,Parameter :: TASK_INVALID_ID = -1
  Integer,Parameter :: DATA_INVALID_ID = -1
  Integer,Parameter :: LSNR_INVALID_ID = -1

  Integer,Parameter :: COORDINATOR_PID = 0

  Integer , Parameter :: COMM_STS_SEND_INIT 		= 100
  Integer , Parameter :: COMM_STS_SEND_PROGRESS 	= 200
  Integer , Parameter :: COMM_STS_SEND_COMPLETE 	= 300
  
  Integer , Parameter :: TASK_STS_INITIALIZED 	= 1
  Integer , Parameter :: TASK_STS_WAIT_FOR_DATA = 2
  Integer , Parameter :: TASK_STS_READY_TO_RUN 	= 3
  Integer , Parameter :: TASK_STS_SCHEDULED 	= 4
  Integer , Parameter :: TASK_STS_INPROGRESS 	= 5
  Integer , Parameter :: TASK_STS_FINISHED 	= 6
  Integer , Parameter :: TASK_STS_CLEANED 	= 7

  Integer , Parameter :: DATA_STS_INITIALIZED 	= 1
  Integer , Parameter :: DATA_STS_READ_READY 	= 2
  Integer , Parameter :: DATA_STS_MODIFY_READY 	= 3
  Integer , Parameter :: DATA_STS_CLEANED 	= 4
  Integer , Parameter :: DATA_STS_REDIRECTED 	= 5

  Integer , Parameter :: LSNR_STS_INITIALIZED 	= 1
  Integer , Parameter :: LSNR_STS_ACTIVE	= 2
  Integer , Parameter :: LSNR_STS_DATA_RCVD 	= 3
  Integer , Parameter :: LSNR_STS_RCVD 		= 4
  Integer , Parameter :: LSNR_STS_TASK_WAIT 	= 5
  Integer , Parameter :: LSNR_STS_DATA_SENT 	= 6
  Integer , Parameter :: LSNR_STS_DATA_ACK 	= 7
  Integer , Parameter :: LSNR_STS_CLEANED 	= 8

  Integer , Parameter :: MAX_LSNR_LIST 	= 5000
  Integer , Parameter :: MAX_TASK_LIST 	= 5000
  Integer , Parameter :: MAX_DATA_LIST 	= 5000
  Integer , Parameter :: MAX_DATA_AXS_IN_TASK	= 4

  Integer , Parameter :: VERSION_DONT_CARE = 0
  Integer , Parameter :: STATUS_DONT_CARE  = 0
  Integer , Parameter :: EVENT_DONT_CARE   = 0 

  Integer , Parameter :: EVENT_DATA_RECEIVED 	=  1019       ! data id, 0,0
  Integer , Parameter :: EVENT_DATA_ACK 	=  2       ! data id, 0,0
!!$ VIZIT line consists of :
!!$ 'VIZ' time_stamp node_id event id1 id2 id3
                                                           ! id1 , id2 ,id3
  Integer , Parameter :: EVENT_LSNR_RECEIVED 	= 10       ! lsnr id, 0,0
  Integer , Parameter :: EVENT_LSNR_ACK 	= 20       ! lsnr id, 0,0

  Integer , Parameter :: EVENT_TASK_RECEIVED 	= 100      ! task id, 0,0
  Integer , Parameter :: EVENT_TASK_ACK	 	= 200      ! task id, 0,0
  Integer , Parameter :: EVENT_TASK_STARTED 	= 300      ! task id, tot task cnt,0
  Integer , Parameter :: EVENT_TASK_RUNNING 	= 400      ! task id, tot task cnt,0
  Integer , Parameter :: EVENT_TASK_FINISHED 	= 500      ! task id, tot task cnt,0

  Integer , Parameter :: EVENT_ALL_TASK_FINISHED = 600     ! 0, 0,0
!VIZIT Extra Events
  Integer , Parameter :: EVENT_DATA_STS_CHANGED     = 1000 ! data id ,0,0
  Integer , Parameter :: EVENT_TASK_STS_CHANGED     = 1001 ! task id, 0,0 
  Integer , Parameter :: EVENT_LSNR_STS_CHANGED     = 1002 ! lsnr id, 0,0
  Integer , Parameter :: EVENT_DATA_ADDED           = 1003 ! data id, 0,0
  Integer , Parameter :: EVENT_TASK_ADDED           = 1004 ! task id, 0,0
  Integer , Parameter :: EVENT_DATA_POPULATED       = 1005 ! data id, 0,0
  Integer , Parameter :: EVENT_LSNR_CLEANED         = 1006 ! lsnr id, 0,0
  Integer , Parameter :: EVENT_LSNR_DATA_RECEIVED   = 1007 ! lsnr id, 0,0
  Integer , Parameter :: EVENT_LSNR_DATA_SENT       = 1008 ! lsnr id, 0,0
  Integer , Parameter :: EVENT_TASK_CHECKED         = 1009 ! task id, 0,0
  Integer , Parameter :: EVENT_TASK_CLEANED         = 1010 ! task id, 0,0
  Integer , Parameter :: EVENT_DATA_SEND_REQUESTED  = 1011 ! data id, dest,0
  Integer , Parameter :: EVENT_LSNR_SENT_REQUESTED  = 1012 ! lsnr id, dest,0
  Integer , Parameter :: EVENT_TASK_SEND_REQUESTED  = 1013 ! task id, dest,0
  Integer , Parameter :: EVENT_CYCLED               = 1014 ! 0      , 0,0
  Integer , Parameter :: EVENT_LSNR_ADDED           = 1015 ! lsnr id, 0,0
  Integer , Parameter :: EVENT_DATA_DEP             = 1016 ! task id, data id,access type
  Integer , Parameter :: EVENT_SUBTASK_STARTED 	    = 1017 ! task id, sub task id, thrno
  Integer , Parameter :: EVENT_SUBTASK_FINISHED     = 1018 ! task id, sub task id, thrno

  Integer , Parameter :: EVENT_DTLIB_DIST           = 1020 
  Integer , Parameter :: EVENT_DTLIB_EXEC     	    = 1021 
  Integer , Parameter :: EVENT_SRCH_LIST     	    = 1022


  Integer , Parameter :: TASK_OBJ  	= 1
  Integer , Parameter :: DATA_OBJ  	= 2
  Integer , Parameter :: LSNR_OBJ  	= 3

  Integer , Parameter :: NOTIFY_OBJ_DATA  	= 1
  Integer , Parameter :: NOTIFY_OBJ_TASK  	= 10
  Integer , Parameter :: NOTIFY_OBJ_LISTENER  	= 100
  Integer , Parameter :: NOTIFY_OBJ_SCHEDULER  	= 1000
  Integer , Parameter :: NOTIFY_OBJ_MAILBOX  	= 10000
  Integer , Parameter :: NOTIFY_DONT_CARE  	= 0


  Integer , Parameter :: xSTG_TASKS_CREATING  		= 0
  Integer , Parameter :: xSTG_TASKS_SCATTERING  	= 1
  Integer , Parameter :: xSTG_TASKS_EXECUTION  		= 2
  Integer , Parameter :: xSTG_TASKS_RESULT_GATHERING  	= 3
  Character(len=*),Parameter :: TNAME_ASM_MAT   = "AsmblMat"
  Character(len=*),Parameter :: TNAME_CALC_DIST = "CalcDist"
  Character(len=*),Parameter :: TNAME_CHOL_DIAG = "Diag"
  Character(len=*),Parameter :: TNAME_CHOL_PNLU = "PnlU"
  Character(len=*),Parameter :: TNAME_CHOL_XADD = "XAdd"
  Character(len=*),Parameter :: TNAME_CHOL_MMUL = "MMul"
  Character(len=*),Parameter :: TNAME_CHOL_MADD = "MAdd"
  Character(len=*),Parameter :: TNAME_INIT_DATA = "Init"


End Module dist_const
