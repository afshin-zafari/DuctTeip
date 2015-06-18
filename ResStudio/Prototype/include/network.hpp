#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__
#include "basic.hpp"

class INetwork 
{
public:
  virtual unsigned long  send(byte *buffer,int length, int tag,int destination,bool wait =false)=0;
  virtual int receive(byte *buffer,int length, int tag,int source,bool wait=true)=0;
  virtual bool isAnySendCompleted(int *,unsigned long  *)=0;
  virtual bool isAnyPostCompleted(int *,unsigned long  *)=0;
  virtual bool isLastReceiveCompleted(bool *)=0;
  virtual int probe(int *tag,int *,int *,bool)=0;
  virtual int probeTags(int *tag,int ,int *,int *,int *)=0;
  virtual int initialize(int , char**)=0;
  virtual int finish()=0;
  virtual int get_host_id()=0;
  virtual int get_host_count()=0;
  virtual bool canMultiThread() = 0;
  virtual bool canTerminate() = 0;
  virtual void barrier()=0;
  virtual void initialize()=0;
  virtual void getEventBlocking(int *,unsigned long  *)=0;
  virtual unsigned long  postDataReceive(byte *,int,int,int,unsigned long)=0;
  virtual unsigned long  postListenerReceive(byte *,int,int,int)=0;
  virtual unsigned long  postTerminateReceive(byte *,int,int,int)=0;

};

#endif //__NETWORK_HPP__
