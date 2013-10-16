#ifndef __BASIC_HPP__
#define __BASIC_HPP__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>


#define DUMP_FLAG 0
#define EXPORT_FLAG 0

#define INSERT_TASK_FLAG  0
#define SKIP_TASK_FLAG    0
#define DATA_UPGRADE_FLAG 0
#define KERNEL_FLAG       0
#define TERMINATE_FLAG    0

//#define TRACE_LOCATION printf("%s , %d\n",__FILE__,__LINE__);
#define  TRACE_LOCATION
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

#define TIME_SCALE_TO_MICRO_SECONDS 1
#define TIME_SCALE_TO_MILI_SECONDS 1000
#define TIME_SCALE_TO_SECONDS 1000000

typedef long int TimeUnit;
static inline TimeUnit getTime() {
  timeval tv;
  gettimeofday(&tv, 0);
  return (tv.tv_sec*1000000+tv.tv_usec);
}



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
}

#define PRINT_IF(a) if(a) printf

# if EXPORT_FLAG != 0
#  define export(a,b) export_( a,b,__FILE__,__LINE__)
#  define export_end(a) export_end_(a,__FILE__,__LINE__)
#  define export_prefix printf("\nfile:%-12.12s,line:%4.4d,time:%ld,",file.c_str(),line,(long)getTime())

    void export_(int verb, int object,string file,int line)
    {
      if ( object != O_LSNR) 
	return ;
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
#  define export_str(a)    printf ( ",%s:%s," ,#a,a)
#  define export_info(f,v) printf ( f,v)
# else // undefine all export macros 
#  define export(a,b) 
#  define export_end(a) 
#  define export_prefix 
#  define export_int(a)    
#  define export_long(a)   
#  define export_str(a)    
#  define export_info(f,v) 
# endif // EXPORT_FLAG

#endif //__BASIC_HPP__
