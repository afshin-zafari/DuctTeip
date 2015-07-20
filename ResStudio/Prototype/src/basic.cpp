#include "basic.hpp"

  MessageBuffer::MessageBuffer(int header_size,int content_size){
            size = content_size + header_size;
     header.size =  header_size ;
    content.size = content_size ;

            address = new byte[size];
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

TimeUnit StartTime;
void SetStartTime(){StartTime = getTime();}
TimeUnit UserTime(){  return (getTime() - StartTime)/1000000;}


void nanoSleep(int n=1){
  //return;
  timespec ts;
  ts.tv_sec=0;
  ts.tv_nsec=n*1000;

  clock_nanosleep(CLOCK_PROCESS_CPUTIME_ID,0,&ts,NULL);
}

