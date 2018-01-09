#include "engine.hpp"
/*---------------------------------------------------------------------------------*/
int  engine::getTaskCount(){    return task_list.size();  }
/*---------------------------------------------------------------------------------*/
IDuctteipTask *engine::getTask(TaskHandle th)
{
  list<IDuctteipTask *>::iterator it;
  for ( it = task_list.begin(); it !=task_list.end();it++){
    IDuctteipTask *t = *it;
    assert(t);
    if ( t->getHandle() == th ) 
      return t;
  }
  return NULL;
}
/*---------------------------------------------------------------------------------*/
void engine::register_task(IDuctteipTask *task)
{
  if ( task == nullptr ){
    LOG_INFO(LOG_TASKS,"Null pointer of a new task is passed in.\n");
    return ;
  }
  criticalSection(Enter) ;
  TaskHandle task_handle = last_task_handle ++;
  if(last_task_handle ==1){
    //    if (me == 0)
    printf("First task submitted at pureTime %ld.\n",getTime());
    printf("First task submitted at UserTime %ld.\n",UserTime());
    dt_log.addEventStart(this,DuctteipLog::ProgramExecution);
    assert(net_comm);
    net_comm->barrier();
    LOG_INFO(LOG_MULTI_THREAD,"First task is submitted.\n");
  }
  task->setHandle(task_handle);
  LOG_METRIC(DuctteipLog::TaskDefined);
  if (task->getHost() != me ) {
    if(0)putWorkForSendingTask(task);
    LOG_INFO(0*LOG_TASKS,"New task not added, its host is:%d.\n",task->getHost() );
  }
  else {
    task_list.push_back(task);
    LOG_INFO(0*LOG_TASKS,"New task added, total count :%d.\n",task_list.size());
    putWorkForNewTask(task);
  }
  criticalSection(Leave) ;
  signalWorkReady();  
  //doProcessWorks();  doProcessWorks();  doProcessMailBox();
  if ( !runMultiThread ) {
    doProcessMailBox();
    doProcessWorks();
  }
}

