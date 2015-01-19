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
#define KERNEL_FLAG       1
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
  MessageBuffer(int header_size,int content_size){
            size = content_size + header_size;
     header.size =  header_size ;
    content.size = content_size ;

            address = new byte[size];
	    TRACE_ALLOCATION(size);
     header.address = address;
    content.address = address + header_size;

  }
  ~MessageBuffer(){
    delete[] address;
  }
  void setAddress(byte *new_address){
    delete[] address;
            address = new_address;
     header.address = address;
    content.address = address + header.size;
  }
};



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

void flushBuffer(byte *buffer,int length){
  printf("flush buffer:%p \n",buffer);
  for ( int i=0;i<length ;i++){
    printf ("%2.2X ",buffer[i]);
    if ( (i>=20) && (i % 20) == 0 ) printf("\n");
  }
  printf("\n");
}

#define TIME_SCALE_TO_MICRO_SECONDS 1000
#define MICRO_SECONDS 1000000
#define MILI_SECONDS 1000
#define TIME_SCALE_TO_MILI_SECONDS 1000000
#define TIME_SCALE_TO_SECONDS 1000000000


typedef unsigned long ClockTimeUnit;
static inline ClockTimeUnit getClockTime(int unit) {
  timeval tv;
  gettimeofday(&tv, 0);
  if ( unit == MILI_SECONDS)
    return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec/unit);
  return (tv.tv_sec*(ClockTimeUnit)unit+tv.tv_usec);
}
#ifdef MPI_WALL_TIME
typedef unsigned long TimeUnit;
#pragma message("getTime()=MPI WTIME")
static inline TimeUnit getTime(){
  return (TimeUnit)(MPI_Wtime()*1000000000);  
}
#else
typedef Time::TimeUnit  TimeUnit;
static inline TimeUnit getTime(){
  return Time::getTime();
}
#endif


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

string verb_str(int verb){
  switch(verb){
  case V_SEND:
    return "send";
  case V_RCVD:
    return "received";
  case V_TERMINATED:
    return "terminated";
  case V_ADDED:
    return "added";
  case V_CHECK_DEP:
    return "check_dep";
  case V_DUMP:
    return "dump";
  case V_CHECK_RDY:
    return "check_ready";
  case V_REMOVE:
    return "remove";
  default:
    return "Not Defined";
  }
}

string obj_str(int obj ) {
  switch(obj){
  case O_TASK:
    return "task";
  case O_DATA:
    return "data";
  case O_LSNR:
    return "listener";
  case O_PROG:
    return "program";
    
  }
  return "";
}


void threadInfo(int arg){
  sched_param sp;
  int r;
  if ( arg == 1 ) {
    sp.sched_priority=1;
    r=pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp);
  }
  int schd;
  int r2=pthread_getschedparam (pthread_self(),&schd,&sp);
  //  sched_getparam(0,&sp);
  if(0){
    printf("ret-vals:%d,%d schedule policy:%d, sch-priority:%d\n",r,r2,schd,sp);  
    printf("fifo:%d,other:%d,batch:%d,rr:%d,idle:%d\n",SCHED_FIFO,SCHED_OTHER,SCHED_BATCH,SCHED_RR,SCHED_IDLE);
    printf("esrch:%d,einval:%d,eperm:%d\n",ESRCH,EINVAL,EPERM);
  }
}
void nanoSleep(int n=1){
  //return;
  timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=n*1000;

  clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID,0,&ts,NULL);
}

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
