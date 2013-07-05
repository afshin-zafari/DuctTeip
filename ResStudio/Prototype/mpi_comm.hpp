#ifndef __MPI_COMM_HPP__
#define __MPI_COMM_HPP__

#include "basic.hpp"
#include "mpi.h"
#include "network.hpp"

extern int me;
unsigned long last_comm_handle;// ToDo
struct CommRequest{
public:
  MPI_Request *request;
  int tag;
  unsigned long handle;
  CommRequest():request(NULL),tag(-1),handle(-1){}
  CommRequest(MPI_Request *mr,int t,unsigned long h):request(mr),tag(t),handle(h){}
  ~CommRequest(){delete request;}
};
class MPIComm : public  INetwork
{
private:
  list <CommRequest*> request_list;
  int rank;
public:
  MPIComm(){
    int stat = MPI_Init(NULL,NULL );
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    printf("result of MPI_Init:%d, host=%d\n",stat,rank);
    last_comm_handle =0 ;
  }
  ~MPIComm(){
    int stat = MPI_Finalize();
    printf("result of MPI_Finalize:%d, host=%d\n",stat,rank);
  }
  bool canTerminate(){
    bool allSendFinished = (request_list.size() == 0 );
    return allSendFinished;
  }
  
  unsigned long send ( byte *buffer, int length, int tag, int dest){
    printf ("call MPI_Isend: length: %d, source: %d ,dest:%d, tag: %d\n",length,me,dest,tag);
    MPI_Request *mpi_request = new MPI_Request;
    int result = MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,last_comm_handle++);
    request_list.push_back(comm_request);
    printf ("result of MPI_Isend: %d\n",result);
    return (last_comm_handle -1) ; 
  }
  int receive ( byte *buffer, int length, int tag, int source){
    MPI_Status status;
    printf ("call MPI_Recv: length: %d, source:%d, tag: %d\n",length,source,tag);
    int result = MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
    printf ("result of MPI_Recv: %d\n",result);
  }
  int isAnySentCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    int flag=0;
    for (it = request_list.begin(); it != request_list.end() ; ++it) {
      MPI_Test((*it)->request,&flag,MPI_STATUS_IGNORE);
      if ( flag ) {
	*tag = (*it)->tag;
	*handle = (*it)->handle;
	request_list.erase(it); 
	return flag;
      }
    }
  }
  int probe(int tag,int *source,int *length,bool wait){
    int flag;
    MPI_Status status;
    flag = wait;
    printf("host %d probes for: tag:%d\n",me,tag);
    if (wait ) 
      MPI_Probe (MPI_ANY_SOURCE,tag,MPI_COMM_WORLD,      &status);
    else
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
