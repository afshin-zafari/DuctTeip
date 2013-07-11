#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

typedef unsigned char byte;
#include <pthread.h>
#include <sched.h>
#include "task.hpp"
#include "listener.hpp"
#include "mailbox.hpp"
#include "mpi_comm.hpp"
#include "simulate_comm.hpp"
extern int me;


struct DuctTeipWork{
public:
  enum WorkTag{
    TaskWork,
    ListenerWork,
    DataWork
  };
  enum WorkEvent{
    Ready,
    Sent,
    Finished,
    DataSent,
    DataReceived,
    DataUpgraded,
    DataReady,
    Received,
    Added
  };
  enum TaskState{
    Initialized,
    WaitForData,
    Running
  };
  enum WorkItem{
    CheckTaskForData,
    CheckTaskForRun,
    SendTask,
    CheckListenerForData,
    CheckAfterDataUpgraded,
    SendListenerData
  };
  ITask *task;
  IData *data;
  IListener *listener;
  int state,tag,event,host,item;
  //  DuctTeipWork(){ }
  DuctTeipWork operator =(DuctTeipWork *_work){
    task     = _work->task;
    data     = _work->data;
    listener = _work->listener;
    state    = _work->state;
    item     = _work->item;
    tag      = _work->tag;
    event    = _work->event;
    host     = _work->host;
  }
  void dump(){
    printf("work dump: tag:%d , item:%d\n",tag,item);
    printf("work dump:  ev:%d , stat:%d\n",event,state);
    printf("work dump: host:%d\n",tag,item);
  }
};
class engine
{
private:
  list<ITask*>  task_list;
  byte           *prop_buffer;
  int             prop_buffer_length;
  MailBox        *mailbox;
  INetwork         *net_comm;
  list<DuctTeipWork *> work_queue;
  pthread_t thread_id;
  pthread_mutex_t thread_lock;
  pthread_mutexattr_t mutex_attr;
  long last_task_handle,last_listener_handle;
  list<IListener *> listener_list;
  enum { Enter, Leave};
public:
  engine(){
    net_comm = new MPIComm;
    mailbox = new MailBox(net_comm);
    me = net_comm->get_host_id();
    last_task_handle = 0;
  }
  ~engine(){
    delete net_comm;
  }

  TaskHandle  addTask(string task_name, int task_host, list<DataAccess *> *data_access){
    ITask *task = new ITask (task_name,task_host,data_access);
    TRACE_LOCATION;
    criticalSection(Enter) ;//pthread_mutex_lock(&thread_lock);
    TRACE_LOCATION;
    TaskHandle task_handle = last_task_handle ++;
    task->setHandle(task_handle);
    task_list.push_back(task);
    TRACE_LOCATION;
    if (task_host != me ) {
      putWorkForSendingTask(task);
    }
    else {
      TRACE_LOCATION;
      putWorkForNewTask(task);
    }
    TRACE_LOCATION;
    criticalSection(Leave) ;
    if (!net_comm->canMultiThread()) {
      doProcessMailBox();
      doProcessWorks();
    }
    TRACE_LOCATION;
    return task_handle;
  }
  void dumpTasks(){
    list<ITask *>::iterator it;
    for (  it =task_list.begin();  it!= task_list.end(); it ++) {
      ITask &t = *(*it);
      t.dump();
    }
  }
  void sendPropagateTask(byte *msg_buffer, int msg_size, int dest_host) {//ToDo
    prop_buffer = msg_buffer;
    prop_buffer_length = msg_size;
  }
  void addPropagateTask(void *P){
  }
  void receivePropagateTask(byte **buffer, int *length){//ToDo
    *buffer  = prop_buffer ;
    *length = prop_buffer_length ;
  }
  void sendTask(ITask* task,int destination);
  void sendListener(){} //ToDo: engine.sendListener
  void sendData() {}//ToDo: engine.sendData
  void receivedListener(MailBoxEvent *event){

    IListener *listener = new IListener;
    int offset = 0 ;
    TRACE_LOCATION;
    listener->deserialize(event->buffer,offset,event->length);
    TRACE_LOCATION;
    listener->setHost ( me ) ;
    listener->setSource ( event->host ) ;
    listener->setReceived(true);
    TRACE_LOCATION;


    criticalSection(Enter); //pthread_mutex_lock(&thread_lock);
    TRACE_LOCATION;
    listener->setHandle( last_listener_handle ++);
    listener_list.push_back(listener);
    putWorkForReceivedListener(listener);
    criticalSection(Leave); 

  }
  void receivedData(MailBoxEvent *event);
  void finalize(){
    if ( net_comm->canMultiThread() ) {
      pthread_mutex_destroy(&thread_lock);
      pthread_mutexattr_destroy(&mutex_attr);
      pthread_join(thread_id,NULL);
    }
    else
      doProcessLoop((void *)this);
    globalSync();
  }
  void globalSync(){
    net_comm->finish();
  }
  bool canTerminate(){
    if (!net_comm->canMultiThread())
      if (task_list.size() < 1){
	return true;
      }
      else ;
    else{//Todo
      if (last_task_handle > 0 && task_list.size() < 1)
	return true;
    }
    return false;
  }
  int  getTaskCount(){
    return task_list.size();
  }
  void doProcess(){

    if (net_comm->canMultiThread()) {
      pthread_mutexattr_init(&mutex_attr);
      pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);
      pthread_create(&thread_id,NULL,engine::doProcessLoop,this);
      pthread_mutex_init(&thread_lock,&mutex_attr);
      printf("multiple  thread run\n");
    }
    else{
      printf("single thread run\n");
    }
  }
  static 
  void *doProcessLoop(void *p){
    engine *_this = (engine *)p;
    while(true){
      _this->doProcessMailBox();
      _this->doProcessWorks();
      if ( _this->canTerminate() )
	break;
    }
  }
