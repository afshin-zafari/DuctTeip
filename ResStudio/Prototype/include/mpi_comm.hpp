#ifndef __MPI_COMM_HPP__
#define __MPI_COMM_HPP__

#include "basic.hpp"
#include "mpi.h"
#include "network.hpp"
#include "dt_log.hpp"

extern int me;

/*============================================================================*/
struct CommRequest{
public:
  MPI_Request *request;
  int tag;
  unsigned long handle,length;
  ClockTimeUnit start_time;
  CommRequest():request(NULL),tag(-1),handle(-1){}
  CommRequest(MPI_Request *mr,int t,unsigned long h,unsigned long len=0);
  ~CommRequest(){delete request;}
};
/*============================================================================*/
typedef struct {
  MPI_Request *list;
  int count,postdata_index,send_index,postlsnr_index,postterm_index;
  unsigned long *handles;
  int *tags;
}RequestList;
/*-----------------------------------------------------------------------------*/
class MPIComm : public  INetwork
{
private:
  unsigned long last_comm_handle,last_postdata,last_postlsnr,last_postterm;// ToDo
  list <CommRequest*> request_list,postdata_list,postlsnr_list,postterm_list;
  int rank;
  bool  thread_enabled;
  unsigned long tot_sent_len;
  TimeUnit tot_sent_time;
  MPI_Request  last_receive;
public:
  /*--------------------------------------------------------------------------*/
  MPIComm(){}
  void initialize();
  ~MPIComm();
  bool canTerminate();
  unsigned long send ( byte *buffer, int length, int tag, int dest,bool wait=false);
  unsigned long  postDataReceive(byte *buffer, int length , int tag, int source,unsigned long key);
  unsigned long  postListenerReceive(byte *buffer, int length ,int tag, int source);
  unsigned long  postTerminateReceive(byte *buffer, int length ,int tag, int source);
  bool isAnyDataPostCompleted(int *tag,unsigned long *handle);
  bool isAnyListenerPostCompleted(int *tag,unsigned long *handle);
  bool isAnyTerminatePostCompleted(int *tag,unsigned long *handle);
  bool isAnyPostCompleted(int *tag,unsigned long *handle);
  int receive ( byte *buffer, int length, int tag, int source,bool wait=true);
  bool isLastReceiveCompleted(bool *is_any);
  bool isAnySendCompleted(int *tag,unsigned long *handle);
  int probe(int *tag,int *source,int *length,bool wait);
  int probeTags(int *tag,int n,int *source,int *length,int *outtag);
  int initialize(int argc,char **argv);
  int finish();    
  int get_host_id();
  int get_host_count();
  bool canMultiThread() ;    
  void barrier();    
  void getRequestList(RequestList *req_list);

};
#endif //__MPI_COMM_HPP__
