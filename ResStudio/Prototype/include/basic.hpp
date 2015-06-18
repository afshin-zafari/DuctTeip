#ifndef __BASIC_HPP__
#define __BASIC_HPP__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <sched.h>
#include <errno.h>


#define registerAccess register_access
#define getAccess get_access
#define getHandle  get_handle
#define ThreadManager SuperGlue
#define getTaskCount get_task_count

#define MPI_WALL_TIME 1

#ifdef MPI_WALL_TIME
#include "mpi.h"
#else
#include "platform/gettime.hpp"
#endif


#define DLB_DEBUG 0
#define DLB_BUSY_TASKS 5
#define DLB_MODE 0

#define DEBUG 0

#define TERMINATE_TREE  1

#define DUMP_FLAG DEBUG
#define EXPORT_FLAG 0

#define INSERT_TASK_FLAG  DEBUG
#define SKIP_TASK_FLAG    DEBUG
#define DATA_UPGRADE_FLAG DEBUG
#define KERNEL_FLAG       0
#define TERMINATE_FLAG    0
#define IRECV             0
#define OVERSUBSCRIBED    0
#define POSTPRINT         0

#define TRACE_LOCATION printf("%s , %d\n",__FILE__,__LINE__);
//#define  TRACE_LOCATION
//#define TRACE_ALLOCATION(a) printf("alloc mem %s,%d, %ld\n",__FILE__,__LINE__,a)
#define TRACE_ALLOCATION(a) 
//#define TRACE(a) TRACE_LOCATION;printf(a);
#define TRACE(a)


typedef unsigned char byte;
using namespace std;

struct Buffer{
  byte *address;
  int size;
};
struct MessageBuffer{
  int size;
  Buffer header,content;
  byte *address;
  MessageBuffer(int header_size,int content_size);
  ~MessageBuffer();
  void setAddress(byte *new_address);
};



#define TIME_SCALE_TO_MICRO_SECONDS 1000
#define MICRO_SECONDS 1000000
#define MILI_SECONDS 1000
#define TIME_SCALE_TO_MILI_SECONDS 1000000
#define TIME_SCALE_TO_SECONDS 1000000000



typedef unsigned long ClockTimeUnit;
#ifdef MPI_WALL_TIME
typedef unsigned long TimeUnit;
static inline TimeUnit getTime(){
  return (TimeUnit)(MPI_Wtime()*1000000000);  
}
#else
typedef Time::TimeUnit  TimeUnit;
static inline TimeUnit getTime(){
  return Time::getTime();
}
#endif

static inline ClockTimeUnit getClockTime(int unit) {
  timeval tv;
  gettimeofday(&tv, 0);
  if ( unit == MILI_SECONDS)
    return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec/unit);
  return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec);
}

template<class T> void copy(byte * b,int &o,T a);
template<class T> void paste(byte * b,int &o,T *a);


enum verbs_ {
  V_RCVD,
  V_SEND,
  V_TERMINATED,
  V_ADDED,
  V_CHECK_DEP,
  V_DUMP,
  V_CHECK_RDY,
  V_REMOVE
} ;

enum objects_{
  O_TASK,
  O_DATA,
  O_LSNR,
  O_PROG
};



#define PRINT_IF(a) if(a) printf

# if EXPORT_FLAG != 0
#  define export(a,b) export_( a,b,__FILE__,__LINE__)
#  define export_end(a) export_end_(a,__FILE__,__LINE__)
#  define export_prefix printf("\nfile:%-12.12s,line:%4.4d,time:%ld,thread-id:%ld",file.c_str(),line,(long)getTime(),pthread_self())

    void export_(int verb, int object,string file,int line)
    {
      export_prefix;
      printf(" ,verb:%s,obj:%s,",verb_str(verb).c_str(),obj_str(object).c_str());
    }
    void export_end_(int verb,string file,int  line)
    {
      export_prefix;
      printf(",/verb:%s,\n",verb_str(verb).c_str());      
    }
#  define export_int(a)    printf ( ",%s:%d," ,#a,a)
#  define export_long(a)   printf ( ",%s:%ld,",#a,a)
#  define export_time(a)   printf ( ",%s:%Ld,",#a,a)
#  define export_str(a)    printf ( ",%s:%s," ,#a,a)
#  define export_info(f,v) printf ( f,v)
# else // undefine all export macros 
#  define export(a,b) 
#  define export_end(a) 
#  define export_prefix 
#  define export_int(a)    
#  define export_long(a)   
#  define export_time(a) 
#  define export_str(a)    
#  define export_info(f,v) 
# endif // EXPORT_FLAG

#endif //__BASIC_HPP__
