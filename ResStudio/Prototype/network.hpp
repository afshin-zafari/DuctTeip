#ifndef __NETWORK_HPP__
#define __NETWORK_HPP__


class INetwork 
{
public:
  virtual int  send(byte *buffer,int length, int tag,int destination)=0;
  virtual int receive(byte *buffer,int length, int tag,int source)=0;
  virtual int probe(int tag,int *,int *)=0;
  virtual int initialize(int , char**)=0;
  virtual int finish()=0;
  virtual int get_host_id()=0;
  virtual int get_host_count()=0;
};

#endif //__NETWORK_HPP__
