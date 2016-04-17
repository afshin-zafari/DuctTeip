Module options_class

  Use constants

Implicit None

Integer         , Parameter :: MAX_LINE_LEN = 100
Integer         , Parameter :: STR_MAX_LEN = 100
Integer         , Parameter :: MAX_OPTIONS  = 100
Integer         , Parameter :: BCASTMTD_PIPE  = 1
Integer         , Parameter :: BCASTMTD_12ALL = 2
Integer         , Parameter :: TASK_DIST_ONCE = 0
Integer         , Parameter :: TASK_DIST_1BY1 = 1
Integer         , Parameter :: LBM_NONE  = 0 ! NOT Load Balance 
Integer         , Parameter :: LBM_TASK_STEALING  = 1 ! Load Balance Method=Stealing tasks from others
Integer         , Parameter :: LBM_TASK_EXPORTING = 2 ! Load Balance Method=Exporting extra tasks to others
Integer         , Parameter :: LBM_NODES_GROUPING = 3 ! Load Balance Method=nodes are grouped and report the load to central node,recursively

Character(len=1), Parameter :: ASSIGN_TOKEN = "="
Character(len=1), Parameter :: COMMENT_TOKEN = "#"

Integer  , Parameter :: OPT_IDX_START               = 1
Integer  , Parameter :: OPT_READ_RESOURCE_FROM_FILE = 2
Integer  , Parameter :: OPT_CHUNK_PTS               = 3
Integer  , Parameter :: OPT_BLOCK_CNT               = 4
Integer  , Parameter :: OPT_BLOCK_CNT2              = 20
Integer  , Parameter :: OPT_TLIB_THREAD_COUNT       = 5
Integer  , Parameter :: OPT_DATA_READY_LAG          = 6
Integer  , Parameter :: OPT_NODE_AUTONOMY           = 7
Integer  , Parameter :: OPT_TEST_CORRECTNESS        = 8
Integer  , Parameter :: OPT_SAVE_RESULTS            = 9
Integer  , Parameter :: OPT_BROADCAST_MTD           = 10
Integer  , Parameter :: OPT_PURE_MPI                = 11
Integer  , Parameter :: OPT_TASK_DISTRIBUTION       = 12
Integer  , Parameter :: OPT_IMPORT_TASKS            = 13
Integer  , Parameter :: OPT_SEQUENTIAL              = 14
Integer  , Parameter :: OPT_SEQ_CHUNK               = 15
Integer  , Parameter :: OPT_LOAD_BALANCE            = 16
Integer  , Parameter :: OPT_GROUP_SIZE              = 17
Integer  , Parameter :: OPT_NODE_CAPACITY           = 18
Integer  , Parameter :: OPT_CHOLESKY                = 19
Integer  , Parameter :: OPT_PART_CNT                = 21
Integer  , Parameter :: OPT_TIME_OUT                = 22
Integer  , Parameter :: OPT_TPL_TYPE                = 23
Integer  , Parameter :: OPT_TPL_LEVELCNT            = 24
Integer  , Parameter :: OPT_TPL_NODESCNT            = 25
Integer  , Parameter :: OPT_CHOL_PART_CNT           = 26
Integer  , Parameter :: OPT_CHOL_CACHEOPT           = 27
Integer  , Parameter :: OPT_CHOL_GRP_ROWS           = 28
Integer  , Parameter :: OPT_CHOL_GRP_COLS           = 29
Integer  , Parameter :: OPT_DIST_BLK_CNT            = 30
Integer  , Parameter :: OPT_IDX_END                 = MAX_OPTIONS !$$  == 100
Integer  , Parameter :: STR_OPT_CHOL_TASKS_FILE     = 1
Integer  , Parameter :: STR_OPT_SCHED_DIR           = 2

 Type options
    Integer,Dimension(1:MAX_OPTIONS) :: opt_list
    Logical :: READ_INFO_FROM_FILE = .True.
    Character(len=FILE_NAME_LENGTH)     :: fname
    character(len=STR_MAX_LEN),dimension(1:MAX_OPTIONS) :: str_opt_list
    Integer :: CHUNK_PTS = 20
 End Type options
