#ifndef __BASIC_HPP__
#define __BASIC_HPP__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_LOCATION printf("%s , %d\n",__FILE__,__LINE__);



typedef unsigned char byte;
using namespace std;

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

#endif //__BASIC_HPP__
