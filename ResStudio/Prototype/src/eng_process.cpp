#include "engine.hpp"
/*---------------------------------------------------------------------------------*/
bool engine::canTerminate(){
  static int once=0;
  TimeUnit t =getTime();
  {

    LOG_EVENT(DuctteipLog::CheckedForTerminate);

    if ((signed)elapsedTime(TIME_SCALE_TO_SECONDS)>(signed)time_out){
      if (checkRunningTasks(1) >0 || work_queue.size() >0){
	wasted_time += getTime() - t;
	return false;
      }
      TimeUnit t = elapsedTime(TIME_SCALE_TO_SECONDS);
      LOG_ERROR("error: timeout %ld\n",t);
      LOG_INFO(LOG_MULTI_THREAD,"unf task:%ld %ld  unf-lsnr:%d task_submission_finished:%d\n",
	       task_list.size(),getUnfinishedTasks(),isAnyUnfinishedListener(),task_submission_finished);
      for(auto t: task_list){
	LOG_INFO(LOG_MULTI_THREAD,"remaining task:%s \n",t->getName().c_str());
      }
      for(auto L : listener_list){
	LOG_INFO(LOG_MULTI_THREAD,"remaining listener for data :%s ver:%s, its data sent?:%d, isRecvd?:%d\n",
		 L->getData()->getName().c_str(), L->getRequiredVersion().dumpString().c_str(),L->isDataSent(),L->isReceived());
      }
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
  //  LOG_INFO(LOG_MLEVEL,"task list size:%ld\n",task_list.size());
  wasted_time += getTime() - t;
  return false;
}
/*---------------------------------------------------------------------------------*/
void engine::doProcess(){

  LOG_LOADZ;
  if ( runMultiThread ) {
    sched_param sp;
    //int r1,r2;
    sp.sched_priority=1;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_setschedparam(&attr,&sp);
    pthread_create(&thread_id,&attr,engine::doProcessLoop,this);
    pthread_mutex_init(&thread_lock,&mutex_attr);
    if (thread_model >=1){
      int err=pthread_create(&mbsend_tid,&attr,engine::runMBSendThread,this);
      LOG_INFO(LOG_MULTI_THREAD,"MBSendThrdID:%X, Err:%d\n",(uint)mbsend_tid,err);
      (void)err;
    }
    if (thread_model >=2){
      int err=pthread_create(&mbrecv_tid,&attr,engine::runMBRecvThread,this);
      (void)err;
      LOG_INFO(LOG_MULTI_THREAD,"MBRecvThrdID:%X, Err:%d\n",(uint)mbrecv_tid,err);
    }
  }
  else{
    fprintf(stderr,"single thread run\n");
  }
}
/*---------------------------------------------------------------------------------*/
void *engine::doProcessLoop(void *p){
  engine *_this = (engine *)p;
  unsigned long loop=0;(void)loop;
  TimeUnit tot=0,t,dt=0,pw=0,mb=0,tf=0,ct=0;
  LOG_INFO(LOG_MULTI_THREAD, "do process loop started\n");
  while(true){
    t=getTime();
    if ( _this->getThreadModel() <1){
      dt = getTime();
      //      LOG_INFO(LOG_MULTI_THREAD, "MailBox check\n");
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
  //if (0 != usleep(config.ps))loop++;
  }
  LOG_INFO(LOG_MULTI_THREAD, "do process loop finished, tot-time=%lf, loop-cnt:%ld, sleep-time=%lf\n",
	   tot/1000000.0,loop,config.ps *loop/1000000.0);
  LOG_INFO(LOG_MULTI_THREAD,"prc-wrk:%ld, mail-box:%ld, task-finish:%ld, terminate:%ld, total=%lf\n",pw,mb,tf,ct,(pw+mb+tf+ct)/1000000.0);
  pthread_exit(NULL);
}
/*---------------------------------------------------------------------------------*/
void engine::doProcessMailBox(){
  MailBoxEvent event ;
  TimeUnit t = getTime();
  bool wait= false,completed,found;
  static bool once=false;
  if (net_comm->get_host_count() == 1)
    return ;
  int *counters = glbCtx.getCounters();
  //  if ( counters[GlobalContext::TaskInsert] == 0)    return;
  if(!once){
    once = true;
#if POST_RECV_DATA == 1
    DataHandle dh;
    DataVersion dv;
    IDuctteipTask z;
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
    //    net_comm->postReceiveData(P,dps,data_memory);
#endif
  }
  do {
    event.host =-1;
    event.tag = -1;
    LOG_EVENT(DuctteipLog::MailboxGetEvent);
    found =  mailbox->getEvent(data_memory,&event,&completed,wait);

    if ( !found ){
      TimeUnit tt=getTime();
      wasted_time += tt - t;
      ///LOG_INFO(LOG_MULTI_THREAD,"No comm happened, wt:%ld,tot-wt:%ld\n",tt-t,wasted_time);
      return ;
    }
    
	LOG_INFO(LOG_MULTI_THREAD,"comp:%d,found:%d,tag:%d,dir:%d\n",
	completed, found,event.tag,event.direction);
    
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
      LOG_INFO(LOG_LISTENERS,"received listener\n");
      receivedListener(&event);
    }
    else {
      /*
      IListener *listener = getListenerByCommHandle(event.handle);
      if (listener){
	LOG_EVENT(DuctteipLog::ListenerSendCompleted);
      }
      */
    }
    break;
  case MailBox::DataTag:
#ifdef UAMD_COMM
  case MailBox::UAMDataTag:
#endif
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
	putWorkForSingleDataReady(listener->getData());
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
    case MailBox::PropagationTag:
    case MailBox::SelfTerminate:
        break;
  }
  if ( event.direction  == MailBoxEvent::Received) {
    if (event.tag != MailBox::DataTag) {
    }
  }
}