!==============	Optimization Schemes and Alternatives	=======================
!-----------------------------------------------------------------------------
!   Multiple Epsilon Values
!	1) every process asks root for the next epsilon
!		(-): extra unnecessary communications
!	2) at the startup, every process chooses its own epsilons
!		(-):wf class has to be able to execute for multiple epsilons
!	Multiple Values for Phi and Epsilon
!	1) every process asks from root for next set of parameters
!		(-): extra unnecessary communications
!	2) every process chooses its own set of parameters
!		(-):partitioning the parameters based only on the rank
!-----------------------------------------------------------------------------
!   Large Input Data
!	1) Every process reads a portion of a single file.
!	2) Every process reads its individual file.
!	3) Root process reads and sends to others.
!	4) Combining 1,2 and 3.  
!-----------------------------------------------------------------------------
!	Result Data
!	1) Every process writes the result to its individual file.
!		(-): wf class 'execute' method has to support this
!	2) Every process sends the result data back to root. Root saves them.
!		(-): wf class 'execute' method has to support this
!	3) 
!-----------------------------------------------------------------------------
!	Computation Node information
!	1) Get at run time.
!	2) Read from Input Parameter file/command-line.
!-----------------------------------------------------------------------------

 Contains 
   Function opt_new(fname) Result(this)

     Character(len=*),Intent(in):: fname
     Type(options)              :: this

     this%READ_INFO_FROM_FILE = .True.
     this%fname = fname
     Call opt_read_options(this)

   End Function opt_new

   Subroutine  opt_read_options(this) 

     Type(options) , Intent(inout) :: this
     integer :: opt,ival,iostat,i
     Character (len=MAX_LINE_LEN) :: optname,valstr ,line
     Character (len=STR_MAX_LEN) :: str

    ! READ  line from file
     Open ( UNIT = 38,FILE = this%fname)

     iostat = 0
     Do While ( iostat == 0 ) 
        Read(38,"(A100)", IOSTAT = iostat) line
        If ( iostat /= 0 ) Exit
        Call opt_parse_str( this,line,optname,valstr)
        Select Case(optname)
            Case ("SCHED_DIR")
               opt = STR_OPT_SCHED_DIR
               i=index(valstr," ")-1
               str= valstr(1:i)//char(0)
               Call opt_set_option_str(this,opt,str)
            Case ("CHOL_TASKS_FILE")
               opt = STR_OPT_CHOL_TASKS_FILE
               Read ( valstr,"(A100)") str
               Call opt_set_option_str(this,opt,str)
            Case ("READ_RESOURCE_FROM_FILE")
               opt = OPT_READ_RESOURCE_FROM_FILE
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHUNK_PTS")
               opt = OPT_CHUNK_PTS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("BLOCK_CNT")
               opt = OPT_BLOCK_CNT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("BLOCK_CNT2")
               opt = OPT_BLOCK_CNT2
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("TLIB_THREAD_COUNT")
               opt = OPT_TLIB_THREAD_COUNT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("DATA_READY_LAG")
               opt = OPT_DATA_READY_LAG
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("NODE_AUTONOMY")
               opt = OPT_NODE_AUTONOMY
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("TEST_CORRECTNESS")
               opt =OPT_TEST_CORRECTNESS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("SAVE_RESULTS")
               opt =OPT_SAVE_RESULTS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("BROADCAST_MTD")
               opt =OPT_BROADCAST_MTD
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("PURE_MPI")
               opt =OPT_PURE_MPI
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("TASK_DISTRIBUTION")
               opt =OPT_TASK_DISTRIBUTION
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("IMPORT_TASKS")
               opt =OPT_IMPORT_TASKS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("SEQUENTIAL")
               opt =OPT_SEQUENTIAL
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("SEQ_CHUNK")
               opt =OPT_SEQ_CHUNK
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("LOAD_BALANCE")
               opt =OPT_LOAD_BALANCE
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("GROUP_SIZE")
               opt =OPT_GROUP_SIZE
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("NODE_CAPACITY")
               opt =OPT_NODE_CAPACITY
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHOLESKY")
               opt =OPT_CHOLESKY
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("PART_CNT")
               opt =OPT_PART_CNT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHOL_CACHEOPT")
               opt =OPT_CHOL_CACHEOPT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHOL_GRP_ROWS")
               opt =OPT_CHOL_GRP_ROWS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHOL_GRP_COLS")
               opt =OPT_CHOL_GRP_COLS
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("CHOL_PART_CNT")
               opt =OPT_CHOL_PART_CNT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("DIST_BLK_CNT")
               opt =OPT_DIST_BLK_CNT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            Case ("TIME_OUT")
               opt =OPT_TIME_OUT
               Read ( valstr,"(I5)") ival
               Call opt_set_option(this,opt,ival)
            End Select
     End Do 

   End Subroutine opt_read_options
   
   function opt_get_option(this,opt) result(val)

     Type(options) , Intent(in ) :: this
     Integer       , Intent(in)     :: opt
     Integer                        :: val

     val = -1
     If ( opt > Ubound(this%opt_list,1) .Or. opt < Lbound(this%opt_list,1) ) Return
     val = this%opt_list(opt)

   end function opt_get_option
   function opt_get_option_str(this,opt) result(val)

     Type(options) , Intent(in ) :: this
     Integer       , Intent(in)     :: opt
     Character(len=STR_MAX_LEN)     :: val

     val = ""
     If ( opt > Ubound(this%str_opt_list,1) .Or. opt < Lbound(this%str_opt_list,1) ) Return
     val = this%str_opt_list(opt)

   end function opt_get_option_str


   Subroutine  opt_set_option(this,opt,val)

     Type(options) , Intent(inout ) :: this
     Integer       , Intent(in)     :: opt
     Integer       , Intent(in)     :: val

     If ( opt > Ubound(this%opt_list,1) .Or. opt < Lbound(this%opt_list,1) ) Return
     this%opt_list(opt) = val 

   End Subroutine  opt_set_option

   subroutine  opt_set_option_str(this,opt,val)

     Type(options) , Intent(inout ) :: this
     Integer       , Intent(in)     :: opt
     character(len=STR_MAX_LEN)       , Intent(in)     :: val

     If ( opt > Ubound(this%str_opt_list,1) .Or. opt < Lbound(this%str_opt_list,1) ) Return
     this%str_opt_list(opt) = val 

   end subroutine  opt_set_option_str

   Subroutine opt_parse_str(this,line,opt,val)

     Character(len=MAX_LINE_LEN) , Intent(inout)  :: opt,val
     Character(len=*)            , Intent(inout)  :: line
     Type(options)               , Intent(inout)  :: this
     Integer                                      :: idx

     opt = "" ; val = ""
     if (line(1:6) == "export" ) Then
         line(:) = line(8:) 
     end if
     idx = Index (line,COMMENT_TOKEN)
     If (idx > 0 .And. idx < Len(line) ) Then 
        line(idx:Len(line)) = ""
     End If
     idx = Index(line,ASSIGN_TOKEN)
     If ( idx > 0 .And. idx < Len(line) ) Then 
        opt = line(1:idx-Len(ASSIGN_TOKEN))
        Call to_upper(opt)        
        val = line(idx+Len(ASSIGN_TOKEN):)
     End If

   End Subroutine opt_parse_str
        
   Subroutine to_upper(str)

     Character(*), Intent(in out) :: str
     Integer :: i
 
     Do i = 1, Len(str)
       Select Case(str(i:i))
         Case("a":"z")
           str(i:i) = Achar(Iachar(str(i:i))-32)
       End Select
     End Do 

   End Subroutine to_upper

   Function replace(str,substr,withstr) Result (rep)
     Character(*), Intent(in ) :: str,substr,withstr
     Character(len=100) :: rep
     Integer :: idx
     rep(:) = str(:)
     idx = Index(rep,substr)
     If ( idx > 0 .And. idx < Len(str) ) Then 
        rep(idx:Len(substr)) = withstr(1:Len(withstr))
     End If
     
   End Function replace

	
End Module options_class
