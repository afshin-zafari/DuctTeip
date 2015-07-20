#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__
#include "basic.hpp"


class INetwork 
{
public:
  virtual unsigned long  send(byte *buffer,int length, int tag,int destination,bool wait =false)=0;
  virtual int receive(byte *buffer,int length, int tag,int source,bool wait=true)=0;
  virtual bool isAnySendCompleted(int *,unsigned long  *)=0;
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
  virtual void waitForAnySendComplete(int*,ulong*)=0;
  virtual void waitForAnyReceive(int*,int*,int*)=0;
  virtual bool anyDataReceived(void *)=0;
  virtual void postReceiveData(int,int,void *)=0;
};

#endif //__NETWORK_HPP__
