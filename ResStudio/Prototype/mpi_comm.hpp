#ifndef __MPI_COMM_HPP__
#define __MPI_COMM_HPP__

#include "basic.hpp"
#include "mpi.h"
#include "network.hpp"

extern int me;
unsigned long last_comm_handle;// ToDo
/*============================================================================*/
struct CommRequest{
public:
  MPI_Request *request;
  int tag;
  unsigned long handle;
  CommRequest():request(NULL),tag(-1),handle(-1){}
  CommRequest(MPI_Request *mr,int t,unsigned long h):request(mr),tag(t),handle(h){}
  ~CommRequest(){delete request;}
};
/*============================================================================*/
class MPIComm : public  INetwork
{
private:
  list <CommRequest*> request_list;
  int rank;
  bool  thread_enabled;
public:
  /*--------------------------------------------------------------------------*/
  MPIComm(){
    int thread_level,request=MPI_THREAD_FUNNELED;
    int err = MPI_Init_thread(NULL,NULL,request,&thread_level);
    thread_enabled = (request == thread_level) ;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    printf("result of MPI_Init:%d, host=%d, thread-support:%d,requested:%d, err:%d thrd-enabled:%d\n",
	   err,rank,thread_level,MPI_THREAD_FUNNELED,err,thread_enabled);
    last_comm_handle =0 ;
  }
  /*--------------------------------------------------------------------------*/
  ~MPIComm(){
    int stat = MPI_Finalize();
    printf("result of MPI_Finalize:%d, host=%d\n",stat,rank);
  }
  
  /*--------------------------------------------------------------------------*/
  bool canTerminate(){
    bool allSendFinished = (request_list.size() == 0 );
    PRINT_IF(0)("req list size:%ld\n",request_list.size());
    return allSendFinished;
  }
  
  /*--------------------------------------------------------------------------*/
  unsigned long send ( byte *buffer, int length, int tag, int dest){
    MPI_Request *mpi_request = new MPI_Request;
    int result = MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_comm_handle);
    request_list.push_back(comm_request);
    return last_comm_handle  ; 
  }
  /*--------------------------------------------------------------------------*/
  int receive ( byte *buffer, int length, int tag, int source){
    MPI_Status status;
    int result = MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
  }
  /*--------------------------------------------------------------------------*/
  bool isAnySendCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    MPI_Status st;
    int flag=0;
    for (it = request_list.begin(); it != request_list.end() ; ++it) {
      MPI_Test((*it)->request,&flag,&st);//MPI_STATUS_IGNORE);
      if ( flag ) {
	*tag = (*it)->tag;
	*handle = (*it)->handle;
	request_list.erase(it); 
	return flag;
      }
    }
    return (flag!=0);
  }
  /*--------------------------------------------------------------------------*/
  int probe(int tag,int *source,int *length,bool wait){
    int flag;
    MPI_Status status;
    flag = wait;
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
  /*--------------------------------------------------------------------------*/
  int initialize(int argc,char **argv){MPI_Init(&argc,&argv);}//ToDo: argv,argc  
  /*--------------------------------------------------------------------------*/
  int finish(){    MPI_Finalize();  } 
  /*--------------------------------------------------------------------------*/
  int get_host_id(){
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    return my_rank;
  }
  /*--------------------------------------------------------------------------*/
  int get_host_count(){
    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    return comm_size;
  }
  /*--------------------------------------------------------------------------*/
  bool canMultiThread() {    return thread_enabled;  }
  /*--------------------------------------------------------------------------*/
  void barrier(){    MPI_Barrier(MPI_COMM_WORLD);  }
};
#endif //__MPI_COMM_HPP__
