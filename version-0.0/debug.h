
#define data_find_by_name(a,b)   data_find_by_name_dbg(a,b,__FILE__,__LINE__)

#ifdef UPPMAX
#define TIME_STAMP         (getime())
#define SIZEOF_PTR(a)   (8)
#define TRACEX(a,b)
#define TRACEY(a,b) write(*,*)  a," : ",b
#define ARGS(a,b)    a,b
#define ARGS3(a,b,c)  a,b,c
#else
#define TIME_STAMP         (MPI_Wtime())
#define SIZEOF_PTR(a)   sizeof(a)
#define TRACEX(a,b)  write(*,*)  a," : ",b
#define TRACEY(a,b) TRACEX(a,b)
#define ARGS(a,b)    (a,b)
#define ARGS3(a,b,c)  (a,b,c)
#endif
#if TIMING_TRACE > 0
#define TIMING(a,b,c,d)   write(*,*) event_name(a),',',TIME_STAMP,',',a,',',b,',',c,',',d
#else
#define TIMING(a,b,c,d)   
#endif

#if VIZ_LEVEL >= 0
#define VIZIT4(a,b,c,d)   call instrument(a,b,c,d)
                          
#define VIZIT5(a,b,c,d)   write(*,"(A4 I15.15 A1 I2 A1 I5.5 A1 I5.5 A1 I5.5 A1 I5.5)") "VIZ ",TIME_STAMP," ",this%proc_id," ",a," ",b," ",c," ",d
#define VIZIT3(a,b,c)     VIZIT4(a,b,c,0)
#define VIZIT(a,b)        VIZIT4(a,b,0,0)
#else
#define VIZIT(a,b)   
#define VIZIT3(a,b,c)   
#define VIZIT4(a,b,c,d)   
#endif

#if DEBUG_LEVEL > 0
#define DEBUG_PRINT(a,b)  write(*,*)  a," : ",b
#define SEP0             ;
#define TEST_ONLY(a)     a
#else
#define DEBUG_PRINT(a,b)  
#define SEP0           
#define TEST_ONLY(a)      
#endif

#if DEBUG_LEVEL >= 1                
#define TRACE1(a,b)     DEBUG_PRINT(a,b)
#define TEST_ONLY1(a)     a
#define SEP1         ;
#else
#define TRACE1(a,b)
#define TEST_ONLY1(a)     
#define SEP1           
#endif

#if DEBUG_LEVEL >= 2                
#define TRACE2(a,b)     DEBUG_PRINT(a,b)
#define TEST_ONLY2(a)     a
#define SEP2         ;
#else
#define TRACE2(a,b)
#define TEST_ONLY2(a)     
#define SEP2           
#endif

#if DEBUG_LEVEL >= 3                
#define TRACE3(a,b)     DEBUG_PRINT(a,b)
#define TEST_ONLY3(a)     a
#define SEP3         ;
#else
#define TRACE3(a,b)
#define TEST_ONLY3(a)     
#define SEP3           
#endif

#if DEBUG_LEVEL >= 4                
#define TRACE4(a,b)     DEBUG_PRINT(a,b)
#define TEST_ONLY4(a)     a
#define SEP4         ;
#else
#define TRACE4(a,b)
#define TEST_ONLY4(a)     
#define SEP4           
#endif

#if DEBUG_LEVEL >= 5                 
#define TRACE5(a,b)     DEBUG_PRINT(a,b)
#define TEST_ONLY5(a)     a
#define SEP5            ;
#else
#define TRACE5(a,b)
#define TEST_ONLY5(a)     
#define SEP5
#endif
