#ifndef __BASIC_HPP__
#define __BASIC_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <sched.h>
#include <errno.h>
#include <pthread.h>


#define registerAccess register_access
#define getAccess get_access
#define getHandle  get_handle
#define ThreadManager SuperGlue
#define getTaskCount get_task_count


#if WITH_MPI == 1
#include "mpi.h"
#else
#include "sg/platform/gettime.hpp"
#endif


#define PRINT_IF(a) if(a)printf
//#define DLB_DEBUG 0
//#define DLB_MODE 0
#define POST_RECV_DATA 0

#define DEBUG 0


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


typedef unsigned long ulong;
typedef unsigned char byte;


#define LOG_MULTI_THREAD 1
#define LOG_DLB          2
#define LOG_CONFIG       4
#define LOG_DATA         8
#define LOG_COMM         16
#define LOG_PROFILE      32
#define LOG_TASKS        64
#define LOG_LISTENERS    128
#define LOG_DLBX         256
#define LOG_MLEVEL       512
#define LOG_DLB_SMART    1024
#define LOG_TESTS        2048


#define RELEASE 1
#ifndef BUILD
#define BUILD DEBUG
#endif

#define LOG_FLAG  (0xffffffffL) //LOG_MLEVEL+LOG_PROFILE+LOG_CONFIG)

#if BUILD == RELEASE
#define LOG_INFO(a,b,c...)
#else
#define LOG_INFO(f,p,args...) if((LOG_FLAG &(f))||((f)==LOG_TESTS)){	\
    std::string __fname=__FILE__;					\
    std::string::size_type __n= __fname.rfind("/");			\
    fprintf(stderr,"%20s,%4d, %-32s, tid:%9X, %6llu ::",		\
	    __fname.substr(__n).c_str(),				\
	    __LINE__,__FUNCTION__,					\
	    (uint)pthread_self(),UserTime());				\
    fprintf(stderr,p , ##args);						\
  }
#endif



#define LOG_ERROR(p,args...) fprintf(stderr,p , ##args )



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
  double ti=MPI_Wtick()/1000;

  return (TimeUnit)(MPI_Wtime()/ti);
}
#else
typedef Time::TimeUnit  TimeUnit;
static inline TimeUnit getTime(){
  return Time::getTime();
}
#endif

TimeUnit UserTime    ();
void     SetStartTime();
extern TimeUnit StartTime     ;

static inline ClockTimeUnit getClockTime(int unit) {
  timeval tv;
  gettimeofday(&tv, 0);
  if ( unit == MILI_SECONDS)
    return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec/unit);
  return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec);
}

void flushBuffer(byte *,int);

template<class T>
void copy(byte * b,int &o,T a){
  memcpy(b+o,(char *)&a,sizeof(a));
  o +=  sizeof(a);
}
template<class T>
void paste(byte * b,int &o,T *a){
  memcpy((char *)a,b+o,sizeof(T));
  o +=  sizeof(T);
}



#endif //__BASIC_HPP__
