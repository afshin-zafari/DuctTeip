#include "basic.hpp"
#include "engine.hpp"
#include "glb_context.hpp"
#include "procgrid.hpp"

#define MTHRD_DBG 0
engine dtEngine;
int version,simulation;

DuctTeipWork &DuctTeipWork::operator =(DuctTeipWork *_work){
  task     = _work->task;
  data     = _work->data;
  listener = _work->listener;
  state    = _work->state;
  item     = _work->item;
  tag      = _work->tag;
  event    = _work->event;
  host     = _work->host;
  return *this;
}
void DuctTeipWork::dump(){
  printf("work dump: tag:%d , item:%d\n",tag,item);
  printf("work dump:  ev:%d , stat:%d\n",event,state);
  printf("work dump: host:%d\n",host);
}

/*===================================================================*/
engine::engine(){
  net_comm = new MPIComm;
  mailbox = new MailBox(net_comm);
  SetStartTime();
  initComm();
  last_task_handle = 0;
  term_ok = TERMINATE_INIT;
  runMultiThread=true;
  initDLB();
  thread_model = 0;
  task_submission_finished=false;
  LOG_INFO(LOG_MULTI_THREAD,"mpi tick :%f\n",MPI_Wtick());
}
/*---------------------------------------------------------------------------------*/
void engine::initComm(){
  net_comm->initialize();
  resetTime();
  me = net_comm->get_host_id();

}
/*---------------------------------------------------------------------------------*/

engine::~engine(){
  delete net_comm;
  delete thread_manager;
  delete data_memory;
}
/*---------------------------------------------------------------------------------*/
void engine::setSkips(long st,long sw,long sm,long to){
  skip_term = st;
  skip_work = sw;
  skip_mbox = sm;
  time_out = to;
}
/*---------------------------------------------------------------------------------*/
SuperGlue<Options> * engine::getThrdManager() {return thread_manager;}
int engine::getLocalNumBlocks(){return local_nb;}
/*---------------------------------------------------------------------------------*/

