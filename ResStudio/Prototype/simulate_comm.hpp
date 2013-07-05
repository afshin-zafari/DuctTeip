#ifndef __SIMULATE_COMM_HPP__
#define __SIMULATE_COMM_HPP__

#include "basic.hpp"
#include "network.hpp"

class SimulateComm : public  INetwork
{
private:
  byte *local_buffer;
  int buffer_length;
  
public:
  SimulateComm(){
    
  }
  ~SimulateComm(){
  }
  unsigned long send ( byte *buffer, int length, int tag, int dest){
  }
  int receive ( byte *buffer, int length, int tag, int source){
  }
  int probe(int tag,int *source,int *length){
    int flag;
    return flag;
  }
  int initialize(int argc,char **argv){
  }
  int finish(){
  } 
  int get_host_id(){
    int my_rank;
    return my_rank;
  }
  int get_host_count(){
    int comm_size;
    return comm_size;
  }
  
};
#endif //__SIMULATE_COMM_HPP__