private :
  void putWorkForCheckAllTasks(){
    list<ITask *>::iterator it;
    for(it = task_list.begin(); it != task_list.end(); it ++){
      ITask * task = (*it);
      if (task->getHost() != me ) continue;
      DuctTeipWork *work = new DuctTeipWork;
      work->task  = task;
      work->tag   = DuctTeipWork::TaskWork;
      work->event = DuctTeipWork::Received;
      work->item  = DuctTeipWork::CheckTaskForRun;
      work->host  = task->getHost();
      work_queue.push_back(work);
    }
  }
  void putWorkForReceivedListener(IListener *listener){//ToDo
    TRACE_LOCATION;
    DuctTeipWork *work = new DuctTeipWork;
    work->listener  = listener;
    work->tag   = DuctTeipWork::ListenerWork;
    work->event = DuctTeipWork::Received;
    work->item  = DuctTeipWork::CheckListenerForData;
    work->host  = me ;
    work_queue.push_back(work);
  }
  void putWorkForSendingTask(ITask *task){
    TRACE_LOCATION;
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Added;
    work->item  = DuctTeipWork::SendTask;
    work->host  = task->getHost(); // ToDo : me
    work_queue.push_back(work);
  }
  void putWorkForNewTask(ITask *task){
    TRACE_LOCATION;
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Added;
    work->item  = DuctTeipWork::CheckTaskForData;
    work->host  = task->getHost(); // ToDo : me
    work_queue.push_back(work);
    DuctTeipWork *second_work = new DuctTeipWork;
    *second_work = *work;
    second_work->item  = DuctTeipWork::CheckTaskForRun;
    work_queue.push_back(second_work);
  }
  void putWorkForReceivedTask(ITask *task){
    TRACE_LOCATION;
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
  void putWorkForSendListenerData(IListener *listener){//todo
      DuctTeipWork *work = new DuctTeipWork;
      work->listener = listener;
      work->tag   = DuctTeipWork::ListenerWork;
      work->event = DuctTeipWork::DataReady;
      work->item  = DuctTeipWork::SendListenerData;
      work_queue.push_back(work);      
    
  }
  
  void putWorkForDataReady(list<DataAccess *> *data_list){//ToDo
    list<DataAccess *>::iterator it;
    for (it =data_list->begin(); it != data_list->end() ; it ++) {
      IData *data=(*it)->data;
      DuctTeipWork *work = new DuctTeipWork;
      work->data = data;
      work->tag   = DuctTeipWork::DataWork;
      work->event = DuctTeipWork::DataUpgraded;
      work->item  = DuctTeipWork::CheckAfterDataUpgraded;
      work_queue.push_back(work);      
    }
  }
  void receivedTask(MailBoxEvent *event){
    ITask *task = new ITask;
    int offset = 0 ;
    TRACE_LOCATION;
    task->deserialize(event->buffer,offset,event->length);
    TRACE_LOCATION;
    task->setHost ( me ) ;
    TRACE_LOCATION;

    criticalSection(Enter); 
    TRACE_LOCATION;
    task->setHandle( last_task_handle ++);
    task_list.push_back(task);
    putWorkForReceivedTask(task);
    criticalSection(Leave); 

  }
  void doProcessWorks(){
    criticalSection(Enter); 
    if ( work_queue.size() < 1 ) {
      putWorkForCheckAllTasks();    
      criticalSection(Leave);
      return;
    }
    DuctTeipWork *work = work_queue.front();
    work_queue.pop_front();
    executeWork(work);
    criticalSection(Leave); 
  }
  void addListener(IListener *listener ){
    int handle = last_listener_handle ++;
    listener_list.push_back(listener);
    listener->setHandle(handle);
  }
  void checkTaskDependencies(ITask *task){
    list<DataAccess *> *data_list= task->getDataAccessList();
    list<DataAccess *>::iterator it;
    for (it = data_list->begin(); it != data_list->end() ; it ++) {
      DataAccess &data_access = *(*it);
      int host = data_access.data->getHost();
      if ( host != me ) {
	IListener * lsnr = new IListener((*it),host);
	int buffer_size = sizeof(DataHandle)+sizeof(int)*2+ sizeof(bool);
	byte *buffer = new byte[buffer_size];
	int offset =0;
	addListener(lsnr);
	lsnr->serialize(buffer,offset,buffer_size);
	unsigned long comm_handle = mailbox->send(buffer,buffer_size,MailBox::ListenerTag,host);	
	lsnr->setCommHandle ( comm_handle);
      }
    }
  }
  void executeTaskWork(DuctTeipWork * work){//ToDo
    switch (work->item){
    case DuctTeipWork::SendTask:
      TRACE_LOCATION;
      sendTask(work->task,work->host);
      TRACE_LOCATION;
      break;
    case DuctTeipWork::CheckTaskForData:
      checkTaskDependencies(work->task);
      break;
    case DuctTeipWork::CheckTaskForRun:
      {
	// ToDo . remove the task just for test. this code to be deleted for product version.
	TaskHandle task_handle = work->task->getHandle();
	if ( work->task->canRun() ) {
	  work->task->run();
	  putWorkForDataReady(work->task->getDataAccessList());
	  removeTaskByHandle(task_handle);
	}
      }
      break;
    default:
      printf("work :? %d \n",work->item);
      break;
    }
  }
  void executeDataWork(DuctTeipWork * work){//ToDo
    switch (work->item){
    case DuctTeipWork::CheckAfterDataUpgraded://todo
      list<IListener *>::iterator lsnr_it;
      list<ITask *>::iterator task_it;
      for(lsnr_it = listener_list.begin() ; lsnr_it != listener_list.end(); ++lsnr_it){//Todo , not all of the listeners
	IListener *listener = (*lsnr_it);
	if (listener->isReceived())
	  if (listener->isDataReady())
	    if (!listener->isDataSent() )  
	      {
		putWorkForSendListenerData(listener);
		listener->setDataSent(true);
	      }
      }
      for(task_it = task_list.begin() ; task_it != task_list.end(); ++task_it){//Todo , not all of the tasks
	ITask *task = (*task_it);
	if (task->canRun()) {
	  task->run();
	  //putWorkForDataReady(work->task->getDataAccessList());
	}
      }
      break;
    }
  }
  void sendListenerData(IListener *listener){
    IData *data = listener->getData();
    int offset = 0, max_length = data->getPackSize();
    byte *buffer = new byte[max_length] ;
    TRACE_LOCATION;
    data->serialize(buffer,offset,max_length);
    TRACE_LOCATION;    
    unsigned long handle= mailbox->send(buffer,offset,MailBox::DataTag,listener->getSource());
    TRACE_LOCATION;
    listener->setCommHandle(handle);
    TRACE_LOCATION;
  }
  void executeListenerWork(DuctTeipWork * work){//ToDo
    switch (work->item){
    case DuctTeipWork::CheckListenerForData:
      break;
    case DuctTeipWork::SendListenerData:
      sendListenerData(work->listener);
      break;
    }
  }
  void executeWork(DuctTeipWork *work){
    switch (work->tag){
    case DuctTeipWork::TaskWork:      executeTaskWork(work);      break;
    case DuctTeipWork::ListenerWork:  executeListenerWork(work);  break;
    case DuctTeipWork::DataWork:      executeDataWork(work);      break;
    default: printf("work: tag:%d\n",work->tag);break;
    }
  }
  void doProcessMailBox(){
    MailBoxEvent event ;
    bool wait= false;
    bool found =  mailbox->getEvent(&event,wait);
    if ( !found )
      return ;
    TRACE_LOCATION;
    switch ((MailBox::MessageTag)event.tag){
      case MailBox::TaskTag:
	if ( event.direction == MailBoxEvent::Received ) {
	  TRACE_LOCATION;
	  receivedTask(&event);
	}
	else {
	  ITask *task = getTaskByCommHandle(event.handle);
	  TRACE_LOCATION;
	  TaskHandle task_handle = task->getHandle();
	  TRACE_LOCATION;
	  removeTaskByHandle(task_handle);
	}
	break;
    case MailBox::ListenerTag:
      if (event.direction == MailBoxEvent::Received) {
	receivedListener(&event);
      }
      else{
	IListener *listener= getListenerByCommHandle(event.handle);
	removeListenerByHandle(listener->getHandle()); 
	
      }
      break;
    case MailBox::DataTag:
      TRACE_LOCATION;      
      if (event.direction == MailBoxEvent::Received) {
	TRACE_LOCATION;
	receivedData(&event);
      }
      else{// when data is sent its corresponding listener should be removed.
	TRACE_LOCATION;
	IListener *listener= getListenerByCommHandle(event.handle);
	TRACE_LOCATION;
	listener->getData()->incrementRunTimeVersion(IData::READ);
	removeListenerByHandle(listener->getHandle()); 	
      }
      break;
    }

  }
  IListener *getListenerByCommHandle ( unsigned long  comm_handle ) {//ToDo
    list<IListener *>::iterator it;
    for (it = listener_list.begin(); it !=listener_list.end();it ++){
      IListener *listener=(*it);
      if ( listener->getCommHandle() == comm_handle)
	return listener;
    }
    return NULL;
  }
  void removeListenerByHandle(int handle ) {//ToDo
    list<IListener *>::iterator it;
	TRACE_LOCATION;
        criticalSection(Enter); 
	TRACE_LOCATION;
    for ( it = listener_list.begin(); it != listener_list.end(); it ++){
      IListener *listener = (*it);
      if (listener->getHandle() == handle){
	TRACE_LOCATION;
	listener_list.erase(it);
	criticalSection(Leave); 
	return;
      }
    }
  }

  void removeTaskByHandle(TaskHandle task_handle){
    list<ITask *>::iterator it;
	TRACE_LOCATION;
        criticalSection(Enter); //pthread_mutex_lock(&thread_lock);
	TRACE_LOCATION;
    for ( it = task_list.begin(); it != task_list.end(); it ++){
      ITask *task = (*it);
      if (task->getHandle() == task_handle){
	TRACE_LOCATION;
	task_list.erase(it);
	criticalSection(Leave); //pthread_mutex_unlock(&thread_lock);
	return;
      }
    }
  }
  ITask *getTaskByHandle(TaskHandle  task_handle){
    list<ITask *>::iterator it;
    criticalSection(Enter); //pthread_mutex_lock(&thread_lock);
    TRACE_LOCATION;
    for ( it = task_list.begin(); it != task_list.end(); it ++){
      ITask *task = (*it);
      TRACE_LOCATION;
      if (task->getHandle() == task_handle){
	TRACE_LOCATION;
	criticalSection(Leave);
	return task;
      }
    }
    criticalSection(Leave); //pthread_mutex_unlock(&thread_lock);
  }
  ITask *getTaskByCommHandle(unsigned long handle){
    list<ITask *>::iterator it;
    TRACE_LOCATION;
    criticalSection(Enter); 
    TRACE_LOCATION;
    for ( it = task_list.begin(); it != task_list.end(); it ++){
      ITask *task = (*it);
      if (task->getCommHandle() == handle){
	TRACE_LOCATION;
	criticalSection(Leave);
	return task;
      }
    }
    criticalSection(Leave); //pthread_mutex_unlock(&thread_lock);
    TRACE_LOCATION;
    return NULL;
  }
  void criticalSection(int direction){
    if ( !net_comm->canMultiThread())
      return;
    if ( direction == Enter )
      pthread_mutex_lock(&thread_lock);
    else
      pthread_mutex_unlock(&thread_lock);
  }

};

engine dtEngine;

#endif //__ENGINE_HPP__