TaskHandle  engine::addTask(IContext * context,
			    string task_name,
			    unsigned long key, 
			    int task_host, 
			    list<DataAccess *> *data_access)
{
  IDuctteipTask *task = new IDuctteipTask (context,task_name,key,task_host,data_access);
  criticalSection(Enter) ;
  
  TaskHandle task_handle = last_task_handle ++;
  if(last_task_handle ==1){
    LOG_INFO(LOG_MULTI_THREAD,"First task is submitted.\n");

#if POST_RECV_DATA == 1
    DataHandle dh;
    DataVersion dv;
    IDuctteipTask t;
    IListener l;
    int mb =  cfg->getYBlocks();
    int nb =  cfg->getXBlocks();
    int ny = cfg->getYDimension() / mb;
    int nx = cfg->getXDimension() / nb;

    long dps = ny * nx * sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
    if ( simulation ) {
      dps = sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
    }
    LOG_INFO(LOG_MULTI_THREAD,"DataPackSize:%ld\n",dps);
    int P = net_comm->get_host_count();
    net_comm->postReceiveData(P,dps,data_memory);    
#endif
    dt_log.addEventStart(this,DuctteipLog::ProgramExecution);
  }
  task->setHandle(task_handle);
  LOG_METRIC(DuctteipLog::TaskDefined);
  task_list.push_back(task);
  if (task_host != me ) {
    putWorkForSendingTask(task);
  }
  else {
    putWorkForNewTask(task);
  }
  criticalSection(Leave) ;
  signalWorkReady();
  if ( !runMultiThread ) {
    doProcessMailBox();
    doProcessWorks();
  }
  return task_handle;
}
/*---------------------------------------------------------------------------------*/
void engine::dumpTasks(){
  list<IDuctteipTask *>::iterator it;
  for (  it =task_list.begin();  it!= task_list.end(); it ++) {
    IDuctteipTask &t = *(*it);
    t.dump();
    break;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::sendPropagateTask(byte *buffer, int size, int dest_host) {
  mailbox->send(buffer,size,MailBox::PropagationTag,dest_host);
  prop_buffer = buffer;
  prop_buffer_length = size;
}
/*---------------------------------------------------------------------------------*/
void engine::receivedListener(MailBoxEvent *event){
  IListener *listener = new IListener;
  int offset = 0 ;

  LOG_EVENT(DuctteipLog::ListenerReceived);

  LOG_INFO(LOG_LISTENERS,"ev.buf:%p ev.len:%d\n",event->buffer,event->length);
  
  listener->deserialize(event->buffer,offset,event->length);
  listener->setHost ( me ) ;
  listener->setSource ( event->host ) ;
  listener->setReceived(true);
  listener->setDataSent(false);

  int host = event->host;
  DataVersion version = listener->getRequiredVersion();
  version.dump();
  IData *data= listener->getData();
  if ( data ==NULL){
    LOG_INFO(LOG_MULTI_THREAD,"invalid data \n");
    return;
  }
  listener->getData()->listenerAdded(listener,host,version);
  criticalSection(Enter); 
  listener->setHandle( last_listener_handle ++);
  listener->setCommHandle(-1);
  listener->dump();
  listener_list.push_back(listener);
  putWorkForReceivedListener(listener);
  criticalSection(Leave); 

}
/*---------------------------------------------------------------------------------*/
void engine::waitForTaskFinish(){
  
   
  if ( !checkRunningTasks() ) 
    return;
}
  
/*---------------------------------------------------------------------------------*/
void engine::finalize(){
  if ( runMultiThread ) {
    pthread_mutex_destroy(&thread_lock);
    pthread_mutexattr_destroy(&mutex_attr);
    task_submission_finished=true;
    LOG_INFO(LOG_MULTI_THREAD,"before join\n");
    if ( thread_model  >=1){
      pthread_join(mbsend_tid,NULL);
      LOG_INFO(LOG_MULTI_THREAD," MBSend thread joined\n");
    }
    if ( thread_model  >=2){
      pthread_join(mbrecv_tid,NULL);
      LOG_INFO(LOG_MULTI_THREAD,"MBRecv thread joined\n");
    }
    pthread_join(thread_id,NULL);
    LOG_INFO(LOG_MULTI_THREAD,"Admin thread joined\n");
  }
  else
    doProcessLoop((void *)this);
  globalSync();
  if(cfg->getDLB())
    dumpDLB();
}
/*---------------------------------------------------------------------------------*/
void engine::globalSync(){
  dt_log.logLoadChange(0);
  dt_log.logImportTask(0);
  dt_log.logExportTask(0);

  LOG_INFO(LOG_MULTI_THREAD,"Non-Busy time:%ld\n",wasted_time);
  if (true || cfg->getYDimension() == 2400){
    char s[20];
    sprintf(s,"sg_log_file-%2.2d.txt",me);
    Log<Options>::dump(s);
  }
  long tc=thread_manager->getTaskCount();
  dt_log.dump(tc);
  LOG_INFO(LOG_MULTI_THREAD,"after  dt log \n");

  LOG_INFO(LOG_MULTI_THREAD,"before comm->finish\n");
  net_comm->finish();
  LOG_INFO(LOG_MULTI_THREAD,"after  comm->finish\n");


}
/*---------------------------------------------------------------------------------*/
TimeUnit engine::elapsedTime(int scale){
  TimeUnit t  = getTime();
  return ( t - start_time)/scale;
}
/*---------------------------------------------------------------------------------*/
void engine::dumpTime(char c){
}
/*---------------------------------------------------------------------------------*/
bool engine::isAnyUnfinishedListener(){
  list<IListener *>::iterator it;
  for ( it = listener_list.begin(); it != listener_list.end(); it ++)
    {
      IListener *listener = (*it);
      if ( listener->isReceived() ) 
	if ( !listener->isDataSent() ) {
	  listener->dump();
	  return true;
	}
    }
  return false;
}
/*---------------------------------------------------------------------------------*/
long engine::getUnfinishedTasks(){
  list<IDuctteipTask *>::iterator it;
  TimeUnit t = getTime();
  it = task_list.begin();
  for(; it != task_list.end(); ){
    IDuctteipTask * task = (*it);
    if (task->canBeCleared()){
      checkRunningTasks();
      it=task_list.erase(it);
    }
    else
      it ++;
  }
  wasted_time += getTime() - t;
  //  LOG_INFO(LOG_MULTI_THREAD,"Non-Busy time:%ld\n",wasted_time);
  return task_list.size();
}
/*---------------------------------------------------------------------------------*/
bool engine::canTerminate(){
  static int once=0;
  TimeUnit t =getTime();
  {

    LOG_EVENT(DuctteipLog::CheckedForTerminate);

    if (elapsedTime(TIME_SCALE_TO_SECONDS)>time_out){
      if (checkRunningTasks(1) >0 || work_queue.size() >0){	
	wasted_time += getTime() - t;
	return false;
      }
      LOG_INFO(LOG_MULTI_THREAD,"error: timeout\n");
      wasted_time += getTime() - t;
      return true;
    }
    /*
    if ( !net_comm->canTerminate() ) {
      wasted_time += getTime() - t;
      return false;
    }
    */

    if ( term_ok == TERMINATE_OK ) {
      wasted_time += getTime() - t;
      return true;
    }
    
    
  
    if (task_submission_finished && getUnfinishedTasks() < 1){
      if (once==0){
	dt_log.addEventEnd(this,DuctteipLog::ProgramExecution);
	once++;
      }
      if ( net_comm->get_host_count() ==1 ) {
	wasted_time += getTime() - t;
	return true;
      }
      if (!isAnyUnfinishedListener() ){
	sendTerminateOK();
	static int flag=0;
	if ( !flag){
	  LOG_INFO(LOG_MULTI_THREAD,"unf task:%ld unf-lsnr:%d task_submission_finished:%d\n",
		   getUnfinishedTasks(),isAnyUnfinishedListener(),task_submission_finished);
	  flag=1;
	}
      }
    }
  }
  wasted_time += getTime() - t;
  return false;
}
/*---------------------------------------------------------------------------------*/
int  engine::getTaskCount(){    return task_list.size();  }
/*---------------------------------------------------------------------------------*/
void engine::show_affinity() {
  DIR *dp;
  assert((dp = opendir("/proc/self/task")) != NULL);
    
  struct dirent *dirp;
    
  while ((dirp = readdir(dp)) != NULL) {
    if (dirp->d_name[0] == '.')
      continue;
      
    cpu_set_t affinityMask;
    sched_getaffinity(atoi(dirp->d_name), sizeof(cpu_set_t), &affinityMask);
    std::stringstream ss;
    for (size_t i = 0; i < sizeof(cpu_set_t); ++i) {
      if (CPU_ISSET(i, &affinityMask))
	ss << 1;
      else
	ss << 0;
    }
    if(0)fprintf(stderr, "tid %d affinity %s\n", atoi(dirp->d_name), ss.str().c_str());
  }
  closedir(dp);
}
/*---------------------------------------------------------------------------------*/
void engine::setConfig(Config *cfg_){
  cfg = cfg_;
  num_threads = cfg->getNumThreads();
  local_nb = cfg->getXLocalBlocks();
  int mb =  cfg->getYBlocks();
  int nb =  cfg->getXBlocks();
  int ny = cfg->getYDimension() / mb;
  int nx = cfg->getXDimension() / nb;
  DataHandle dh;
  DataVersion dv;
  IDuctteipTask t;
  IListener l;

  long dps = ny * nx * sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
  dt_log.setParams(cfg->getProcessors(),dps,l.getPackSize(),t.getPackSize());
  if ( simulation ) {
    dps = sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
  }
  LOG_INFO(LOG_MULTI_THREAD,"DataPackSize:%ld\n",dps);
  data_memory = new MemoryManager (  nb * mb/3 ,dps );
  int ipn = cfg->getIPN();
  thread_manager = new ThreadManager<Options> ( num_threads ,0* (me % ipn )  * 16/ipn) ;
  show_affinity();
  dt_log.N = cfg->getXDimension();
  dt_log.NB = nb;
  dt_log.nb = local_nb;
  dt_log.p = cfg->getP_pxq();
  dt_log.q = cfg->getQ_pxq();
  dt_log.cores = num_threads;

}
/*---------------------------------------------------------------------------------*/
void *engine::runMBRecvThread(void  *p){
  engine *_this=(engine *)p;
  MailBoxEvent event;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);	
  while(!_this->mb_canTerminate(1)){
    LOG_INFO(LOG_MULTI_THREAD,"any receive?\n");
    _this->mailbox->waitForAnyReceive(_this->getMemoryManager(),&event);
    _this->criticalSection(Enter);
    _this->processEvent(event);  
    _this->criticalSection(Leave);
    _this->signalWorkReady();
    LOG_INFO(LOG_MULTI_THREAD,"MB Recv Thread, some recv happened\n");
  }
  pthread_exit(NULL);
  return 0;
}
/*---------------------------------------------------------------------------------*/
bool engine::mb_canTerminate(int s_or_r){
  if (mailbox->getSelfTerminate(s_or_r) ){
    return true;
  }
  if ( canTerminate() ){
    LOG_INFO(LOG_MULTI_THREAD,"returns true\n");
    return true;
  }
  return false;  
}
/*---------------------------------------------------------------------------------*/
void engine::waitForWorkReady(){
  if (work_queue.size() )
    return;
  LOG_INFO(LOG_MULTI_THREAD,"wait for lock\n"); 
  pthread_mutex_lock  (&work_ready_mx);

  LOG_INFO(LOG_MULTI_THREAD,"wait for work cond\n");
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 1;
  pthread_cond_timedwait   (&work_ready_cv,&work_ready_mx,&ts);

  pthread_mutex_unlock(&work_ready_mx);
  LOG_INFO(LOG_MULTI_THREAD,"unlock work ready\n"); 
  
}
/*---------------------------------------------------------------------------------*/
void engine::signalWorkReady(IDuctteipTask * task){
  pthread_mutex_lock(&work_ready_mx);
  pthread_cond_signal(&work_ready_cv);
  pthread_mutex_unlock(&work_ready_mx);
  if ( thread_model<1){
    criticalSection(Enter);
    if ( task){
      putWorkForFinishedTask(task);
      task->setState(IDuctteipTask::UpgradingData);    
    }
    criticalSection(Leave);
  }

}
/*---------------------------------------------------------------------------------*/
void *engine::runMBSendThread(void  *p){
  engine *_this=(engine *)p;
  MailBoxEvent event;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
  LOG_INFO(LOG_MULTI_THREAD,"run MB Send Thread\n");
  while(!_this->mb_canTerminate(0)){
    LOG_INFO(LOG_MULTI_THREAD,"any send?\n");
    _this->mailbox->waitForAnySendComplete(&event);
    _this->criticalSection(Enter);
    _this->processEvent(event);  
    _this->criticalSection(Leave);
    _this->signalWorkReady();
  }
  LOG_INFO(LOG_MULTI_THREAD,"exits MB Send Thread\n");

  pthread_exit(NULL);
  return 0;
}
/*---------------------------------------------------------------------------------*/
long engine::getThreadId(int t){
  if ( t == MAIN_THREAD)
    return pthread_self();
  if ( t == ADMIN_THREAD)
    return thread_id;
  if ( t == MBSEND_THREAD)
    return mbsend_tid;
  if ( t == MBRECV_THREAD)
    return mbrecv_tid;
  
  return -1;
}
/*---------------------------------------------------------------------------------*/
void engine::doProcess(){

  dt_log.logLoadChange(0);
  dt_log.logImportTask(0);
  dt_log.logExportTask(0);
  if ( runMultiThread ) {
    sched_param sp;
    int r1,r2;
    sp.sched_priority=1;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    r1=pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    r2=pthread_attr_setschedparam(&attr,&sp);
    pthread_create(&thread_id,&attr,engine::doProcessLoop,this);
    pthread_mutex_init(&thread_lock,&mutex_attr);
    if (thread_model >=1){
      int err=pthread_create(&mbsend_tid,&attr,engine::runMBSendThread,this);
      LOG_INFO(LOG_MULTI_THREAD,"MBSendThrdID:%X, Err:%d\n",mbsend_tid,err);
    }
    if (thread_model >=2){
      int err=pthread_create(&mbrecv_tid,&attr,engine::runMBRecvThread,this);
      LOG_INFO(LOG_MULTI_THREAD,"MBRecvThrdID:%X, Err:%d\n",mbrecv_tid,err);
    }
  }
  else{
    fprintf(stderr,"single thread run\n");
  }
}
/*---------------------------------------------------------------------------------*/
void engine::doSelfTerminate(){
  byte  dummy;
  mailbox->send(&dummy,1,MailBox::SelfTerminate,me,false);
}  
/*---------------------------------------------------------------------------------*/
void *engine::doProcessLoop(void *p){
  engine *_this = (engine *)p;
  unsigned long loop=0;
  TimeUnit tot=0,t,dt=0,pw=0,mb=0,tf=0,ct=0;
  LOG_INFO(LOG_MULTI_THREAD, "do process loop started\n");
  while(true){
    t=getTime();
    if ( _this->getThreadModel() <1){
      dt = getTime();
      _this->doProcessMailBox();
      mb += getTime() - dt;
    }
    else{
      _this->waitForWorkReady();
      LOG_INFO(LOG_MULTI_THREAD,"some works are ready.\n");
    }
    dt = getTime();
    _this->waitForTaskFinish();
    tf += getTime() - dt;
    dt = getTime();
    _this->doProcessWorks();
    pw += getTime() - dt;
    _this->doDLB();
    dt = getTime();
    if ( _this->canTerminate() ){
      if ( _this->getThreadModel() >=1){
	_this->doSelfTerminate();
      }
      ct += getTime() - dt;
      tot += getTime() - t;
      break;
    }
    ct += getTime() - dt;
    tot += getTime() - t;
    loop++;
    usleep(config.ps);
  }
  LOG_INFO(LOG_MULTI_THREAD, "do process loop finished, tot-time=%ld, sleep-time=%ld\n",tot,config.ps *loop);
  LOG_INFO(LOG_MULTI_THREAD,"prc-wrk:%ld, mail-box:%ld, task-finish:%ld, terminate:%ld, total=%lf\n",pw,mb,tf,ct,(pw+mb+tf+ct)/1000000.0);
  pthread_exit(NULL);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForDataReady(list<DataAccess *> *data_list){
  list<DataAccess *>::iterator it;
  for (it =data_list->begin(); it != data_list->end() ; it ++) {
    IData *data=(*it)->data;
    putWorkForSingleDataReady(data);
  }
}
/*---------------------------------------------------------------------------------*/
MemoryItem *engine::newDataMemory(){
  MemoryItem *m = data_memory->getNewMemory();
  return m;
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForCheckAllTasks(){
  list<IDuctteipTask *>::iterator it;
  it = task_list.begin();
  for(; it != task_list.end(); ){
    IDuctteipTask *task = (*it);
    if (task->canBeCleared()){
      it=task_list.erase(it);
      if(0)printf("TL:%ld\n",task_list.size());
    }
    else
      it ++;
  }
  for(it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask * task = (*it);
    if (task->getHost() != me ) continue;
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Received;
    work->item  = DuctTeipWork::CheckTaskForRun;
    work->host  = task->getHost();
    work_queue.push_back(work);
  }
  //printf("------------------------------------\n");
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForReceivedListener(IListener *listener){
  DuctTeipWork *work = new DuctTeipWork;
  work->listener  = listener;
  work->tag   = DuctTeipWork::ListenerWork;
  work->event = DuctTeipWork::Received;
  work->item  = DuctTeipWork::CheckListenerForData;
  work->host  = me ;
  work_queue.push_back(work);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForSendingTask(IDuctteipTask *task){
  DuctTeipWork *work = new DuctTeipWork;
  work->task  = task;
  work->tag   = DuctTeipWork::TaskWork;
  work->event = DuctTeipWork::Added;
  work->item  = DuctTeipWork::SendTask;
  work->host  = task->getHost(); // ToDo : create_place, dest_place
  work_queue.push_back(work);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForPropagateTask(IDuctteipTask *task){
  DuctTeipWork *work = new DuctTeipWork;
  work->task  = task;
  work->tag   = DuctTeipWork::TaskWork;
  work->event = DuctTeipWork::Added;
  work->item  = DuctTeipWork::CheckTaskForRun;
  work->host  = task->getHost(); // ToDo : create_place, dest_place
  work_queue.push_back(work);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForNewTask(IDuctteipTask *task){
  DuctTeipWork *work = new DuctTeipWork;
  work->task  = task;
  work->tag   = DuctTeipWork::TaskWork;
  work->event = DuctTeipWork::Added;
  work->item  = DuctTeipWork::CheckTaskForData;
  work->host  = task->getHost(); // ToDo : create_place, dest_place
  work_queue.push_back(work);
  DuctTeipWork *second_work = new DuctTeipWork;
  *second_work = *work;
  second_work->item  = DuctTeipWork::CheckTaskForRun;
  work_queue.push_back(second_work);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForReceivedTask(IDuctteipTask *task){
  DuctTeipWork *work = new DuctTeipWork;
  work->task  = task;
  work->tag   = DuctTeipWork::TaskWork;
  work->event = DuctTeipWork::Received;
  work->item  = DuctTeipWork::CheckTaskForData;
  work->host  = task->getHost();
  work_queue.push_back(work);
  DuctTeipWork *second_work = new DuctTeipWork;
  *second_work = *work;
  second_work->item  = DuctTeipWork::CheckTaskForRun;
  work_queue.push_back(second_work);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForFinishedTask(IDuctteipTask * task){
  DuctTeipWork *work = new DuctTeipWork;
  work->task  = task;
  work->tag   = DuctTeipWork::TaskWork;
  work->event = DuctTeipWork::Finished;
  work->item  = DuctTeipWork::UpgradeData;
  work->host  = task->getHost();
  work_queue.push_back(work);
  LOG_INFO(LOG_MULTI_THREAD,"%s\n",task->getName().c_str());
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForSingleDataReady(IData* data){
  DuctTeipWork *work = new DuctTeipWork;
  work->data = data;
  work->tag   = DuctTeipWork::DataWork;
  work->event = DuctTeipWork::DataUpgraded;
  work->item  = DuctTeipWork::CheckAfterDataUpgraded;
  work_queue.push_back(work); 
  //LOG_INFO(LOG_MULTI_THREAD+LOG_DATA,"Put DataUpgrade work for:%s.\n",data->getName().c_str());     
}
/*---------------------------------------------------------------------------------*/
void engine::receivedTask(MailBoxEvent *event){
  IDuctteipTask *task = new IDuctteipTask;
  int offset = 0 ;
  task->deserialize(event->buffer,offset,event->length);
  task->setHost ( me ) ;

  criticalSection(Enter);
  
  task->setHandle( last_task_handle ++);
  task_list.push_back(task);
  putWorkForReceivedTask(task);
  LOG_EVENT(DuctteipLog::TaskReceived);
  
  criticalSection(Leave); 

}
/*---------------------------------------------------------------------------------*/
void engine::importedTask(MailBoxEvent *event){
  IDuctteipTask *task = new IDuctteipTask;
  int offset = 0 ;
  task->deserialize(event->buffer,offset,event->length);
  task->setHost ( me ) ;
  task->setImported(true);
  task->createSyncHandle();
  criticalSection(Enter); 

  task->setHandle( last_task_handle ++);
  import_tasks.push_back(task);
  dt_log.logImportTask(import_tasks.size());
    
  LOG_EVENT(DuctteipLog::TaskImported);

  criticalSection(Leave); 
  if(0)fprintf(stderr,"Task is imported:%s\n",task->getName().c_str());
  task->dump();

}
/*---------------------------------------------------------------------------------*/
long int engine::checkRunningTasks(int v){

  list<IDuctteipTask *>::iterator it;
  int cnt =0 ;
  long sc =0;
  TimeUnit t= getTime();
  for(it = running_tasks.begin(); it != running_tasks.end(); ){
    IDuctteipTask *task=(*it);
    if(task->canBeCleared()){
      it = running_tasks.erase(it);	
      dt_log.logLoadChange(running_tasks.size());
      continue;
    }
    if (task->isFinished() ) {
      //putWorkForFinishedTask(task);
      //task->setState(IDuctteipTask::UpgradingData);
      cnt ++;
    }
    else
      it++;
  }
  if ( config.getDLB() )
    checkMigratedTasks(); 
  sc = cnt+import_tasks.size()+export_tasks.size()+work_queue.size();
  t = getTime() - t;
  //  LOG_INFO(LOG_MULTI_THREAD,"Local Non-Busy time:%ld\n",t);

  return cnt+import_tasks.size()+export_tasks.size();
}
/*---------------------------------------------------------------------------------*/
void  engine::checkMigratedTasks(){
  checkImportedTasks();
  checkExportedTasks();
}
/*---------------------------------------------------------------------------------*/
void  engine::checkImportedTasks(){

  list<IDuctteipTask *>::iterator task_it;
  for(task_it = import_tasks.begin(); task_it != import_tasks.end(); ){
    IDuctteipTask *task=(*task_it);
    if (task->isFinished() ) {
      //task->upgradeData();
      list<DataAccess *> *data_list=task->getDataAccessList() ;
      list<DataAccess *> :: iterator it;
      for ( it = data_list->begin(); it != data_list->end()  ; it ++){
	IData * data = (*it)->data;
	if ( (*it)->type == IData::WRITE ){
	  data->serialize();
	  if(0)printf("Result of imported task is sent back to :%d\n",data->getHost());
	  data->dumpCheckSum('R');
	  data->dump(' ');

	  mailbox->send(data->getHeaderAddress(),
			data->getPackSize(),
			MailBox::MigratedTaskOutDataTag,
			data->getHost());	  
	}	
      }
      task_it = import_tasks.erase(task_it);
      if (dlb_state == TASK_IMPORTED && import_tasks.size() ==0){
	dlb_state=DLB_STATE_NONE;
	dlb_stage= DLB_NONE;
	if(0)printf("import task list emptied.\n");
      }
      dt_log.logImportTask(import_tasks.size());
    }
    else{
      task_it++;
    }
  }
}
/*---------------------------------------------------------------------------------*/
void  engine::checkExportedTasks(){
  if (dlb_state == TASK_EXPORTED && export_tasks.size() ==0){
    printf("export task list emptied.\n");
    dlb_state=DLB_STATE_NONE;
    dlb_stage= DLB_NONE;
  }
  return;


  list<IDuctteipTask *>::iterator task_it;
  for(task_it = export_tasks.begin(); task_it != export_tasks.end(); ){
    IDuctteipTask *task=(*task_it);
    list<DataAccess *> *data_list=task->getDataAccessList() ;
    list<DataAccess *> :: iterator it;
    bool all_cleared=true;
      
    for ( it = data_list->begin(); it != data_list->end()  ; it ++){
      IData * data = (*it)->data;
      if ( (*it)->type == IData::WRITE ){
	printf("task:%d,%s - data:%s, rtv:%s, reqv:%s \n",
	       task->getState(),
	       task->getName().c_str(),
	       data->getName().c_str(),
	       data->getRunTimeVersion((*it)->type).dumpString().c_str(),
	       (*it)->required_version.dumpString().c_str());
	if ( data->getRunTimeVersion((*it)->type) != (*it)->required_version ) {
	  all_cleared = false;
	}
      }	
    }
    if(0 && all_cleared){
      if(1)printf("Task '%s' is removed from export.\n",task->getName().c_str());
      task->dump();
      task->setState(IDuctteipTask::CanBeCleared);
      task_it = export_tasks.erase(task_it);
      dt_log.logExportTask(export_tasks.size());
    }
    /*
      if (task->getState() == IDuctteipTask::CanBeCleared){
      if(1)printf("Task '%s' is removed from export.\n",task->getName().c_str());
      task_it = export_tasks.erase(task_it);
      dt_log.logExportTask(export_tasks.size());
      }
      else{
      task_it++;
      }
    */
    task_it++;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::doProcessWorks(){
  TimeUnit t= getTime();
  if ( work_queue.size() < 1 ) {
    wasted_time += getTime() - t;
//    LOG_INFO(LOG_MULTI_THREAD,"No work. #running tasks:%d, wt:%ld\n",running_tasks.size(),wasted_time);
    return;
  }
  criticalSection(Enter); 
    
  LOG_EVENT(DuctteipLog::WorkProcessed);
    
  DuctTeipWork *work = work_queue.front();
  work_queue.pop_front();
  executeWork(work);    
  //  LOG_INFO(LOG_MULTI_THREAD,"work:%d, tag:%d, item:%d\n",work->event,work->tag,work->item);
  criticalSection(Leave); 
}
/*---------------------------------------------------------------------------------*/
bool engine::isDuplicateListener(IListener * listener){
  list<IListener *>::iterator it;
  for(it = listener_list.begin(); it != listener_list.end();it ++){
    IListener *lsnr=(*it);
    if (listener->getData()->getDataHandle() == lsnr->getData()->getDataHandle()) {
      if (listener->getRequiredVersion() == lsnr->getRequiredVersion()) {
	return true;
      }
    }      
  }
  return false;
}
/*---------------------------------------------------------------------------------*/
bool engine::addListener(IListener *listener ){
  if (isDuplicateListener(listener))
    return false;
  int handle = last_listener_handle ++;
  listener_list.push_back(listener);
  listener->setHandle(handle);
  listener->setCommHandle(-1);
  LOG_EVENT(DuctteipLog::ListenerDefined);

  return true;
}
/*---------------------------------------------------------------------------------*/
void engine::checkTaskDependencies(IDuctteipTask *task){
  list<DataAccess *> *data_list= task->getDataAccessList();
  list<DataAccess *>::iterator it;
  for (it = data_list->begin(); it != data_list->end() ; it ++) {
    DataAccess &data_access = *(*it);
    int host = data_access.data->getHost();
    data_access.data->addTask(task);
    if ( host != me ) {
      IListener * lsnr = new IListener((*it),host);
      if ( addListener(lsnr) ){
	MessageBuffer *m=lsnr->serialize();
	unsigned long comm_handle = mailbox->send(m->address,m->size,MailBox::ListenerTag,host);	
	IData *data=data_access.data;//lsnr->getData();
	data->allocateMemory();
	data->prepareMemory();
	LOG_EVENT(DuctteipLog::ListenerSent);
	lsnr->setCommHandle ( comm_handle);
      }
      else{
	delete lsnr;
      }
    }
  }
}
/*---------------------------------------------------------------------------------*/
void engine::runFirstActiveTask(){
  if (!cfg->getDLB())
    return;
  if(getActiveTasksCount()>DLB_BUSY_TASKS){
    return;
  }
  list<IDuctteipTask *>::iterator it;
  for(it= running_tasks.begin(); it != running_tasks.end(); it ++){
    IDuctteipTask *task = *it;
    if (task->isExported())continue;
    if(task->isRunning()) continue;
    if(task->isFinished()) continue;
    if(task->canBeCleared()) continue;
    if(task->isUpgrading()) continue;
    task->run();
    return;
  }
}

/*---------------------------------------------------------------------------------*/
long engine::getActiveTasksCount(){
  list<IDuctteipTask *>::iterator it;
  long count = 0 ;
  if(running_tasks.size() && 0 )
    printf("WaitForData=2, Running, Finished, UpgradingData, CanBeCleared\n");
  for(it= running_tasks.begin(); it != running_tasks.end(); it ++){
    IDuctteipTask *task = *it;
    if(DLB_DEBUG)printf(" %d,%d ",task->getState(),task->isExported());
    if ((task->isRunning()   ||
	 task->isFinished()  ||
	 task->isUpgrading() ||
	 task->canBeCleared() ) &&
	!task->isExported()
	){
      count ++;
    }
  }
  dlb_profile.max_para = (running_tasks.size() > dlb_profile.max_para )?running_tasks.size():dlb_profile.max_para ;
  dlb_profile.max_para2 = (count > dlb_profile.max_para2 )?count:dlb_profile.max_para2 ;
  if(DLB_DEBUG)printf("\n");
  if(DLB_DEBUG)printf("-RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,running_tasks.size()
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  if ( dlb_state != TASK_IMPORTED && dlb_state!= TASK_EXPORTED){
    if ( dlb_stage != DLB_FINDING_BUSY && dlb_stage != DLB_FINDING_IDLE){
      dlb_state = ( (running_tasks.size()-count)>DLB_BUSY_TASKS)?DLB_BUSY:DLB_IDLE;
      if(DLB_DEBUG)printf("%d\n",__LINE__);
      restartDLB(0);
      doDLB(0);
    }
  }
  if(DLB_DEBUG)printf("+RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,running_tasks.size()
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  return count;
}
/*---------------------------------------------------------------------------------*/
void engine::executeTaskWork(DuctTeipWork * work){
  switch (work->item){
  case DuctTeipWork::UpgradeData:
    LOG_INFO(LOG_MULTI_THREAD,"Task %s, upgrade data. \n",work->task->getName().c_str());
    work->task->upgradeData('W');
    //runFirstActiveTask(); 
    break;
  case DuctTeipWork::SendTask:
    sendTask(work->task,work->host);
    break;
  case DuctTeipWork::CheckTaskForData:
    checkTaskDependencies(work->task);
    break;
  case DuctTeipWork::CheckTaskForRun:
    {
      TaskHandle task_handle = work->task->getHandle();
      LOG_EVENT(DuctteipLog::CheckedForRun);
      if ( work->task->canRun('W') ) {
	running_tasks.push_back(work->task);
	dt_log.logLoadChange(running_tasks.size());
	if (cfg->getDLB()){
	  if (getActiveTasksCount() < DLB_BUSY_TASKS ){
	    work->task->run();
	  }
	}
	else{
	  LOG_INFO(LOG_MULTI_THREAD,"Data Upgraded, run task.\n");
	  work->task->run();
	}
	return;  
      }
    }
    break;
  default:
    fprintf(stderr,"error: undefined work : %d \n",work->item);
    break;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::executeDataWork(DuctTeipWork * work){
  switch (work->item){
  case DuctTeipWork::CheckAfterDataUpgraded:
    list<IListener *>::iterator lsnr_it;
    list<IDuctteipTask *>::iterator task_it;
    list<IListener *>          d_listeners = work->data->getListeners();
    list<IDuctteipTask *>      d_tasks     = work->data->getTasks();
    LOG_INFO(LOG_MULTI_THREAD,"Data Upgrade Work, Listener Check.\n");
    LOG_EVENT(DuctteipLog::CheckedForListener);
    for(lsnr_it = d_listeners.begin() ; 
	lsnr_it != d_listeners.end()  ; 
	++lsnr_it){
      IListener *listener = (*lsnr_it);
      listener->checkAndSendData(mailbox);
    }
    //LOG_EVENTX(cft,DuctteipLog::CheckedForTask);
    for(task_it = d_tasks.begin() ; 
	task_it != d_tasks.end()  ;
	++task_it){
      IDuctteipTask *task = (*task_it);
      //LOG_EVENTX(cfr,DuctteipLog::CheckedForRun);
      if (task->canRun('D')) {
	running_tasks.push_back(task);	  
	dt_log.logLoadChange(running_tasks.size());
	if (cfg->getDLB()){
	}	  
	else{
	  //LOG_INFO(LOG_MULTI_THREAD,"Data Upgrade Work, Task run.\n");
	  task->run();
	}	  
      }
    }
    if (cfg->getDLB()){
      runFirstActiveTask();
    }
    break;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::executeListenerWork(DuctTeipWork * work){
  switch (work->item){
  case DuctTeipWork::CheckListenerForData:
    {
      IListener *listener = work->listener;
      listener->checkAndSendData(mailbox) ; 
    }
    break;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::executeWork(DuctTeipWork *work){
  switch (work->tag){
  case DuctTeipWork::TaskWork:      executeTaskWork(work);      break;
  case DuctTeipWork::ListenerWork:  executeListenerWork(work);  break;
  case DuctTeipWork::DataWork:      executeDataWork(work);      break;
  default: fprintf(stderr,"work: tag:%d\n",work->tag);break;
  }
}
/*---------------------------------------------------------------------------------*/
void engine::doProcessMailBox(){
  MailBoxEvent event ;
  TimeUnit t = getTime();
  bool wait= false,completed,found;

  if (net_comm->get_host_count() == 1) 
    return ;
  int *counters = glbCtx.getCounters();
  if ( counters[GlobalContext::TaskInsert] == 0)
    return;
  event.host =-1;
  event.tag = -1;
  LOG_EVENT(DuctteipLog::MailboxGetEvent);
  found =  mailbox->getEvent(data_memory,&event,&completed,wait);

  if ( !found ){
    TimeUnit tt=getTime();
    wasted_time += tt - t;
//    LOG_INFO(LOG_MULTI_THREAD,"No comm happened, wt:%ld,tot-wt:%ld\n",tt-t,wasted_time);
    return ;
  }
  /*  
  LOG_INFO(LOG_MULTI_THREAD,"comp:%d,found:%d,tag:%d,dir:%d\n",
	   completed, found,event.tag,event.direction); 
  */
  processEvent(event);
}
/*---------------------------------------------------------------------------------*/
void engine::processEvent(MailBoxEvent &event){

  switch ((MailBox::MessageTag)event.tag){
  case MailBox::TaskTag:
    if ( event.direction == MailBoxEvent::Received ) {
      receivedTask(&event);
    }
    else {
      IDuctteipTask *task = getTaskByCommHandle(event.handle);
      if (task == NULL) {
	break;
      }
      TaskHandle task_handle = task->getHandle();
      removeTaskByHandle(task_handle);
    }
    break;
  case MailBox::ListenerTag:
    if (event.direction == MailBoxEvent::Received) {
      receivedListener(&event);
    }
    else { 
      IListener *listener = getListenerByCommHandle(event.handle);
      if (listener){
	LOG_EVENT(DuctteipLog::ListenerSendCompleted);
      }
    }
    break;
  case MailBox::DataTag:
    if (event.direction == MailBoxEvent::Received) {
      LOG_INFO(LOG_MULTI_THREAD,"Data received.\n");
      receivedData(&event,event.getMemoryItem());
      LOG_METRIC(DuctteipLog::DataReceived);
    }
    else{
      IListener *listener = getListenerByCommHandle(event.handle);
      if (listener){
	LOG_EVENT(DuctteipLog::DataSendCompleted);
	listener->getData()->dataIsSent(listener->getSource());
      }
    }
    break;
  case MailBox::PropagationTag:
    if (event.direction == MailBoxEvent::Received) {
      receivePropagateTask(event.buffer,event.length);
    }
    break;
  case MailBox::TerminateOKTag:
    receivedTerminateOK(event.host);
    break;
  case MailBox::TerminateCancelTag:
    receivedTerminateCancel(event.host);
    break;
  case MailBox::FindIdleTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(DLB_DEBUG)("FIND_IDLE tag recvd from %d\n",event.host);
      received_FIND_IDLE(event.host);
    }
    break;
  case MailBox::FindBusyTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(DLB_DEBUG)("FIND_BUSY tag recvd from %d\n",event.host);
      received_FIND_BUSY(event.host);
    }
    break;
  case MailBox::DeclineMigrateTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(DLB_DEBUG)(" DECLINE  tag recvd from %d\n",event.host);
      receivedDeclineMigration(event.host);
    }
    break;
  case MailBox::AcceptMigrateTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(DLB_DEBUG)("ACCEPT  tag recvd from %d\n",event.host);
      received_ACCEPTED(event.host);
    }
    break;
  case MailBox::MigrateTaskTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(0)("TASKS imported from %d\n",event.host);
      receivedMigrateTask(&event);
    }
    break;
  case MailBox::MigrateDataTag:
    if ( event.direction == MailBoxEvent::Received ) {
      PRINT_IF(0)("DATA imported from %d\n",event.host);
      PRINT_IF(0)("time:%Ld\n",elapsedTime(1));
      importData(&event);
    }
    break;
  case MailBox::MigratedTaskOutDataTag:
    if ( event.direction == MailBoxEvent::Received ) {
      receiveTaskOutData(&event);
    }
    break;

  }
  if ( event.direction  == MailBoxEvent::Received) {
    if (event.tag != MailBox::DataTag) {
    }
  }
}
/*---------------------------------------------------------------------------------*/
IListener *engine::getListenerForData(IData *data){
  list<IListener *>::iterator it ;
  for(it = listener_list.begin();it != listener_list.end(); it ++){
    IListener *listener = (*it);
    if (listener->getData()->getDataHandle() == data->getDataHandle() )
      if ( listener->getRequiredVersion() == data->getRunTimeVersion(IData::READ) ) 
	{
	  return listener;
	}
  }
  return NULL;
}
/*---------------------------------------------------------------------------------*/
void engine::dumpListeners(){
    
  list<IListener *>::iterator it;
  for (it = listener_list.begin(); it != listener_list.end(); it ++){
    (*it)->dump();
  }
}
/*---------------------------------------------------------------------------------*/
void engine::dumpAll(){
  printf("------------------------------------------------\n");
  dumpTasks();
  dumpListeners();
  printf("------------------------------------------------\n");
}
/*---------------------------------------------------------------------------------*/
void engine::removeListenerByHandle(int handle ) {
  list<IListener *>::iterator it;
  for ( it = listener_list.begin(); it != listener_list.end(); it ++){
    IListener *listener = (*it);
    if (listener->getHandle() == handle){
      criticalSection(Enter); 
      listener_list.erase(it);
      criticalSection(Leave);
      return;
    }
  }
}
/*---------------------------------------------------------------------------------*/
void engine::removeTaskByHandle(TaskHandle task_handle){
  list<IDuctteipTask *>::iterator it;
  criticalSection(Enter);
  for ( it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask *task = (*it);
    if (task->getHandle() == task_handle){
      task_list.erase(it);
      if(0)printf("TL:%ld\n",task_list.size());
      criticalSection(Leave);
      return;
    }
  }
}
/*---------------------------------------------------------------------------------*/
void engine::criticalSection(int direction){
  if ( !runMultiThread)
    return;
  if ( direction == Enter )
    pthread_mutex_lock(&thread_lock);
  else
    pthread_mutex_unlock(&thread_lock);
}
/*---------------------------------------------------------------------------------*/
IDuctteipTask *engine::getTaskByHandle(TaskHandle  task_handle){
  list<IDuctteipTask *>::iterator it;
  criticalSection(Enter); 
  for ( it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask *task = (*it);
    if (task->getHandle() == task_handle){
      criticalSection(Leave);
      return task;
    }
  }
  criticalSection(Leave);
  return NULL;
}
/*---------------------------------------------------------------------------------*/
IDuctteipTask *engine::getTaskByCommHandle(unsigned long handle){
  list<IDuctteipTask *>::iterator it;
  criticalSection(Enter); 
  for ( it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask *task = (*it);
    if (task->getCommHandle() == handle){
      criticalSection(Leave);
      return task;
    }
  }
  criticalSection(Leave);
  fprintf(stderr,"error:task not found by comm-handle:%ld\n",handle);
  return NULL;
}
/*---------------------------------------------------------------------------------*/
IListener *engine::getListenerByCommHandle ( unsigned long  comm_handle ) {
  list<IListener *>::iterator it;
  for (it = listener_list.begin(); it !=listener_list.end();it ++){
    IListener *listener=(*it);
    if ( listener->getCommHandle() == comm_handle)
      return listener;
  }
  fprintf(stderr,"\nerror:listener not found by comm-handle %ld\n",comm_handle);
  return NULL;
}

/*---------------------------------------------------------------*/
void engine::resetTime(){
  start_time = getTime();
}
inline bool engine::IsOdd (int a){return ( (a %2 ) ==1);}
inline bool engine::IsEven(int a){return ( (a %2 ) ==0);}
/*---------------------------------------------------------------------------------*/
void engine::createThreads(){
}
/*=================== Nodes Tree ===========================
                         0
              1                        2
       3            5            4             6
    7     9     11     13     8     10     12     14
  15 17 19 21 23  25 27  29 16 18 20  22 24  26 28  30
  ============================================================
*/
int engine::getParentNodeInTree(int node){
  int parent = -1;
  if ( node <2 ) {
    PRINT_IF(TERMINATE_FLAG)("parent of node:%d is %d\n",node,node-1);
    return node-1;
  }
  parent = node /2;
  if ( parent%2 != 0 && node%2 ==0 ) 
    parent --;
  else if ( parent%2 == 0 && node%2 !=0 ) 
    parent --;
  PRINT_IF(TERMINATE_FLAG)("parent of :%d is :%d\n",node,parent);
  return parent;
}
/*---------------------------------------------------------------------------------*/
void engine::getChildrenNodesInTree(int node,int *nodes,int *count){
    
  int N = net_comm->get_host_count();
  int left=2 *node + node%2;
  int right = 2 *(node+1) + node%2;
  if ( node ==0 ) {
    left  = 1;
    right = 2;
  }
  *count =0;
  nodes[0]=nodes[1]=-1;
  if (left <N ) {
    nodes[0] = left;
    *count=1;
  }
  if ( right <N ) {
    nodes[1]=right;
    *count=2;
  }
  PRINT_IF(TERMINATE_FLAG)("children of %d is %d,[0]=%d,[1]=%d\n",node,*count,nodes[0],nodes[1]);
}
/*---------------------------------------------------------------------------------*/
bool engine::amILeafInTree(){
  const int TREE_BASE=2; // binary tree
  int n,nodes[TREE_BASE];    
  getChildrenNodesInTree(me,nodes,&n);
  PRINT_IF(TERMINATE_FLAG)("is leaf %d?%d\n ",me,n==0);
  if ( n==0)
    return true;
  return false;
}
/*---------------------------------------------------------------------------------*/
void engine::sendTerminateOK(){
  PRINT_IF(TERMINATE_FLAG)("send_term_ok called by node %d, term_ok:%d\n",me,term_ok);
  int parent = getParentNodeInTree(me);
  if (amILeafInTree() && term_ok ==TERMINATE_INIT){      
    mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,parent);
    term_ok = SENT_TO_PARENT;
    PRINT_IF(TERMINATE_FLAG)("TERM_INIT->SENT 2 PARENT,node %d is leaf, sent TERM_OK to parent %d,term_ok is %d\n",
			     me,parent,term_ok);
  }
  else{
    if ( term_ok == ALL_CHILDREN_OK ) {
      if ( parent >=0) { // non-root node
	mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,parent);
	term_ok = SENT_TO_PARENT;
	PRINT_IF(TERMINATE_FLAG)("ALL_OK->SENT2PARENT,parent %d,term_ok:%d\n",parent,term_ok);
      }
      else{ // only root node
	const int TREE_BASE=2; // binary tree
	int n,nodes[TREE_BASE];    
	bool wait = false;
	getChildrenNodesInTree(me,nodes,&n);
	if ( n >=1)
	  mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[0],wait);	  
	if ( n >=2)
	  mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[1],wait);	  
	term_ok = TERMINATE_OK;
	PRINT_IF(TERMINATE_FLAG)("ALL_OK->TERM_OK,node %d,child[0]:%d,child[1]:%d,term_ok:%d\n",
				 me,nodes[0],nodes[1],term_ok);
      }
    }
  }
    
}
/*---------------------------------------------------------------------------------*/
void engine::receivedTerminateOK(int from){
  const int TREE_BASE=2; // binary tree
  int n,nodes[TREE_BASE];
  int parent = getParentNodeInTree(me);
  getChildrenNodesInTree(me,nodes,&n);
  PRINT_IF(TERMINATE_FLAG)("node:%d recv TERM msg from:%d,my term_ok:%d\n",me,from,term_ok);
    
  if (term_ok == TERMINATE_INIT ){
    if ( from == nodes[0] || from == nodes[1]) {
      term_ok = ONE_CHILD_OK;
      PRINT_IF(TERMINATE_FLAG)("TERM_INIT->ONE CHILD OK,node :%d,term_ok:%d\n",me,term_ok);
      if (n == 1) {
	term_ok = ALL_CHILDREN_OK;
	PRINT_IF(TERMINATE_FLAG)("ONE_CHILD->ALL OK,node :%d has only one child,term_ok:%d\n"
				 ,me,term_ok);
      }
    }      
  }
  else if (term_ok == ONE_CHILD_OK) {
    if ( from == nodes[0] || from == nodes[1]) {
      term_ok = ALL_CHILDREN_OK;
      PRINT_IF(TERMINATE_FLAG)("ONE OK->ALL OK,node :%d, term_ok:%d\n",me,term_ok);
    }    	
  }
  else if (term_ok == SENT_TO_PARENT && from == parent) {
    bool wait = false;
    if (n >=1)
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[0],wait);	  
    if (n >= 2)
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[1],wait);	        
    term_ok = TERMINATE_OK;
    PRINT_IF(TERMINATE_FLAG)("SENT 2 PARENT-> TERM OK, node %d,n:%d,child[0]:%d,child[1]:%d,term_ok:%d\n",
			     me,n,nodes[0],nodes[1],term_ok);
  }
}
void engine::receivedTerminateCancel(int from){}
void engine::sendTerminateCancel(){}
/*---------------------------------------------------------------*/
void engine::initDLB(){
  dlb_state = DLB_STATE_NONE;
  dlb_substate = DLB_NONE;
  dlb_stage = DLB_NONE;
  dlb_node = -1;
  dlb_failure = dlb_glb_failure=0;
  dlb_silent_start = 0 ;
  dlb_profile.tot_silent=0;
  dlb_profile.export_task=
    dlb_profile.export_data=
    dlb_profile.import_task=
    dlb_profile.import_data=
    dlb_profile.max_para   =
    dlb_profile.max_para2  =0;
  srand(time(NULL));
}
/*---------------------------------------------------------------*/
void engine::updateDLBProfile(){
  dlb_profile.tot_failure+=dlb_failure+dlb_glb_failure;
  dlb_profile.max_loc_fail=
    (dlb_profile.max_loc_fail >dlb_failure) ?
    dlb_profile.max_loc_fail :dlb_failure  ;
}
/*---------------------------------------------------------------*/
void engine::restartDLB(int s){    
  TIME_DLB;
  if ( dlb_state == TASK_IMPORTED || dlb_state == TASK_EXPORTED){
    if(0)printf("state = %d(Imp=2,Exp=3), =>no restart.\n",dlb_state);
    return;
  }
  if ( dlb_stage == DLB_FINDING_IDLE ||dlb_stage == DLB_FINDING_BUSY){
    return;
  }
  updateDLBProfile();
  dlb_prev_state = dlb_state;
  if ( s <0){
    dlb_stage = DLB_NONE;
    PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
    dlb_state = getDLBStatus();
  }
  if ( dlb_state != dlb_prev_state){
    dlb_failure =0;
  }
  dlb_silent_start = 0 ;
}
/*---------------------------------------------------------------*/
void engine::goToSilent(){
  // log_event start silent
  dlb_failure =0;
  dlb_silent_start = getClockTime(MICRO_SECONDS);
  dlb_stage = DLB_SILENT;
  PRINT_IF(DLB_DEBUG)("stage set to SILENT.\n");
}
bool engine::passedSilentDuration(){
  return ((unsigned long)getClockTime(MICRO_SECONDS) > (unsigned long)SILENT_PERIOD ) ;
}
/*---------------------------------------------------------------*/
void engine::dumpDLB(){
  fprintf(stderr,"DLB Stats:\n");
  fprintf(stderr,"try:%ld fail:%ld tick:%ld decline:%ld\n",
	  dlb_profile.tot_try,
	  dlb_profile.tot_failure,
	  dlb_profile.tot_tick,
	  dlb_glb_failure);
  fprintf(stderr,"loc fail:%ld cost:%ld silence:%ld\n",
	  dlb_profile.max_loc_fail,
	  dlb_profile.tot_cost/1000000L,
	  dlb_profile.tot_silent); 
  fprintf(stderr,"DLB ex task:%ld ex data:%ld im task:%ld im data:%ld max para:%ld or %ld\n",
	  dlb_profile.export_task,
	  dlb_profile.export_data,
	  dlb_profile.import_task,
	  dlb_profile.import_data,
	  dlb_profile.max_para,
	  dlb_profile.max_para2);
}
/*---------------------------------------------------------------*/
int engine::getDLBStatus(){
  PRINT_IF(0&&DLB_DEBUG)("task#:%ld active:%ld\n",running_tasks.size(),getActiveTasksCount());
  if ( (running_tasks.size()-getActiveTasksCount() )> DLB_BUSY_TASKS){
    PRINT_IF(DLB_DEBUG)("BUSY\n");
    return DLB_BUSY;
  }
  PRINT_IF(DLB_DEBUG)("IDLE\n");
  return DLB_IDLE;
}
/*---------------------------------------------------------------*/
void engine::doDLB(int st){
  TIME_DLB;
  if (!cfg->getDLB())
    return;
  if (net_comm->get_host_count()==1) 
    return;
  if ( task_list.size() ==0 && running_tasks.size() ==0 ) 
    return;
  static int s=0;
  if(++s<10000)
    return;
  s=0;
  if ( dlb_stage == DLB_SILENT ){
    PRINT_IF(DLB_DEBUG)("I'm Silent.  Duration passed so far: %ld ? %ld.\n",dlb_profile.tot_silent,SILENT_PERIOD);
    if ( !passedSilentDuration() ) 
      return;
    else{
      PRINT_IF(DLB_DEBUG)("I'm not Silent any more.=> DLB_NONE.\n");
      dlb_profile.tot_silent+=SILENT_PERIOD;
      dlb_silent_start=0;
      // log_event exit silent 

      dlb_stage = DLB_NONE;
      PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
    }
  }
    
  if (dlb_failure > FAILURE_MAX ) {
    PRINT_IF(DLB_DEBUG)("Too many failures(%ld), go to Silent.\n",dlb_failure);
    goToSilent();
    return;
  }
  if ( dlb_stage != DLB_NONE )
    return;
  if ( dlb_state == TASK_IMPORTED || dlb_state == TASK_EXPORTED)
    return;
  PRINT_IF(DLB_DEBUG)("Not in any specific state. Get status and test again.\n");
  dlb_profile.tot_tick++;
  if ( st<0)
    dlb_state =  getDLBStatus();
  if ( dlb_state == DLB_BUSY ) {
    PRINT_IF(DLB_DEBUG)("I am busy and finding an Idle.\n");
    findIdleNode();
  }
  else 
    if ( dlb_state == DLB_IDLE ) {
      PRINT_IF(DLB_DEBUG)("I am Idle and finding a Busy.\n");
      findBusyNode();
      PRINT_IF(DLB_DEBUG)("--------------\n");
    }
}
/*---------------------------------------------------------------*/
void engine::findBusyNode(){
  if ( dlb_stage == DLB_FINDING_BUSY ) 
    return;
  dlb_profile.tot_try++;
  dlb_stage = DLB_FINDING_BUSY ;
  dlb_node = getRandomNodeEx();    
  // log_event find_busy (dlb_node)
  PRINT_IF(DLB_DEBUG)("send FIND_BUSY to :%d\n",dlb_node);
  mailbox->send((byte*)&dlb_node,1,MailBox::FindBusyTag,dlb_node);
}
/*---------------------------------------------------------------*/
void engine::findIdleNode(){
  if ( dlb_stage == DLB_FINDING_IDLE ) 
    return;
  dlb_profile.tot_try++;
  dlb_stage = DLB_FINDING_IDLE ;
  dlb_node = getRandomNodeEx();    
  // log_event find_idle (dlb_node)
  PRINT_IF(DLB_DEBUG)("send FIND_IDLE to :%d\n",dlb_node);
  mailbox->send((byte*)&dlb_node,1,MailBox::FindIdleTag,dlb_node);
}
/*---------------------------------------------------------------*/
void engine::received_FIND_IDLE(int p){
  TIME_DLB;
  // log_event received find_idle
  if ( dlb_state == DLB_IDLE ) {
    acceptImportTasks(p);
    return;
  }
  PRINT_IF(DLB_DEBUG)("I'm not IDLE(%d) or not Silent(%d). Decline Import tasks.\n",dlb_state,dlb_stage);
  declineImportTasks(p);
}
void engine::receivedImportRequest(int p){ received_FIND_IDLE(p);}
/*---------------------------------------------------------------*/
void engine::received_FIND_BUSY(int p ) {
  TIME_DLB;
  // log_event received find_busy
  dlb_substate = DLB_NONE;
  if ( dlb_state != DLB_BUSY ){
    PRINT_IF(DLB_DEBUG)("I'm Idle and cannot export any task.\n");
    declineExportTasks(p);
    return;
  }
  if ( dlb_state == DLB_BUSY){
    PRINT_IF(DLB_DEBUG)("I'm BUSY  and already sent FIND_IDLE to %d(not %d).\n",dlb_node,p);
    if (dlb_node !=-1 && dlb_node != p){
      PRINT_IF(DLB_DEBUG)("Thus export tasks (EARLY_ACCEPT).\n");
      exportTasks(p);
      if(DLB_DEBUG)printf("%d\n",__LINE__);
      restartDLB();
      dlb_substate = EARLY_ACCEPT;
    }
  }
}
void engine::receivedExportRequest(int p){received_FIND_BUSY(p);}
/*---------------------------------------------------------------*/
void engine::received_DECLINE(int p){
  TIME_DLB;
  if(DLB_DEBUG)printf("decline from %d\n",p);
  // log_event received decline
  // log_event local failure
  dlb_failure ++;
  dlb_stage = DLB_NONE;
  PRINT_IF(DLB_DEBUG)("stage reset to NONE 1896.\n");
  if(DLB_DEBUG)printf("%d\n",__LINE__);
  restartDLB();    
}
void engine::receivedDeclineMigration(int p){received_DECLINE(p); }
/*---------------------------------------------------------------*/
IDuctteipTask *engine::selectTaskForExport(){
  list<IDuctteipTask*>::iterator it;
  for(it=running_tasks.begin();it != running_tasks.end();it++){
    IDuctteipTask *t = *it;
    if(0)printf(" %d,%d ",t->getState(),t->isExported());
    if (t->isExported()) continue;
    if (t->isFinished()) continue;
    if (t->isRunning()) continue;
    if (t->isUpgrading()) continue;
    if (t->canBeCleared()) continue;
    export_tasks.push_back(t);
    dt_log.logExportTask(export_tasks.size());

    if(DLB_DEBUG)printf("Task is moved from running list to export:\n");t->dump();
    running_tasks.erase(it);
    dt_log.logLoadChange(running_tasks.size());
    if(DLB_DEBUG)printf("RUNNING Tasks.:%ld\n",running_tasks.size());
    return t;
  }
  if(DLB_DEBUG)printf("\n");
  return NULL;
}
/*---------------------------------------------------------------*/
void engine::exportTasks(int p){
  if (DLB_DEBUG)printf("export tasks to %d\n",p);
  IDuctteipTask *t;
  do{
    if(DLB_DEBUG)printf("Find a task for export:\n");
    t=selectTaskForExport();
    if( t == NULL){
      if(DLB_DEBUG)printf("No task can be exported;\n");
      dlb_state = DLB_STATE_NONE;
      if(DLB_DEBUG)printf("%d\n",__LINE__);
      restartDLB();
      return;
    }
    dlb_profile.export_task++;
    dlb_state = TASK_EXPORTED;
    t->dump();
    t->setExported( true);
    // log_event export task (t)
    exportTask(t,p);
    list<DataAccess *> *data_list=t->getDataAccessList() ;
    list<DataAccess *> :: iterator it;
    for ( it = data_list->begin(); it != data_list->end()  ; it ++){
      IData * data = (*it)->data;
      if(DLB_DEBUG)printf("Data is exported:\n");data->dump('N');
      if ( data->isExportedTo(p) )
	continue;
      data->setExportedTo(p);
      dlb_profile.export_data++;

      void dumpData(double *,int,int,char);
      if ( 0 ) {
	double *A=(double*)(data->getHeaderAddress()+data->getHeaderSize());
	for ( int i=0;i<5;i++)
	  for (int j=0;j<5;j++){
	    dumpData(A+i*12*12+j*5*12*12,12,12,'x');
	  }
      }

      data->dumpCheckSum('x');
      data->dump(' ');
      data->serialize();
      if(0)printf("data-map:%p,%p,%d,%d,%d\n",data->getHeaderAddress(),
		  data->getContentAddress(),
		  data->getContentSize(),
		  data->getHeaderSize(),
		  data->getPackSize());

      mailbox->send(data->getHeaderAddress(),
		    data->getPackSize(),
		    MailBox::MigrateDataTag,
		    p);
    }
  }while( t !=NULL);
}
/*---------------------------------------------------------------*/
void engine::importData(MailBoxEvent *event){
  dlb_profile.import_data++;
  IData *data = importedData(event,event->getMemoryItem());
  if(0)printf("data-map:%p,%p,%d,%d,%d\n",data->getHeaderAddress(),
	      data->getContentAddress(),
	      data->getContentSize(),
	      data->getHeaderSize(),
	      data->getPackSize());
  data->dumpCheckSum('i');
  data->dump(' ');
  void dumpData(double *,int,int,char);
  if ( 0 ) {
    double *A=data->getContentAddress();
    for ( int i=0;i<5;i++)
      for (int j=0;j<5;j++){
	dumpData(A+i*12*12+j*5*12*12,12,12,'i');
      }
  }
  list<IDuctteipTask *>::iterator it;
  if(DLB_DEBUG)printf("Find which task can run:\n");
  for(it=import_tasks.begin();it != import_tasks.end(); it ++){
    IDuctteipTask *t = *it;
    if ( t->canRun('i')){
      if(DLB_DEBUG)printf("Task can run:\n");t->dump();
      t->run();	
    }
  }
}
/*---------------------------------------------------------------*/
void engine::receiveTaskOutData(MailBoxEvent *event){
  dlb_profile.import_data++;
  IData *data = importedData(event,event->getMemoryItem());
  data->dumpCheckSum('r');
  data->dump(' ');
  list<IDuctteipTask *>::iterator it;
  for(it=export_tasks.begin();it != export_tasks.end(); it ++){
    IDuctteipTask *task = *it;
    list<DataAccess *> *data_list=task->getDataAccessList() ;
    list<DataAccess *> :: iterator data_it;
    if(0)printf("check exported %s task's data:time=%ld\n",task->getName().c_str(),getTime());
    for ( data_it = data_list->begin(); data_it != data_list->end()  ; data_it ++){
      IData * t_data = (*data_it)->data;
      //printf("data:%s",t_data->getName().c_str());
      t_data->dump(' ');
      if ( (*data_it)->type == IData::WRITE ){
	if (*t_data->getDataHandle() == *data->getDataHandle()){
	  if (t_data->getRunTimeVersion(IData::WRITE) == data->getRunTimeVersion(IData::WRITE) ) {
	    task->upgradeData('r');
	    it = export_tasks.erase(it);
	    dt_log.logExportTask(import_tasks.size());
	    if(0)printf("rcvd task-out-data:%s,%s t_data:%s\n",
			task->getName().c_str(),
			data->getName().c_str(),
			t_data->getName().c_str());
	    task->setState(IDuctteipTask::CanBeCleared);
	    data->checkAfterUpgrade(running_tasks,mailbox,'r');
	    putWorkForCheckAllTasks();
	    return;
	  }
	}	
      }
    }
  }

}
/*---------------------------------------------------------------*/
void engine::receivedMigrateTask(MailBoxEvent *event){
  dlb_profile.import_task++;    
  importedTask(event);
  PRINT_IF(DLB_DEBUG)("received tasks from %d\n",event->host);
  dlb_stage = DLB_NONE;
  PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
  if (dlb_state == DLB_BUSY) {
    fprintf(stderr,"error : DLB_State == BUSY but received TASKS.\n");
  }
  if(DLB_DEBUG)printf("%d\n",__LINE__);
  restartDLB();     
}
/*---------------------------------------------------------------*/
void engine::received_ACCEPTED(int p){
  TIME_DLB;
  dlb_stage = DLB_NONE;
  PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
  // log_event accepted (from=p)
  if(DLB_DEBUG)printf("Node %d accepted my export-req.\n",p);
  if ( dlb_state == DLB_IDLE ) {
    fprintf(stderr,"error : DLB_State == IDLE  but received ACCEPTED.\n");
    return;
  }
  if ( dlb_substate == EARLY_ACCEPT ) {
    if(DLB_DEBUG)printf("I'm in EARLY ACCEPT.\n");
    if ( dlb_node == p ){
      fprintf(stderr,"%d\n",__LINE__);
      restartDLB();
      return;
    }      
    if(DLB_DEBUG)printf("But other node(%d and not %d) accepted me.\n",p,dlb_node);
  }
  exportTasks(p);
  if(DLB_DEBUG)    printf("%d\n",__LINE__);
  restartDLB();
}
/*---------------------------------------------------------------*/
void engine::declineImportTasks(int p){    
  dlb_glb_failure ++;
  PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
  mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void engine::declineExportTasks(int p){    
  dlb_glb_failure ++;
  PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
  mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void engine::acceptImportTasks(int p){    
  // log_event accept import  (p)
  dlb_substate = ACCEPTED;
  dlb_state = TASK_IMPORTED;
  PRINT_IF(DLB_DEBUG)("send Accept  to :%d\n",p);
  mailbox->send((byte*)&p,1,MailBox::AcceptMigrateTag,p);
}
/*---------------------------------------------------------------*/
bool engine::isInList(vector<int>&L,int v){
  vector<int>::iterator it;
  for (it=L.begin() ;it!=L.end();it++) {
    if (*it == v ) 
      return true;
  }
  return false;
}
int engine::getRandomNodeEx(){
  vector<int> &L=dlb_fail_list;
  vector<int>::iterator it;
  //    for (it=L.begin() ;it!=L.end();it++) printf(" %d ",*it);printf("<-\n");
  int np = net_comm->get_host_count();
  int p=me;
  if(0)printf("RandList size:%ld\n",L.size());
  if(L.size() > FAILURE_MAX || L.size() >= (np-1))
    L.clear();	
  while  ( true ){
    p = rand() % np ;
    if ( !isInList ( L, p) && p!= me )
      break;
  }    
  L.push_back(p);    
  //for (it=L.begin() ;it!=L.end();it++) printf(" %d ",*it);printf("->\n");    
  return p;
}
/*---------------------------------------------------------------*/
int engine::getRandomNodeOld(int exclude){
  srand(time(NULL));
  int np = net_comm->get_host_count();
  int p=me;
  while  ( p== me || p == exclude)
    p = rand() % np ;
  return p;
}
/*---------------------------------------------------------------------------------*/


void engine::receivePropagateTask(byte *buffer, int len){
  PropagateInfo *p = new PropagateInfo;
  int offset =0 ;
  p->deserialize(buffer,offset,len);
  addPropagateTask(p);
}
/*--------------------------------------------------------------*/
void engine::addPropagateTask(PropagateInfo *P){
  IDuctteipTask *task = new IDuctteipTask(P);
  string s("PROPTASK");
  task->setName(s);
  criticalSection(Enter) ;
  TaskHandle task_handle = last_task_handle ++;
  task->setHandle(task_handle);
  task_list.push_back(task);
  putWorkForPropagateTask(task);
  criticalSection(Leave) ;
}
void engine:: sendTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  mailbox->send(m->address,m->size,MailBox::TaskTag,destination);
  task->setCommHandle (ch);
}
/*--------------------------------------------------------------*/
void engine:: exportTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  mailbox->send(m->address,m->size,MailBox::MigrateTaskTag,destination);
  task->setCommHandle (ch);
  fprintf(stderr,"Task is exported:%s\n",task->getName().c_str());
  task->dump();
}
/*=====================================================================*/
void engine::receivedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;
  dh.deserialize(event->buffer,offset,event->length);

  LOG_INFO(LOG_MULTI_THREAD,"Data handle %d.\n",dh.data_handle);
  IData *data = glbCtx.getDataByHandle(&dh);
  offset =0 ;
  data->deserialize(event->buffer,offset,event->length,mi,all_content);
  LOG_INFO(LOG_MULTI_THREAD+LOG_DATA,"data:%s, cntnt:%p, ev.buf:%p\n",
	   data->getName().c_str(),data->getContentAddress(),event->buffer);

  putWorkForSingleDataReady(data);
  
  LOG_EVENT(DuctteipLog::DataReceived);
}
/*--------------------------------------------------------------------------*/
IData * engine::importedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;

  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }

  dh.deserialize(event->buffer,offset,event->length);
  IData *data = glbCtx.getDataByHandle(&dh);
  void dumpData(double *, int ,int, char);
  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }

  if ( 0 ) {
    printf("Buffer:%p Header size:%d\n",event->buffer,data->getHeaderSize());
    double *A=(double *)(event->buffer+data->getHeaderSize());
    for ( int i=0;i<5;i++)
      for (int j=0;j<5;j++){
	dumpData(A+i*12*12+j*5*12*12,12,12,'I');
      }
  }
  if(0)printf("ev.mem dump:\n   ");
  event->memory->dump();
  if(0)printf("data-before.mem dump:\n   ");
  mi = data->getDataMemory();
  if ( mi != NULL ) {
    mi->dump();
    memcpy(data->getContentAddress(),event->buffer+data->getHeaderSize(),data->getContentSize());
  }
  else{
    if(0)printf("NULL\n");
    data->setDataMemory(event->memory);
    data->setContentSize(event->length-data->getHeaderSize());
    mi = data->getDataMemory();
    data->prepareMemory();
  }
  if (0 ) {
    MemoryItem *mi2 = data->getDataMemory();
    printf("data-after.mem dump:\n   ");
    mi2->dump();
  }
  if ( mi == NULL) {
    fprintf(stderr,"preparememory.///////////////////////////////////////////////// \n");
    data->prepareMemory();
  }
  if(0)printf("imported data %s, mem:%p len:%d size:%d sz2:%d\n",data->getName().c_str(),
	      data->getContentAddress(),event->length,
	      data->getContentSize(),event->length-data->getHeaderSize());

  offset=0;
  data->deserialize(event->buffer,offset,event->length,mi,header_only);
  if(0)printf("Data is imported:\n");data->dump('N');
  return data;
}
/*--------------------------------------------------------------------------*/
void engine::start ( int argc , char **argv){
  int N,Nb,P,p,q,nb,nt,dlb,ipn;
  long st=100,sw=100,sm=1000,to=2000;
  p  = atoi(argv[1]);
  q  = atoi(argv[2]);
  N  = atoi(argv[3]);
  Nb = atoi(argv[4]);
  nb = atoi(argv[5]);
  nt = atoi(argv[6]);
  dlb = atoi(argv[7]);
  ipn= atoi(argv[8]);
  if ( argc > 9 ) 
    to = atol(argv[9]);
  if ( argc > 10 ) 
    sw = atoi(argv[10]);
  if ( argc > 11 ) 
    sm = atoi(argv[11]);    
  if ( argc > 12 ) 
    st = atoi(argv[12]);    
  if ( dlb ==-1){
    simulation = 1; 
    dlb=0;
  }
  P = p * q;
  LOG_INFO(LOG_CONFIG,"ipn:%d, P:%d,p:%d,q:%d\n",ipn,P,p,q);
  LOG_INFO(LOG_CONFIG,"N:%d, Nb:%d,nb:%d,nt:%d dlb:%d\n",N,Nb,nb,nt,dlb);

  
  

  //  config.setParams(N,N,Nb,Nb,P,p,q,nb,nb,nt,dlb,ipn);
  config.getCmdLine(argc,argv);
  P =config.P;
  p =config.p;
  q =config.q;
  to=config.to;
  setSkips(st,sw,sm,to);
  LOG_INFO(LOG_CONFIG,"ipn:%d, P:%d,p:%d,q:%d\n",ipn,P,p,q);
  LOG_INFO(LOG_CONFIG,"N:%d, Nb:%d,nb:%d,nt:%d dlb:%d\n",N,Nb,nb,nt,dlb);
  
  ProcessGrid *_PG = new ProcessGrid(P,p,q);
  ProcessGrid &PG=*_PG;

  DataHostPolicy      *hpData    = new DataHostPolicy    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy      *hpTask    = new TaskHostPolicy    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy   *hpContext = new ContextHostPolicy (ContextHostPolicy::ALL_ENTER         , PG );
  TaskReadPolicy      *hpTaskRead= new TaskReadPolicy    (TaskReadPolicy::ALL_READ_ALL         , PG );
  TaskAddPolicy       *hpTaskAdd = new TaskAddPolicy     (TaskAddPolicy::WRITE_DATA_OWNER      , PG );
  TaskPropagatePolicy *hpTaskProp=new TaskPropagatePolicy(TaskPropagatePolicy::GROUP_LEADER    , PG );

  hpContext->setGroupCount(1,2,1);  // all processors allowed for first level,
  // divide them by 2 for second level
  // all (from previous level) for third level
  glbCtx.setPolicies(hpData,hpTask,hpContext,hpTaskRead,hpTaskAdd,hpTaskProp);
  glbCtx.setConfiguration(&config);
  setConfig(&config);
  glbCtx.doPropagation(false);
  LOG_INFO(LOG_CONFIG,"ipn:%d, P:%d,p:%d,q:%d\n",ipn,P,p,q);
  LOG_INFO(LOG_CONFIG,"N:%d, Nb:%d,nb:%d,nt:%d dlb:%d\n",N,Nb,nb,nt,dlb);
  
  doProcess();
}

void engine::setThreadModel(int m){
  thread_model=m;
}
int  engine::getThreadModel(){
  return thread_model;
}
MemoryManager *engine::getMemoryManager(){return data_memory;}
