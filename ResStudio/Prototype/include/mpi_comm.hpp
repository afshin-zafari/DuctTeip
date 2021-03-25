#ifndef __MPI_COMM_HPP__
#define __MPI_COMM_HPP__

#include "basic.hpp"
#if WITH_MPI
#include "mpi.h"
#endif

#include "network.hpp"
#include "dt_log.hpp"
#include "memory_manager.hpp"
#include "mailbox.hpp"
#include "engine.hpp"


extern int me;

/*============================================================================*/
struct CommRequest{
public:
  ClockTimeUnit start_time;
  MPI_Request *request;
  MemoryItem *mem;
  ulong handle,length;
  byte *buf;
  int tag;
  CommRequest():request(NULL),handle(-1),tag(-1){}
  CommRequest(MPI_Request *mr,int t,unsigned long h,unsigned long len=0);
  ~CommRequest(){delete request;}
};
/*============================================================================*/
typedef struct {
  MPI_Request *list;
  int count,send_index;
  unsigned long *handles;
  int *tags;
}RequestList;
/*-----------------------------------------------------------------------------*/
class MPIComm : public  INetwork
{
private:
  list <CommRequest*> request_list;
  ulong           last_comm_handle;
  int             rank,prcv_count;
  bool            thread_enabled;
  ulong           tot_sent_len,tot_send_count;
  TimeUnit        tot_sent_time,recv_time;
  MPI_Request     last_receive,*prcv_reqs;
  vector<CommRequest *> prcv_vect;
  pthread_mutex_t send_mx;
  pthread_cond_t  send_cv;
  FILE *pending_counts;
  int pending_sends;
  enum {PENDING_DATA, PENDING_NON_DATA};
public:
  /*--------------------------------------------------------------------------*/
  MPIComm(){recv_time=0;}
  ~MPIComm();
  ulong send ( byte *buffer, int length, int tag, int dest,bool wait=false);
  void  initialize();
  bool  canTerminate();
  int   receive ( byte *buffer, int length, int tag, int source,bool wait=true);
  bool  isAnySendCompleted(int *tag,unsigned long *handle);
  int   probe(int *tag,int *source,int *length,bool wait);
  int   probeTags(int *tag,int n,int *source,int *length,int *outtag);
  int   initialize(int argc,char **argv);
  int   finish();
  int   get_host_id();
  int   get_host_count();
  bool  canMultiThread() ;
  void  barrier();
  void  getRequestList(RequestList *req_list);
  void  waitForAnySendComplete(int *tag,ulong *handle);
  void  waitForAnyReceive(int *,int *,int *);
  void  waitForSend();
  void  removeRequest(MPI_Request req,ulong handle);
  void  postReceiveData(int,int, void *);
  bool  anyDataReceived(void  *event);
  double getBandwidth();

};
#endif //__MPI_COMM_HPP__
