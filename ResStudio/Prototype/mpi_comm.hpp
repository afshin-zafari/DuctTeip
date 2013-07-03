#ifndef __MPI_COMM_HPP__
#define __MPI_COMM_HPP__

#include "basic.hpp"
#include "mpi.h"
#include "network.hpp"

class MPIComm : public  INetwork
{
private:
  MPI_Request req[2];
public:
  MPIComm(){
    MPI_Init(NULL,NULL );
    
  }
  ~MPIComm(){
    MPI_Finalize();
  }
  int send ( byte *buffer, int length, int tag, int dest){
    MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,req);
  }
  int receive ( byte *buffer, int length, int tag, int source){
    MPI_Status status;
    MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
  }
  int probe(int tag,int *source,int *length){
    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE,tag,MPI_COMM_WORLD,&flag,&status);
    if ( flag ) {
      *source = status.MPI_SOURCE;
      MPI_Get_count(&status,MPI_BYTE,length);
    }    
    return flag;
  }
  int initialize(int argc,char **argv){
    MPI_Init(&argc,&argv);//ToDo: argv,argc
  }
  int finish(){
    MPI_Finalize();
  } 
  int get_host_id(){
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    return my_rank;
  }
  int get_host_count(){
    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    return comm_size;
  }
  
};
#endif //__MPI_COMM_HPP__
