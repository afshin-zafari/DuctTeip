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
  CommRequest(MPI_Request *mr,int t,unsigned long h,unsigned long len=0):
    request(mr),tag(t),handle(h),length(len)
  {
    start_time = getClockTime(MICRO_SECONDS);
  }
  ~CommRequest(){
    int r=0;
    int flag=false;
    if(0)
    if (*request != MPI_REQUEST_NULL) {
      MPI_Request_get_status(*request,&flag,MPI_STATUS_IGNORE);
      if (flag){
	r = MPI_Request_free(request);
	printf("req deleted. hnd:%ld ret:%d\n",handle,r);
      }
    }
    delete request;
  }
};
/*============================================================================*/
/*-----------------------------------------------------------------------------*/
class MPIComm : public  INetwork
{
private:
  unsigned long last_comm_handle; 
  list <CommRequest*> request_list;
  int rank;
  bool  thread_enabled;
  unsigned long tot_sent_len;
  TimeUnit tot_sent_time;
public:
  /*--------------------------------------------------------------------------*/
  MPIComm(){}
  void initialize(){
    int thread_level,request=MPI_THREAD_SINGLE;
    int err = MPI_Init_thread(NULL,NULL,request,&thread_level);
    //int err = MPI_Init(NULL,NULL);
    thread_enabled = (request == thread_level) ;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    printf("result of MPI_Init:%d, thread-support:%d,requested:%d, thrd-replied:%d\n",
	   err,thread_enabled,MPI_THREAD_SINGLE,thread_level);

    last_comm_handle = 0 ;

    tot_sent_time = 0L;
    tot_sent_len = 0;
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
  unsigned long send ( byte *buffer, int length, int tag, int dest,bool wait=false){
    MPI_Request *mpi_request = new MPI_Request;
    int result ;
    if (wait){
      result= MPI_Send(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD);
      printf("+++++++++++++++++++++++++++\n");
    }
    else{
      result= MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,mpi_request);
      CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_comm_handle,length);
      request_list.push_back(comm_request);
      PRINT_IF(0)("req list size in send function:%ld,tag:%d\n",request_list.size(),tag);
    }
    return last_comm_handle  ; 
  }
  /*--------------------------------------------------------------------------*/
  int receive ( byte *buffer, int length, int tag, int source,bool wait=true){
    MPI_Status status;
    MPI_Request last_receive;
    int result;
    addLogEventStart("MPIReceive",DuctteipLog::MPIReceive);
    if ( wait ) 
      result = MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
    else
      result = MPI_Irecv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&last_receive);
    addLogEventEnd  ("MPIReceive",DuctteipLog::MPIReceive);

    return result;
  }
  /*--------------------------------------------------------------------------*/
  bool isAnySendCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    MPI_Status st;
    int flag=0;
    addLogEventStart("MPI_Test",DuctteipLog::MPITestSent);
    for (it = request_list.begin(); it != request_list.end() ; ++it) {
      MPI_Test((*it)->request,&flag,&st);//MPI_STATUS_IGNORE);
      if ( flag ) {
	*tag = (*it)->tag;
	*handle = (*it)->handle;
	if ((*it)->tag ==2 ) {
	  ClockTimeUnit t = getClockTime(MICRO_SECONDS) - (*it)->start_time;
	  tot_sent_time +=t;
	  tot_sent_len +=(*it)->length;
	  //printf("tot bytes sent:%8ld,tot time:%8ld,last-bytes:%6ld,last-time:%6ld , ",
	 //	 tot_sent_len,tot_sent_time ,(*it)->length,t);
	  //printf("avg BW:%3.1lf Gb/s\n",tot_sent_len/(double)(tot_sent_time)*3 );
	}
	PRINT_IF(0)("sent is complete, tag:%d, handle:%ld\n",*tag,*handle);
	addLogEventEnd("MPI_Test",DuctteipLog::MPITestSent);
	delete (*it);
	request_list.erase(it); 
	return flag;
      }
    }
    addLogEventEnd("MPI_Test",DuctteipLog::MPITestSent);
    return (flag!=0);
  }
  /*--------------------------------------------------------------------------*/
  int probe(int *tag,int *source,int *length,bool wait){
    int exists=0;
    MPI_Status status;
    
    addLogEventStart ( "MPI_Probe", DuctteipLog::MPIProbed);
    if (wait ) 
      MPI_Probe (MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,      &status);
    else
      MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&exists,&status);
    addLogEventEnd ("MPI_Probe", DuctteipLog::MPIProbed);
    PRINT_IF(IRECV)("after probe exists:%d\n",exists);
    if ( exists ) {
      *source = status.MPI_SOURCE;
      *tag = status.MPI_TAG;
      MPI_Get_count(&status,MPI_BYTE,length);
      return true;
    }    
    return false;
  }
  /*--------------------------------------------------------------------------*/
  int probeTags(int *tag,int n,int *source,int *length,int *outtag){

    int exists=0;
    MPI_Status status;
    for ( int i=0;i < n ; i++){
      MPI_Iprobe(MPI_ANY_SOURCE,tag[i],MPI_COMM_WORLD,&exists,&status);
      if ( exists ) {
	PRINT_IF(0)(" probe tag[%d]:%d exists:%d\n",i,tag[i],exists);
	*source = status.MPI_SOURCE;
	*outtag = status.MPI_TAG;
	MPI_Get_count(&status,MPI_BYTE,length);
	return true;
      }    
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  int initialize(int argc,char **argv){return MPI_Init(&argc,&argv);}//ToDo: argv,argc  
  /*--------------------------------------------------------------------------*/
  int finish(){    
    list<CommRequest*>::iterator it;
    int r;
    bool dbg=true;
    //    MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN);
    MPI_Barrier(MPI_COMM_WORLD);
    printf("mpi_err-req:%d, req_count:%ld\n",MPI_ERR_REQUEST,request_list.size());
    if(1){
      for (it = request_list.begin(); it != request_list.end(); it ++){
	printf("req :%d\n",*((*it)->request));
	if ( *((*it)->request) != MPI_REQUEST_NULL ) {
	  r = MPI_Request_free((*it)->request);
	  if(dbg)printf ("req-list:%d,",r);
	}
      }
    }
    int res = MPI_Finalize();
    if(dbg)printf ("finalize return value :%d\n",r);

    return res;  
  } 
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
