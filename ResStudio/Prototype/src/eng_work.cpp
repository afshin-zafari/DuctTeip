#include "engine.hpp"
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
  LOG_INFO(LOG_MULTI_THREAD,"work:%d, tag:%d, item:%d\n",work->event,work->tag,work->item);
  criticalSection(Leave);
  LOG_INFO(LOG_MLEVEL,"\n");
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
void engine::putWorkForDataReady(list<DataAccess *> *data_list){
  list<DataAccess *>::iterator it;
  for (it =data_list->begin(); it != data_list->end() ; it ++) {
    IData *data=(*it)->data;
    putWorkForSingleDataReady(data);
  }
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForCheckAllTasks(){
  list<IDuctteipTask *>::iterator it;
  it = task_list.begin();
  for(; it != task_list.end(); ){
    IDuctteipTask *task = (*it);
    if (task->canBeCleared()){
      it=task_list.erase(it);
      if(1)printf("TL:%ld\n",task_list.size());
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
  criticalSection(Enter);
  work_queue.push_back(work);
  LOG_INFO(LOG_MULTI_THREAD+1,"%s\n",task->getName().c_str());
  criticalSection(Leave);
}
/*---------------------------------------------------------------------------------*/
void engine::putWorkForSingleDataReady(IData* data){
  DuctTeipWork *work = new DuctTeipWork;
  work->data = data;
  work->tag   = DuctTeipWork::DataWork;
  work->event = DuctTeipWork::DataUpgraded;
  work->item  = DuctTeipWork::CheckAfterDataUpgraded;
  work_queue.push_back(work);
  LOG_INFO(LOG_DATA,"Put DataUpgrade work for:%s.\n",data->getName().c_str());
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
      //TaskHandle task_handle =      work->task->getHandle();
      LOG_EVENT(DuctteipLog::CheckedForRun);
      if ( work->task->canRun('W') ) {
	running_tasks.push_back(work->task);
	LOG_LOAD;
	if (cfg->getDLB()){
	  long actives=dlb.getActiveTasksCount();
	  LOG_INFO(LOG_DLBX,"\n");
	  if ( actives< dlb.DLB_BUSY_TASKS ){
	    LOG_INFO(LOG_DLBX,"Let task run, since there is not enough active tasks:%ld\n",actives);
	    work->task->run();
	  }
	}
	else{
	  LOG_INFO(LOG_MULTI_THREAD,"Task can run.\n");
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
    LOG_INFO(LOG_MULTI_THREAD,"Data %s Upgrade Work, Check Listeners cnt:%d .\n",work->data->getName().c_str(),d_listeners.size());
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
	  LOG_INFO(1,"Data Upgrade Work, Task run.\n");
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

