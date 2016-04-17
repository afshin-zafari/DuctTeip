#include "mpi_comm.hpp"
/*--------------------------------------------------------------------------*/
CommRequest::CommRequest(MPI_Request *mr,int t,unsigned long h,unsigned long len):
  request(mr),handle(h),length(len),tag(t)
{
  start_time = UserTime();
}

/*--------------------------------------------------------------------------*/
void MPIComm::initialize(){
  int thread_level=MPI_THREAD_SINGLE,request=MPI_THREAD_SINGLE;
  //int err = MPI_Init_thread(NULL,NULL,request,&thread_level);
  int err = MPI_Init(NULL,NULL);
  (void)err;
  thread_enabled = (request == thread_level) ;

  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  LOG_INFO(LOG_MULTI_THREAD,"result:%d, host=%d, thread-support:%d,requested:%d,  thrd-enabled:%d\n",
	 err,rank,thread_level,request,thread_enabled);

  last_comm_handle = 0 ;

  last_receive = MPI_REQUEST_NULL;
  tot_sent_time = 0L;
  tot_sent_len = 0;
  pthread_mutexattr_t send_mxattr;
  pthread_mutexattr_init(&send_mxattr);
  pthread_mutexattr_settype(&send_mxattr,PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&send_mx,&send_mxattr);
  pthread_cond_init(&send_cv,NULL);
}
/*--------------------------------------------------------------------------*/
MPIComm::~MPIComm(){

  //  int stat = MPI_Finalize();
  //  LOG_INFO(LOG_COMM,"result:%d, host=%d\n",stat,rank);
}

/*--------------------------------------------------------------------------*/
bool MPIComm::canTerminate(){
  bool allSendFinished = (request_list.size() == 0 );
  return allSendFinished;
}

