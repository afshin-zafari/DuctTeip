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
  dlb.initDLB();
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
SuperGlue<Options> * engine::getThrdManager() {return thread_manager;}
int engine::getLocalNumBlocks(){return local_nb;}
/*---------------------------------------------------------------------------------*/
ulong engine::getDataPackSize(){return dps;}
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

    dps = ny * nx * sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
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
  
  static TimeUnit st=0,idle = 0;
  int *c=glbCtx.getCounters();
  if ( c[GlobalContext::TaskInsert] ==0 )
    return;
  
  if ( running_tasks.size() <1 ){
    if ( st ==0)
      st= getTime();
  }
  else{
    if ( st != 0 ){
      idle += getTime() - st;
      LOG_INFO(LOG_MULTI_THREAD,"idle time:%lf\n",idle/1000000.0);
    }
    st=0;
  }
    
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
    dlb.dumpDLB();
}
/*---------------------------------------------------------------------------------*/
void engine::globalSync(){
  LOG_LOADZ;

  LOG_INFO(LOG_MULTI_THREAD,"Non-Busy time:%lf\n",wasted_time/1000000.0);
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
      TimeUnit t = elapsedTime(TIME_SCALE_TO_SECONDS);
      LOG_ERROR("error: timeout %ld\n",t);
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
  dlb.initDLB();
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

  LOG_LOADZ;
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
    if (0 != usleep(config.ps))loop++;
  }
  LOG_INFO(LOG_MULTI_THREAD, "do process loop finished, tot-time=%lf, loop-cnt:%ld, sleep-time=%lf\n",
	   tot/1000000.0,loop,config.ps *loop/1000000.0);
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

/*--------------------------------------------------------------*/
void engine:: sendTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  dtEngine.mailbox->send(m->address,m->size,MailBox::TaskTag,destination);
  task->setCommHandle (ch);
}
/*--------------------------------------------------------------*/
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

  dtEngine.putWorkForSingleDataReady(data);
  
  LOG_EVENT(DuctteipLog::DataReceived);
}