/*---------------------------------------------------------------------------------*/
TaskHandle  engine::addTask(IContext * context,
			    string task_name,
			    unsigned long key,
			    int task_host,
			    list<DataAccess *> *data_access)
{
  IDuctteipTask *task = new IDuctteipTask (context,task_name,key,task_host,data_access);
  assert(task);
  criticalSection(Enter) ;
  task->child_count = 0;
  TaskHandle task_handle = last_task_handle ++;
  if(last_task_handle ==1){
    LOG_INFO(LOG_MULTI_THREAD,"First task is submitted.\n");

    /*
#if POST_RECV_DATA == 12
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
    */
    dt_log.addEventStart(this,DuctteipLog::ProgramExecution);
    if (me == 0)printf("First task submitted at %ld.\n",UserTime());
    dt_log.addEventStart(this,DuctteipLog::ProgramExecution);
    assert(net_comm);
    net_comm->barrier();
    LOG_INFO(LOG_MULTI_THREAD,"First task is submitted.\n");
  }
  task->setHandle(task_handle);
  LOG_METRIC(DuctteipLog::TaskDefined);
  task_list.push_back(task);
  LOG_INFO(LOG_TASKS,"DTTask list size:%d\n",task_list.size());
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
/*--------------------------------------------------------------*/
void engine:: sendTask(IDuctteipTask* task,int destination){
  assert(task);
  assert(dtEngine.mailbox);
  MessageBuffer *m = task->serialize();
  unsigned long ch =  dtEngine.mailbox->send(m->address,m->size,MailBox::TaskTag,destination);
  task->setCommHandle (ch);
}

/*---------------------------------------------------------------------------------*/
IDuctteipTask *engine::getTaskByHandle(TaskHandle  task_handle){
  list<IDuctteipTask *>::iterator it;
  criticalSection(Enter);
  for ( it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask *task = (*it);
    assert(task);
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
    assert(task);
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
void  engine::checkMigratedTasks(){
  dlb.checkImportedTasks();
  dlb.checkExportedTasks();
}
/*---------------------------------------------------------------------------------*/
void engine::checkTaskDependencies(IDuctteipTask *task){
  assert(task);
  list<DataAccess *> *data_list= task->getDataAccessList();
  list<DataAccess *>::iterator it;
  for (it = data_list->begin(); it != data_list->end() ; it ++) {
    DataAccess &data_access = *(*it);
    LOG_INFO(0*LOG_MLEVEL,"parent data:%p hpData:%p.\n",
	     data_access.data->getParentData(),
	     data_access.data->getDataHostPolicy());

    int host;
    assert(data_access.data);
    host = data_access.data->getHost();
    data_access.data->addTask(task);
    LOG_INFO(0*LOG_MLEVEL,"host:%d\n",host);
    if ( host != me ) {
      IListener * lsnr = new IListener((*it),host);
      assert(lsnr);      
      lsnr->setDataRequest(*it);      
      bool not_duplicate = addListener(lsnr);
      assert(lsnr->getData());
      LOG_INFO(LOG_LISTENERS,"(****)Task:%s,Listener  for %s ver %s is created and sent to host:%d.\n",
	       task->getName().c_str(),
	       lsnr->getData()->getName().c_str(),
	       lsnr->getRequiredVersion().dumpString().c_str(),
	       host
	       );
      LOG_INFO(LOG_LISTENERS,"(****)Daxs %s for  %s is %p\n",
	       task->getName().c_str(),
	       data_access.data->getName().c_str(),*it);
      if (1 or not_duplicate  ){
	MessageBuffer *m=lsnr->serialize();
	assert(m);
	unsigned long comm_handle = mailbox->send(m->address,m->size,MailBox::ListenerTag,host);
	LOG_EVENT(DuctteipLog::ListenerSent);
	lsnr->setCommHandle ( comm_handle);
	LOG_INFO(LOG_LISTENERS,"Listener is sent with comm-handle:%ld\n",comm_handle);
	IData *data=data_access.data;//lsnr->getData();
	assert(data);
	data->allocateMemory();
	data->prepareMemory();
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
      LOG_INFO(LOG_DLB_SMART,"run an exported task:%ld\n",task->getHandle());
      list<IDuctteipTask *>::iterator itex;
      for ( itex = export_tasks.begin();itex != export_tasks.end();itex++){
	IDuctteipTask *t=(*itex);
	if ( t->getHandle() == task->getHandle()){
	  LOG_INFO(LOG_DLB_SMART,"remove form export list:%ld\n",t->getHandle());
	  export_tasks.erase(itex);
	  return;
	}
      }
    }
    return;
  }
}

/*---------------------------------------------------------------------------------*/
void engine::removeTaskByHandle(TaskHandle task_handle){
  list<IDuctteipTask *>::iterator it;
  criticalSection(Enter);
  for ( it = task_list.begin(); it != task_list.end(); it ++){
    IDuctteipTask *task = (*it);
    assert(task);
    if (task->getHandle() == task_handle){
      task_list.erase(it);
      if(0)printf("TL:%ld\n",task_list.size());
      criticalSection(Leave);
      return;
    }
  }
}
/*---------------------------------------------------------------------------------*/
long int engine::checkRunningTasks(int v){

  list<IDuctteipTask *>::iterator it;
  int cnt =0 ;
  TimeUnit t= getTime();
  for(it = running_tasks.begin(); it != running_tasks.end(); ){
    IDuctteipTask *task=(*it);
    assert(task);
    if(task->canBeCleared()){
      updateDurations(task);
      it = running_tasks.erase(it);
      LOG_INFO(LOG_MLEVEL,"(++++)task finished:%s \n",task->getName().c_str());
      //delete task;
      LOG_LOAD;
      continue;
    }
    if (task->isFinished() ) {
      task->setState(IDuctteipTask::UpgradingData);
      //task->finished();
      putWorkForFinishedTask(task);
      //task->upgradeData('w');
      LOG_INFO(LOG_MLEVEL,"task finished:%s, run-list size:%ld\n",task->get_name().c_str(),running_tasks.size());
      cnt ++;
      it ++;
    }
    else
      it++;
  }
  if ( config.getDLB() )
    checkMigratedTasks();
  t = getTime() - t;

  return cnt+import_tasks.size()+export_tasks.size();
}
/*---------------------------------------------------------------------------------*/
long engine::getUnfinishedTasks(){
  list<IDuctteipTask *>::iterator it;
  TimeUnit t = getTime();
  it = task_list.begin();
  for(; it != task_list.end(); ){
    IDuctteipTask * task = (*it);
    assert(task);
    if (task_list.size() <=3 ){
      LOG_INFO(0*LOG_MLEVEL,"task list remaining, n:%ld,%s\n",task_list.size(),task->get_name().c_str());
    }
    if (task->canBeCleared()){
      checkRunningTasks();
      it=task_list.erase(it);      
      LOG_INFO(LOG_MLEVEL,"(++++)task removed, %s\n",task->get_name().c_str());
      //      delete task;
      if ( task_list.size() == 1 ){
	LOG_INFO(0*LOG_MLEVEL,"task list remaining, last task:%s\n",(*it)->get_name().c_str());
      }
    }
    else
      it ++;
  }
  wasted_time += getTime() - t;
  //  LOG_INFO(LOG_MULTI_THREAD,"Non-Busy time:%ld\n",wasted_time);
  return task_list.size();
}

/*---------------------------------------------------------------------------------*/
void engine::waitForTaskFinish(){
  static TimeUnit st=0,idle = 0;
  int *c=glbCtx.getCounters();
  //  if ( c[GlobalContext::TaskInsert] ==0 )    return;

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