/*--------------------------------------------------------------------------*/
ulong  MPIComm::send(byte *buffer,int length,int tag,int dest,bool wait){

  MPI_Request *mpi_request = new MPI_Request;
  int result ;
  if (wait)
    result= MPI_Send(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD);
  else
    result= MPI_Isend(buffer,length,MPI_BYTE,dest,tag,MPI_COMM_WORLD,mpi_request);
  CommRequest *comm_request = new CommRequest(mpi_request,tag,++last_comm_handle,length);
  LOG_INFO(LOG_COMM,"msg %d B with tag %d sent to %d\n",length,tag,dest);
  pthread_mutex_lock(&send_mx);
  request_list.push_back(comm_request);
  pthread_cond_signal(&send_cv);
  pthread_mutex_unlock(&send_mx);
  //flushBuffer(buffer,64);
  (void)result;
  return last_comm_handle  ;
}
/*--------------------------------------------------------------------------*/
int MPIComm::receive ( byte *buffer, int length, int tag, int source,bool wait){
  MPI_Status status;
  int        result;

  TimeUnit t = getTime();
  LOG_EVENT(DuctteipLog::MPIReceive);

  if ( wait ) {
    result = MPI_Recv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status);
    int stat_length ;
    MPI_Get_count(&status,MPI_BYTE,&stat_length);
    LOG_INFO(LOG_COMM,"res:%d, src:%d, tag:%d, st.len:%d\n",
	     result,status.MPI_SOURCE,status.MPI_TAG,stat_length);
  }
  else{
    result = MPI_Irecv(buffer,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&last_receive);
    LOG_INFO(LOG_COMM,"res:%d, src:%d, tag:%d, len:%d\n", result,source,tag,length);
  }
  if ( wait ) {
    recv_time += getTime() - t;
  }

  return result;
}
/*--------------------------------------------------------------------------*/
bool MPIComm::isAnySendCompleted(int *tag,unsigned long *handle){
  list <CommRequest*>::iterator it;
  CommRequest* req;
  MPI_Status st;
  int flag=0;
  LOG_EVENT(DuctteipLog::MPITestSent);
  for (it = request_list.begin(); it != request_list.end() ; ++it) {
    req = (*it);
    MPI_Test(req->request,&flag,&st);
    if ( flag ){
      *tag = req->tag;
      *handle = req->handle;
      if (req->tag ==2 ||req->tag == 11) {
	ulong  t = UserTime() - req->start_time;
	tot_sent_time +=t;
	tot_sent_len +=req->length;
	LOG_INFO(LOG_DLB_SMART,"tot-time:%ld, tot-len:%ld\n",tot_sent_time,tot_sent_len);
      }
      request_list.erase(it);
      return flag;
    }
  }
  return (flag!=0);
}
/*--------------------------------------------------------------------------*/
int MPIComm::probe(int *tag,int *source,int *length,bool wait){
  int exists=0;
  MPI_Status status;

  LOG_EVENT(DuctteipLog::MPIProbed);
  if (wait ) {
    MPI_Probe (MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,      &status);
    exists = 1;
  }
  else
    MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&exists,&status);
  if ( exists ) {
    *source = status.MPI_SOURCE;
    *tag = status.MPI_TAG;
    MPI_Get_count(&status,MPI_BYTE,length);
    LOG_INFO(LOG_COMM,"after probe exists:%d, src:%d, tag:%d, len:%d\n",exists,*source,*tag,*length);
    return true;
  }
  return false;
}
/*--------------------------------------------------------------------------*/
int MPIComm::probeTags(int *tag,int n,int *source,int *length,int *outtag){

  int exists=0;
  MPI_Status status;
  for ( int i=0;i < n ; i++){
    MPI_Iprobe(MPI_ANY_SOURCE,tag[i],MPI_COMM_WORLD,&exists,&status);
    if ( exists ) {
      *source = status.MPI_SOURCE;
      *outtag = status.MPI_TAG;
      MPI_Get_count(&status,MPI_BYTE,length);
      return true;
    }
  }
  return false;
}
/*--------------------------------------------------------------------------*/
int MPIComm::initialize(int argc,char **argv){return MPI_Init(&argc,&argv);}//ToDo: argv,argc
/*--------------------------------------------------------------------------*/
int MPIComm::finish(){
  list<CommRequest*>::iterator it;
  //int r,flag;
  //MPI_Status st;

  for (it = request_list.begin(); it != request_list.end(); it ++){
    LOG_INFO(LOG_COMM,"request with tag:%d, handle:%ld cancelled.\n",
	     (*it)->tag,(*it)->handle);
    MPI_Cancel((*it)->request);
  }
  LOG_INFO(LOG_COMM,"mpi finalized.\n");
  LOG_INFO(LOG_MULTI_THREAD,"recv_time:%ld\n", recv_time);
  int flag;
  MPI_Finalized(&flag);
  if ( flag)
    return 0;
  return MPI_Finalize();
  return 1;
}
/*--------------------------------------------------------------------------*/
int MPIComm::get_host_id(){
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  return my_rank;
}
/*--------------------------------------------------------------------------*/
int MPIComm::get_host_count(){
  int comm_size;
  MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
  return comm_size;
}
/*--------------------------------------------------------------------------*/
bool MPIComm::canMultiThread() {    return thread_enabled;  }
/*--------------------------------------------------------------------------*/
void MPIComm::barrier(){    MPI_Barrier(MPI_COMM_WORLD);  }
/*-------------------------------------------------------------------------------*/
void MPIComm::getRequestList(RequestList *req_list){
  list<CommRequest*>::iterator it;

  int index=0;

  int total_count   = request_list.size();
  req_list->count   = total_count;
  req_list->list    = new MPI_Request   [total_count];
  req_list->handles = new unsigned long [total_count];
  req_list->tags    = new int           [total_count];

  for (it = request_list.begin(); it != request_list.end(); it ++){
    req_list->list   [index  ] = *((*it)->request) ;
    req_list->tags   [index  ] =   (*it)->tag      ;
    req_list->handles[index++] =   (*it)->handle   ;
  }

  req_list->send_index = index;
}
/*-------------------------------------------------------------------------------*/
void MPIComm::removeRequest(MPI_Request req,ulong handle){
  list<CommRequest*>::iterator it;

  //int index=0;

  for (it = request_list.begin(); it != request_list.end(); it ++){
    LOG_INFO(LOG_COMM,"handle:%X, exs-handle:%X\n",(uint)handle,(uint)( (*it)->handle));
    if ( req == *((*it)->request) ||
	 handle == (*it)->handle   ){
      LOG_INFO(LOG_COMM,"deleted tag:%d\n",(*it)->tag);
      request_list.erase(it);
      break;
    }
  }

}
/*-------------------------------------------------------------------------------*/
void MPIComm::waitForSend(){
  pthread_mutex_lock(&send_mx);
  pthread_cond_wait(&send_cv,&send_mx);
}
/*-------------------------------------------------------------------------------*/
void MPIComm::waitForAnySendComplete(int *tag,ulong *handle){
  RequestList req_list;
  int 	      index=-1;
  MPI_Status  st;

  LOG_INFO(LOG_COMM,"send count:%ld\n",request_list.size());
  if (request_list.size()==0){
    waitForSend();
    pthread_mutex_unlock(&send_mx);
  }

  getRequestList(&req_list);

  LOG_INFO(LOG_COMM,"send list count:%d\n",req_list.count);


  MPI_Waitany(req_list.count,req_list.list,&index,&st);
  *tag    = req_list.tags   [index];
  *handle = req_list.handles[index];

  removeRequest(req_list.list[index],*handle);

  LOG_INFO(LOG_COMM,"WaitResult:%d, no-req:%d\n",index,MPI_UNDEFINED);
}
/*-------------------------------------------------------------------------------*/
void MPIComm::waitForAnyReceive(int *tag,int *src,int *len){
  probe(tag,src,len,true);
  LOG_INFO(LOG_COMM,"recv t:%d, src:%d, len:%d\n",*tag,*src,*len);
}
/*-------------------------------------------------------------------------------*/
void MPIComm::postReceiveData(int n ,int data_size,void *m){
  const int DATA_TAG = 2;
  MemoryManager *mem_mgr = (MemoryManager *)m;
  LOG_INFO(LOG_MULTI_THREAD,"n:%d, size:%d mem:%p, mngr:%p\n",n,data_size,m,mem_mgr);
  prcv_reqs = new MPI_Request[n];
  prcv_vect.resize(n);
  for(int from =0; from < n; from ++){
    prcv_vect[from]         = new CommRequest();
    LOG_INFO(LOG_MULTI_THREAD,"prcv_vect[%d]:%p\n",from,prcv_vect[from]);
    prcv_vect[from]->mem    = mem_mgr->getNewMemory();
    byte *buf               = prcv_vect[from]->mem->getAddress();
    prcv_vect[from]->buf    = buf;
    LOG_INFO(LOG_MULTI_THREAD,"buf :%p, prcv_vect[%d].buf:%p\n",buf,from,prcv_vect[from]->buf);
    prcv_vect[from]->tag    = DATA_TAG;
    prcv_vect[from]->length = data_size;
    prcv_vect[from]->handle = 0;

    MPI_Recv_init(buf,data_size,MPI_BYTE,from,DATA_TAG,MPI_COMM_WORLD,&prcv_reqs[from]);
    MPI_Start(&prcv_reqs[from]);
    prcv_vect[from]->request = &prcv_reqs[from];
  }
  prcv_count = n;
}
/*-------------------------------------------------------------------------------*/
bool MPIComm::anyDataReceived(void * e){
  MPI_Status st;
  int index,flag;
  MailBoxEvent *event = (MailBoxEvent *)e;
  if ( prcv_count ==0 )
    return false;
  MPI_Testany(prcv_count,prcv_reqs,&index,&flag,&st);
  if ( !flag || index == MPI_UNDEFINED)
    return false;
  LOG_INFO(LOG_MULTI_THREAD,"Flag:%d, From:%d, count:%d\n",flag,index,prcv_count);
  event->direction = MailBoxEvent::Received;
  event->length    = prcv_vect[index]->length;
  event->host      = index;
  event->tag       = prcv_vect[index]->tag ;
  event->handle    = 0;
  event->memory    = prcv_vect[index]->mem;
  event->buffer    = prcv_vect[index]->buf;

  prcv_vect[index]->mem = dtEngine.newDataMemory();
  prcv_vect[index]->buf = prcv_vect[index]->mem->getAddress();
  //  LOG_INFO(LOG_MULTI_THREAD,"prcv_vect[%d].buf:%p\n",index,prcv_vect[index]->buf);
  //  LOG_INFO(LOG_MULTI_THREAD,"prcv_vect[%d].len:%d\n",index,prcv_vect[index]->length);
  //  LOG_INFO(LOG_MULTI_THREAD,"prcv_vect[%d].tag:%d\n",index,prcv_vect[index]->tag);
  MPI_Recv_init( prcv_vect[index]->buf,
	     prcv_vect[index]->length , MPI_BYTE,
	     index, // from node
	     prcv_vect[index]->tag    ,
	     MPI_COMM_WORLD,
	    &prcv_reqs[index]);

  MPI_Start( &prcv_reqs[index] );

  prcv_vect[index]->request = &prcv_reqs[index];

  return true;

}
/*--------------------------------------------*/
double MPIComm::getBandwidth(){
  double bw= tot_sent_len / ((double)tot_sent_time);
  LOG_INFO(LOG_DLB,"%lf\n",bw);
  return bw;
}