/*---------------------------------------------------------------------------------*/
void engine::updateDurations(IDuctteipTask *task){
  long k = task->getKey();
  double dur = task->getDuration()/1000000.0;
  avg_durations[k]=  (avg_durations[k] * cnt_durations[k]+dur)/(cnt_durations[k]+1);
  cnt_durations[k]=cnt_durations[k]+1;
  LOG_INFO(LOG_DLB,"t-key:%ld, t-dur:%.2lf, avg:%.2lf, cnt:%d\n",
	   k,dur,avg_durations[k],cnt_durations[k]);
}
/*---------------------------------------------------------------------------------*/
long engine::getAverageDur(long k){
  return avg_durations[k];
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
      updateDurations(task);
      it = running_tasks.erase(it);	
      LOG_INFO(LOG_MLEVEL,"task finished:%s \n",task->getName().c_str());
      LOG_LOAD;
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
  dlb.checkImportedTasks();
  dlb.checkExportedTasks();
}
/*---------------------------------------------------------------------------------*/
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
    LOG_INFO(LOG_MLEVEL,"parent data:%p hpData:%p.\n",
	     data_access.data->getParentData(),
	     data_access.data->getDataHostPolicy());

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
  if(dlb.getActiveTasksCount()>dlb.DLB_BUSY_TASKS){
    doDLB();
    return;
  }
  list<IDuctteipTask *>::iterator it;
  for(it= running_tasks.begin(); it != running_tasks.end(); it ++){
    IDuctteipTask *task = *it;
    //if (task->isExported())continue;DLB_SMART
    if(task->isRunning()) continue;
    if(task->isFinished()) continue;
    if(task->canBeCleared()) continue;
    if(task->isUpgrading()) continue;
    task->run();
    if ( task->isExported()){
      LOG_INFO(LOG_DLB_SMART,"run an exported task:%d\n",task->getHandle());
      list<IDuctteipTask *>::iterator itex;
      for ( itex = export_tasks.begin();itex != export_tasks.end();itex++){
	IDuctteipTask *t=(*itex);
	if ( t->getHandle() == task->getHandle()){
	  LOG_INFO(LOG_DLB_SMART,"remove form export list:%d\n",t->getHandle());
	  export_tasks.erase(itex);
	  return;
	}
      }      
    }
    return;
  }
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
	LOG_LOAD;
	if (cfg->getDLB()){
	  long actives=dlb.getActiveTasksCount();
	  LOG_INFO(LOG_DLBX,"\n");
	  if ( actives< dlb.DLB_BUSY_TASKS ){
	    LOG_INFO(LOG_DLBX,"Let task run, since there is not enough active tasks:%d\n",actives);
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
    //LOG_INFO(LOG_MULTI_THREAD,"Data Upgrade Work, Listener Check.\n");
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
	LOG_LOAD;
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
  do {
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
  }while(found);
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
  case MailBox::TerminateOKTag:
    receivedTerminateOK(event.host);
    break;
  case MailBox::TerminateCancelTag:
    receivedTerminateCancel(event.host);
    break;
  case MailBox::FindIdleTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"FIND_IDLE tag recvd from %d\n",event.host);
      dlb.received_FIND_IDLE(event.host);
    }
    break;
  case MailBox::FindBusyTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"FIND_BUSY tag recvd from %d\n",event.host);
      dlb.received_FIND_BUSY(event.host);
    }
    break;
  case MailBox::DeclineMigrateTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB," DECLINE  tag recvd from %d\n",event.host);
      dlb.receivedDeclineMigration(event.host);
    }
    break;
  case MailBox::AcceptMigrateTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"ACCEPT  tag recvd from %d\n",event.host);
      dlb.received_ACCEPTED(event.host);
    }
    break;
  case MailBox::MigrateTaskTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"TASKS imported from %d\n",event.host);
      dlb.receivedMigrateTask(&event);
    }
    break;
  case MailBox::MigrateDataTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"DATA imported from %d\n",event.host);
      dlb.importData(&event);
    }
    break;
  case MailBox::MigratedTaskOutDataTag:
    if ( event.direction == MailBoxEvent::Received ) {
      LOG_INFO(LOG_DLB,"result for exported task is imported from %d\n",event.host);
      dlb.receiveTaskOutData(&event);
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
/*---------------------------------------------------------------*/
void engine::receivedTerminateCancel(int from){}
/*---------------------------------------------------------------*/
void engine::sendTerminateCancel(){}
/*--------------------------------------------------------------------------*/
void engine::start ( int argc , char **argv){
  int P,p,q;

  config.getCmdLine(argc,argv);
  P =config.P;
  p =config.p;
  q =config.q;  
  time_out = config.to;

  ProcessGrid *_PG = new ProcessGrid(P,p,q);
  ProcessGrid &PG=*_PG;

  DataHostPolicy      *hpData    = new DataHostPolicy    (DataHostPolicy::BLOCK_CYCLIC         , PG );
  TaskHostPolicy      *hpTask    = new TaskHostPolicy    (TaskHostPolicy::WRITE_DATA_OWNER     , PG );
  ContextHostPolicy   *hpContext = new ContextHostPolicy (ContextHostPolicy::ALL_ENTER         , PG );
  TaskReadPolicy      *hpTaskRead= new TaskReadPolicy    (TaskReadPolicy::ALL_READ_ALL         , PG );
  TaskAddPolicy       *hpTaskAdd = new TaskAddPolicy     (TaskAddPolicy::WRITE_DATA_OWNER      , PG );


  glbCtx.setPolicies(hpData,hpTask,hpContext,hpTaskRead,hpTaskAdd);
  glbCtx.setConfiguration(&config);
  setConfig(&config);
  doProcess();
}
/*---------------------------------------------------------------*/
void engine::setThreadModel(int m){
  thread_model=m;
}
/*---------------------------------------------------------------*/
int  engine::getThreadModel(){
  return thread_model;
}
/*---------------------------------------------------------------*/
MemoryManager *engine::getMemoryManager(){return data_memory;}
/*---------------------------------------------------------------*/
void engine::doDLB(int st){
  static long last_count;
  long c =  running_tasks.size();
  //  if ( c && c!= last_count )    return;
  LOG_INFO(LOG_DLB,"task count last:%ld, cur:%d\n",last_count ,c);
  last_count = c;
  dlb.doDLB(st);
}

