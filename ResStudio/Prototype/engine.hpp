#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

typedef unsigned char byte;
//#include "basic.hpp"
#include <pthread.h>
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
    Received
  };
  enum TaskState{
    Initialized,
    WaitForData,
    Running
  };
  enum WorkItem{
    CheckTaskForData,
    CheckTaskForRun
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
  vector<ITask*>  task_list;
  byte           *prop_buffer;
  int             prop_buffer_length;
  MailBox        *mailbox;
  MPIComm        *net_comm;
  list<DuctTeipWork *> work_queue;
  pthread_t thread_id;
  pthread_mutex_t thread_lock;
public:
  engine(){
    net_comm = new MPIComm;
    mailbox = new MailBox(net_comm);
    me = net_comm->get_host_id();
  }
  ~engine(){
    delete net_comm;
  }
  TaskHandle  addTask(string task_name, int task_host, list<DataAccess *> *data_access){
    ITask *task = new ITask (task_name,task_host,data_access);
    pthread_mutex_lock(&thread_lock);
    TaskHandle task_handle = task_list.size();    
    task_list.push_back(task);
    if (task_host != me ) {
      sendTask(task,task_host);
    }
    pthread_mutex_unlock(&thread_lock);
    return task_handle;
  }
  void dumpTask(TaskHandle th ) {
    ITask &t = *task_list[(int)th];
    printf ("Task:%s @ %d,",t.getName().c_str(),t.getHost());
    dumpDataAccess(t.getDataAccessList());
  }
  void dumpTasks(){
    for ( int i = 0 ; i < task_list.size() ; i ++ ) {
      ITask &t = *task_list[i];
      dumpTask(TaskHandle(i));
    }
  }
  void dumpDataAccess(list<DataAccess *> *dlist){
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("%s,%d rdy:%d ", (*it)->data->getName().c_str(),(*it)->required_version,(*it)->ready);
    }
    printf ("\n");
  }
  void sendPropagateTask(byte *msg_buffer, int msg_size, int dest_host) {
    prop_buffer = msg_buffer;
    prop_buffer_length = msg_size;
  }
  void addPropagateTask(void *P){ 
  }
  void receivePropagateTask(byte **buffer, int *length){
    *buffer  = prop_buffer ;
    *length = prop_buffer_length ;

  }
  void sendTask(ITask* task,int destination);
  void sendListener(){} //ToDo: engine.sendListener
  void sendData() {}//ToDo: engine.sendData
  void receivedListener(){}//ToDo: engine.
  void receivedData(){}//ToDo: engine.
  void finalize(){
    printf("finalize\n");
    pthread_join(thread_id,NULL);
  }
  static void *doProcessLoop(void *p){
    engine *_this = (engine *)p;
    while(true){
      _this->doProcessMailBox();
      _this->doProcessWorks();
    }
  }
  void doProcess(){
    pthread_create(&thread_id,NULL,engine::doProcessLoop,this);
  }
private :
  void receivedTask(MailBoxEvent *event){
    ITask *task = new ITask;
    int offset = 0 ;
    TRACE_LOCATION;
    task->deserialize(event->buffer,offset,event->length);
    TRACE_LOCATION;
    task->setHandle( (TaskHandle)task_list.size());
    TRACE_LOCATION;
    task->setHost ( me ) ;
    TRACE_LOCATION;
    task_list.push_back(task);
    TRACE_LOCATION;
    task->dump();
    TRACE_LOCATION;
    dumpDataAccess(task->getDataAccessList());
    TRACE_LOCATION;
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Received;
    work->item  = DuctTeipWork::CheckTaskForData;
    work->host  = event->host;
    printf("work: add task work tag:%d\n",work->tag);
    work_queue.push_back(work);    
    DuctTeipWork *second_work = new DuctTeipWork;
    *second_work = *work;
    second_work->item  = DuctTeipWork::CheckTaskForRun;
    printf("second work: add task work tag:%d\n",second_work->tag);
    work_queue.push_back(second_work);    
  }
  void doProcessWorks(){
    if ( work_queue.size() < 1 ) 
      return;
    DuctTeipWork *work = work_queue.front();
    work_queue.pop_front();
    executeWork(work);
    printf("work count after run:%ld\n",work_queue.size());
  }
  void executeTaskWork(DuctTeipWork * work){//ToDo
    switch (work->item){
    case DuctTeipWork::CheckTaskForData:   
      printf("work :task check for data \n");
      break;
    case DuctTeipWork::CheckTaskForRun:   
      printf("work :task chech for run \n");
      break;
    default:
      printf("work :? %d \n",work->item);
      break;
    }
    work->task->dump();
    dumpDataAccess(work->task->getDataAccessList());
  }
  void executeDataWork(DuctTeipWork * work){//ToDo
    switch (work->event){
    case DuctTeipWork::Sent:       break;
    case DuctTeipWork::Ready:      break;
    case DuctTeipWork::Received:   break;
    }
  }
  void executeListenerWork(DuctTeipWork * work){//ToDo
    switch (work->event){
    case DuctTeipWork::Sent:          break;
    case DuctTeipWork::DataSent:      break;
    case DuctTeipWork::DataReceived:  break;
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
    printf("received msg tag:%d,len:%d,host:%d. task-tag:%d\n",event.tag,event.length,event.host,MailBox::TaskTag);
    switch ((MailBox::MessageTag)event.tag){
    case MailBox::TaskTag: 	
      if ( event.direction == MailBoxEvent::Received ) {
	TRACE_LOCATION;
	receivedTask(&event);
      }
      else {
	ITask *task = getTaskByCommHandle(event.handle);
	// removeTask(task->getHandle()); // ToDo
      }
      break;
    }
     
  }
  ITask *getTaskByCommHandle(unsigned long handle){
    for ( int i=0; i < task_list.size(); i++){
      if (task_list[i]->getCommHandle() == handle){
	return task_list[i];
      }
    }
  }

};

engine dtEngine;
#endif //__ENGINE_HPP__
