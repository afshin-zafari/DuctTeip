#include "basic.hpp"

  MessageBuffer::MessageBuffer(int header_size,int content_size){
            size = content_size + header_size;
     header.size =  header_size ;
    content.size = content_size ;

            address = new byte[size];
	    TRACE_ALLOCATION(size);
     header.address = address;
    content.address = address + header_size;

  }
  MessageBuffer::~MessageBuffer(){
    delete[] address;
  }
  void MessageBuffer::setAddress(byte *new_address){
    delete[] address;
            address = new_address;
     header.address = address;
    content.address = address + header.size; 
  }




void flushBuffer(byte *buffer,int length){
  printf("flush buffer:%p \n",buffer);
  for ( int i=0;i<length ;i++){
    printf ("%2.2X ",buffer[i]);
    if ( (i>=20) && (i % 20) == 0 ) printf("\n");
  }
  printf("\n");
}



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

/*
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
    printf("fifo:%d,other:%d,batch:%d,rr:%d,idle:%d\n",SCHED_FIFO,SCHED_OTHER,SCHED_BATCH,SCHED_RR,SCHED_IDLE);
    printf("esrch:%d,einval:%d,eperm:%d\n",ESRCH,EINVAL,EPERM);
  }
}
*/
void nanoSleep(int n=1){
  //return;
  timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=n*1000;

  clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID,0,&ts,NULL);
}

