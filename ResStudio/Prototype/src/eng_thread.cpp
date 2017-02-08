#include "engine.hpp"
SuperGlue<Options> * engine::getThrdManager() {return thread_manager;}
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
void engine::signalWorkReady(IDuctteipTask * task){
  return;
  pthread_mutex_lock(&work_ready_mx);
  pthread_cond_signal(&work_ready_cv);
  pthread_mutex_unlock(&work_ready_mx);
  if ( thread_model<1){
    criticalSection(Enter);
    if(task)
      if ( task->isFinished()){
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
void engine::doSelfTerminate(){
  byte  dummy;
  mailbox->send(&dummy,1,MailBox::SelfTerminate,me,false);
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

/*---------------------------------------------------------------*/
void engine::setThreadModel(int m){
  thread_model=m;
}
/*---------------------------------------------------------------*/
int  engine::getThreadModel(){
  return thread_model;
}

