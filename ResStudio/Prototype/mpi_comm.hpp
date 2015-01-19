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
  void initialize(){
    int thread_level,request=MPI_THREAD_SINGLE;
    int err = MPI_Init_thread(NULL,NULL,request,&thread_level);
    thread_enabled = (request == thread_level) ;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    printf("result of MPI_Init:%d, host=%d, thread-support:%d,requested:%d, err:%d thrd-enabled:%d\n",
	   err,rank,thread_level,MPI_THREAD_FUNNELED,err,thread_enabled);

    last_comm_handle = 0 ;
    last_postdata    = 0 ;
    last_postlsnr    = 0 ;
    last_postterm    = 0 ;

    last_receive = MPI_REQUEST_NULL;
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
    if (wait)
      result= MPI_Send(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD);
    else
      result= MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_comm_handle,length);
    request_list.push_back(comm_request);
    PRINT_IF(0)("req list size in send function:%ld,tag:%d\n",request_list.size(),tag);
    if(0)
    if ( tag == 10 || tag ==2){
      double * M= (double*)(buffer+192);
      int size = (length -192)/sizeof(double);
      double sum=0.0;
      for (int i=0;i<size;i++)
	sum+= M[i];
      //printf("@Checksum s :%lf\n",sum);
      /*
      for ( int i=0;i<5;i++)
	for ( int j =0;j<5;j++){
	  sum += M[];
	}
      */
    }
    return last_comm_handle  ; 
  }
  /*--------------------------------------------------------------------------*/
  unsigned long  postDataReceive(byte *buffer, int length , int tag, int source,unsigned long key){

    MPI_Request *mpi_request = new MPI_Request;
    last_postdata++;
    PRINT_IF(POSTPRINT)("postDataRecv, buf:%p,len:%d,source:%d,handle:%ld\n",
	   buffer,length,source,last_postdata);
    int result = MPI_Recv_init(buffer,length,MPI_BYTE,source,tag+key,MPI_COMM_WORLD,mpi_request);
    MPI_Start(mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,last_postdata);
    postdata_list.push_back(comm_request);
    return last_postdata;
  }
/*-------------------------------------------------------------------------------*/
  unsigned long  postListenerReceive(byte *buffer, int length ,int tag, int source){
    MPI_Request *mpi_request = new MPI_Request;
    PRINT_IF(POSTPRINT)("postLsnr, buf:%p,len:%d,source:%d,handle:%ld\n",
	   buffer,length,source,last_postlsnr+1);
    int result = MPI_Recv_init(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,mpi_request);
    MPI_Start(mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_postlsnr);
    postlsnr_list.push_back(comm_request);
    return last_postlsnr;
    
  }
/*-------------------------------------------------------------------------------*/
  unsigned long  postTerminateReceive(byte *buffer, int length ,int tag, int source){


    MPI_Request *mpi_request = new MPI_Request;
    PRINT_IF(POSTPRINT)("postTerm, buf:%p,len:%d,source:%d,handle:%ld\n",
			buffer,length,source,last_postterm+1);
    int result = MPI_Recv_init(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,mpi_request);
    MPI_Start(mpi_request);
    CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_postterm);
    postterm_list.push_back(comm_request);

    return last_postterm;
    
  }
  /*--------------------------------------------------------------------------*/
  bool isAnyDataPostCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    MPI_Status st;
    int flag=0;
    int  DataTag = 2; //todo
    for (it = postdata_list.begin(); it != postdata_list.end() ; ++it) {
      MPI_Test((*it)->request,&flag,&st);
      PRINT_IF(POSTPRINT)("postData checked , handle:%d, status:%d\n",(*it)->handle,st);
      if ( flag ) {
	*tag = DataTag; //(*it)->tag;
	*handle = (*it)->handle;
	PRINT_IF(POSTPRINT)("postData Completed, handle%ld\n",*handle);
	if (*(*it)->request != MPI_REQUEST_NULL) 
	  MPI_Request_free((*it)->request);
	postdata_list.erase(it); 
	return true;
      }
    }
    PRINT_IF(POSTPRINT)("no postData completed yet.\n");
    return false;
  }
  /*--------------------------------------------------------------------------*/
  bool isAnyListenerPostCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    MPI_Status st;
    int flag=0;
    for (it = postlsnr_list.begin(); it != postlsnr_list.end() ; ++it) {
      MPI_Test((*it)->request,&flag,&st);
      PRINT_IF(POSTPRINT)("postLsnr checked , handle:%d, status:%d\n",(*it)->handle,st);
      if ( flag ) {
	*tag = (*it)->tag;
	*handle = (*it)->handle;
	PRINT_IF(POSTPRINT)("postLsnrCompleted, handle:%ld,tag:%d\n",*handle,*tag);
	if (*(*it)->request != MPI_REQUEST_NULL) 
	  MPI_Request_free((*it)->request);
	postlsnr_list.erase(it); 
	return true;
      }
    }
    PRINT_IF(POSTPRINT)("no postLsnr completed yet.\n");
    return false;
    
  }
  /*--------------------------------------------------------------------------*/
  bool isAnyTerminatePostCompleted(int *tag,unsigned long *handle){
    list <CommRequest*>::iterator it;
    MPI_Status st;
    int flag=0;
    PRINT_IF(POSTPRINT)("postTerm checked ,list size:%ld\n",postterm_list.size());
    it = postterm_list.begin();
    if ( postterm_list.size() == 0) 
      return false;
    do  {
      MPI_Test((*it)->request,&flag,&st);
      PRINT_IF(POSTPRINT)("postTerm checked , handle:%ld, status:%d\n",(*it)->handle,st);
      if ( flag ) {
	*tag = (*it)->tag;
	*handle = (*it)->handle;
	PRINT_IF(POSTPRINT)("postTerm Completed, handle:%ld,tag:%d\n",*handle,*tag);
	if (*(*it)->request != MPI_REQUEST_NULL) 
	  MPI_Request_free((*it)->request);
	postterm_list.erase(it); 
	PRINT_IF(POSTPRINT)("postTerm after completion ,list size:%ld\n",postterm_list.size());
	return true;
      }
      it++;
    }while(it != postterm_list.end() );
    PRINT_IF(POSTPRINT)("no postTerm completed yet.\n");
    return false;
    
  }
  /*--------------------------------------------------------------------------*/
  bool isAnyPostCompleted(int *tag,unsigned long *handle){
    if (isAnyDataPostCompleted(tag,handle))
      return true;
    if (isAnyListenerPostCompleted(tag,handle))
      return true;
    if (isAnyTerminatePostCompleted(tag,handle)){
      PRINT_IF(POSTPRINT)("post term return is true.\n");
      return true;
    }
    return false;
  }
  /*--------------------------------------------------------------------------*/
  int receive ( byte *buffer, int length, int tag, int source,bool wait=true){
    MPI_Status status;
    int result;
    addLogEventStart("MPIReceive",DuctteipLog::MPIReceive);
    if ( wait ) 
      result = MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
    else
      result = MPI_Irecv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&last_receive);
    addLogEventEnd  ("MPIReceive",DuctteipLog::MPIReceive);

	if (  tag == 10 || tag == 13) {
	  if (0){
	    double sum = 0.0,*contents=(double *)(buffer+192);
	    long size = (length-192)/sizeof(double);
	    for ( long i=0; i< size; i++)
	      sum += contents[i];
	    printf("+++sum i , ---------,%lf adr:%p\n",sum,contents);
	  }
	}



    return result;
  }
  /*--------------------------------------------------------------------------*/
  bool isLastReceiveCompleted(bool *is_any){
    int flag;
    MPI_Status status;
    *is_any=true;
    
    PRINT_IF(IRECV)("LRC enter\n");
    if ( last_receive != MPI_REQUEST_NULL){
      MPI_Test (&last_receive,&flag,&status);
      *is_any = true;
      PRINT_IF(IRECV)("last receive exists,finsihed:%d,%p,req-null:%p\n",
			   flag,last_receive,MPI_REQUEST_NULL);
    }
    else{
      *is_any = false;
      PRINT_IF(IRECV)("last receive NOT exists.\n");
    }
    PRINT_IF(IRECV)("LRC exit.flag:%d\n",flag);
    return (flag!=0);
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
	if (*(*it)->request != MPI_REQUEST_NULL) 
	  MPI_Request_free((*it)->request);
	addLogEventEnd("MPI_Test",DuctteipLog::MPITestSent);
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
    printf("err-req:%d\n",MPI_ERR_REQUEST);
    for (it = request_list.begin(); it != request_list.end(); it ++){
      r = MPI_Request_free((*it)->request);
      //      if(dbg)printf ("req-list:%d,",r);
    }
    
    for (it = postdata_list.begin(); it != postdata_list.end(); it ++){
      r = MPI_Request_free((*it)->request);
      if(dbg)printf ("pdata:%d,",r);
    }
    for (it = postlsnr_list.begin(); it != postlsnr_list.end(); it ++){
      r=MPI_Request_free((*it)->request);
      if(dbg)printf ("lsnr:%d,",r);
    }
    for (it = postterm_list.begin(); it != postterm_list.end(); it ++){
      r=MPI_Request_free((*it)->request);
      if(dbg)printf ("term:%d,",r);
    }
    if ( last_receive != MPI_REQUEST_NULL ) {
      r = MPI_Request_free(&last_receive);
      if(dbg)printf ("last-rcv:%d,\n",r);
    }
    
    return MPI_Finalize();  
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
/*-------------------------------------------------------------------------------*/
  void getRequestList(RequestList *req_list){
    int post_count = postdata_list.size();
    int send_count = request_list.size();
    int listener_count = postlsnr_list.size();
    int terminate_count = postterm_list.size();
    int total_count = post_count+send_count+listener_count+terminate_count;
    int index=0;
    req_list->count = total_count;
    req_list->list = new MPI_Request[total_count];
    req_list->handles = new unsigned long [total_count];
    req_list->tags = new int [total_count];
    list<CommRequest*>::iterator it;
    for (it = request_list.begin(); it != request_list.end(); it ++){
      req_list->list   [index  ] = *((*it)->request);
      req_list->tags   [index  ] = (*it)->tag;      
      req_list->handles[index++] = (*it)->handle;      
    }
    req_list->send_index = index;
    for (it = postdata_list.begin(); it != postdata_list.end(); it ++){
      req_list->list   [index  ] = *((*it)->request);
      req_list->tags   [index  ] = (*it)->tag;      
      req_list->handles[index++] = (*it)->handle;      
    }
    req_list->postdata_index = index;
    for (it = postlsnr_list.begin(); it != postlsnr_list.end(); it ++){
      req_list->list   [index  ] = *((*it)->request);
      req_list->tags   [index  ] = (*it)->tag;      
      req_list->handles[index++] = (*it)->handle;      
    }
    req_list->postlsnr_index = index;
    for (it = postterm_list.begin(); it != postterm_list.end(); it ++){
      req_list->list   [index  ] = *((*it)->request);
      req_list->tags   [index  ] = (*it)->tag;      
      req_list->handles[index++] = (*it)->handle;      
    }
    req_list->postterm_index = index;
    

  }
/*-------------------------------------------------------------------------------*/
  void getEventBlocking(int *tag,unsigned long *handle){
  
  }

};
#endif //__MPI_COMM_HPP__
