#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

typedef unsigned char byte;
#include <pthread.h>
#include <sched.h>
#include "config.hpp"
#include "task.hpp"
#include "listener.hpp"
#include "mailbox.hpp"
#include "mpi_comm.hpp"
#include "memory_manager.hpp"
extern int me;

struct PropagateInfo;


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
  IDuctteipTask *task;
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
/*===================================================================*/
class engine
{
private:
  list<IDuctteipTask*>  task_list;
  byte           *prop_buffer;
  int             term_ok;
  int             prop_buffer_length,num_threads,local_nb;
  MailBox        *mailbox;
  INetwork         *net_comm;
  list<DuctTeipWork *> work_queue;
  pthread_t thread_id;
  pthread_mutex_t thread_lock;
  pthread_mutexattr_t mutex_attr;
  long last_task_handle,last_listener_handle;
  list<IListener *> listener_list;
  TimeUnit start_time;
  SuperGlue<Options> *thread_manager;
  Config *cfg;
  MemoryManager *data_memory;
  enum { Enter, Leave};
  enum {
    EVEN_INIT     ,
    WAIT_FOR_FIRST  ,
    FIRST_RECV    ,
    WAIT_FOR_SECOND ,
    SECOND_RECV   ,
    TERMINATE_OK
  };
public:
  /*---------------------------------------------------------------------------------*/
  engine(){
    net_comm = new MPIComm;
    mailbox = new MailBox(net_comm);
    me = net_comm->get_host_id();
    last_task_handle = 0;
    start_time = getTime();
    if ( IsEven(me) ) 
      term_ok = EVEN_INIT;
    if ( IsOdd(me) ) 
      term_ok = WAIT_FOR_FIRST;
  }
  /*---------------------------------------------------------------------------------*/

  ~engine(){
    TRACE_LOCATION;
    delete net_comm;
    TRACE_LOCATION;
    //delete thread_manager;//todo 
    TRACE_LOCATION;
    delete data_memory;
  }
  /*---------------------------------------------------------------------------------*/
  SuperGlue<Options> * getThrdManager() {return thread_manager;}
  int getLocalNumBlocks(){return local_nb;}
  /*---------------------------------------------------------------------------------*/
  TaskHandle  addTask(IContext * context,
		      string task_name,
		      unsigned long key, 
		      int task_host, 
		      list<DataAccess *> *data_access)
  {
    IDuctteipTask *task = new IDuctteipTask (context,task_name,key,task_host,data_access);
    criticalSection(Enter) ;
    TaskHandle task_handle = last_task_handle ++;
    task->setHandle(task_handle);
    task_list.push_back(task);
    if (task_host != me ) {
      putWorkForSendingTask(task);
    }
    else {
      putWorkForNewTask(task);
    }
    criticalSection(Leave) ;
    
    if (!net_comm->canMultiThread()) {
      TRACE_LOCATION;
      doProcessMailBox();
      doProcessWorks();
    }
    TRACE_LOCATION;
    return task_handle;
  }
  /*---------------------------------------------------------------------------------*/
  void dumpTasks(){
    list<IDuctteipTask *>::iterator it;
    for (  it =task_list.begin();  it!= task_list.end(); it ++) {
      IDuctteipTask &t = *(*it);
      t.dump();
      break;
    }
  }
  /*---------------------------------------------------------------------------------*/
  void sendPropagateTask(byte *buffer, int size, int dest_host) {
    mailbox->send(buffer,size,MailBox::PropagationTag,dest_host);
    prop_buffer = buffer;
    prop_buffer_length = size;
  }
  /*---------------------------------------------------------------------------------*/
  void addPropagateTask(PropagateInfo *P);
  void receivePropagateTask(byte *buffer, int len);
  void sendTask(IDuctteipTask* task,int destination);
  /*---------------------------------------------------------------------------------*/
  void receivedListener(MailBoxEvent *event){
    IListener *listener = new IListener;
    int offset = 0 ;
    listener->deserialize(event->buffer,offset,event->length);
    listener->setHost ( me ) ;
    listener->setSource ( event->host ) ;
    listener->setReceived(true);
    listener->setDataSent(false);

    int host = event->host;
    DataVersion version = listener->getRequiredVersion();
    version.dump();
    listener->getData()->listenerAdded(host,version);
    criticalSection(Enter); 
    listener->setHandle( last_listener_handle ++);
    listener->setCommHandle(-1);
    export ( V_RCVD,O_LSNR);
    listener->dump();
    export_end(V_RCVD);
    listener_list.push_back(listener);
    putWorkForReceivedListener(listener);
    criticalSection(Leave); 

  }
  /*---------------------------------------------------------------------------------*/
  void receivedData(MailBoxEvent *event,MemoryItem*);
  /*---------------------------------------------------------------------------------*/
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
  /*---------------------------------------------------------------------------------*/
  void globalSync(){
    TimeUnit Duration = elapsedTime(TIME_SCALE_TO_MILI_SECONDS);
    export_long(Duration);
    printf(",before finish:%ld,",Duration);

    net_comm->finish();

     Duration = elapsedTime(TIME_SCALE_TO_MILI_SECONDS);
    export_long(Duration);
    printf(",Duration:%ld,",Duration);
  }
  /*---------------------------------------------------------------------------------*/
  TimeUnit elapsedTime(int scale){
    TimeUnit t  = getTime();
    return ( t - start_time)/scale;
  }
  /*---------------------------------------------------------------------------------*/
  void dumpTime(char c=' '){
    //if (DUMP_FLAG)
    printf ("%c time:%ld ",c,elapsedTime(TIME_SCALE_TO_MILI_SECONDS));
  }
  /*---------------------------------------------------------------------------------*/
  bool isAnyUnfinishedListener(){
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
  long getUnfinishedTasks(){
    list<IDuctteipTask *>::iterator it;
    it = task_list.begin();
    for(; it != task_list.end(); ){
      IDuctteipTask * task = (*it);
      if (task->canBeCleared()){
	task_list.erase(it);
	it = task_list.begin();
      }
      else
	it ++;
    }
    //dumpTime();printf("taskn:%ld\n",task_list.size());
    return task_list.size();
  }
  /*---------------------------------------------------------------------------------*/
  bool canTerminate(){    
    if (elapsedTime(TIME_SCALE_TO_SECONDS)>100){
      export(V_TERMINATED,O_PROG);
      export_long(start_time);
      TimeUnit Duration = elapsedTime(TIME_SCALE_TO_SECONDS);
      export_long(Duration);
      export_end(V_TERMINATED);
      return true;
    }
    if ( !net_comm->canTerminate() ) {
      return false;
    }
    if ( term_ok == TERMINATE_OK ) 
      return true;
    
    
    if (last_task_handle > 0 && getUnfinishedTasks() < 1)
      if (last_listener_handle > 0 && !isAnyUnfinishedListener() ){
	PRINT_IF(0)("task count:%ld unf-lsnr:%d\n",getUnfinishedTasks(),isAnyUnfinishedListener());
	sendTerminateOK();
      }

    return false;
  }
  /*---------------------------------------------------------------------------------*/
  int  getTaskCount(){    return task_list.size();  }
  /*---------------------------------------------------------------------------------*/
  void setConfig(Config *cfg_){
    cfg = cfg_;
    num_threads = cfg->getNumThreads();
    local_nb = cfg->getXLocalBlocks();
    int mb =  cfg->getYBlocks();
    int nb =  cfg->getXBlocks();
    int ny = cfg->getYDimension() / mb;
    int nx = cfg->getXDimension() / nb;
    DataHandle dh;
    DataVersion dv;
    
    
    data_memory = new MemoryManager (  nb * mb/3 , ny * nx * sizeof(double) + dh.getPackSize() + 4*dv.getPackSize());
    thread_manager = new SuperGlue<Options> ( num_threads ) ;
  }
  /*---------------------------------------------------------------------------------*/
  void doProcess(){

    if ( net_comm->canMultiThread() ) {
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
  /*---------------------------------------------------------------------------------*/
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
  /*---------------------------------------------------------------------------------*/
  void putWorkForDataReady(list<DataAccess *> *data_list){
    list<DataAccess *>::iterator it;
    for (it =data_list->begin(); it != data_list->end() ; it ++) {
      IData *data=(*it)->data;
      putWorkForSingleDataReady(data);
    }
  }
  /*---------------------------------------------------------------------------------*/
  MemoryItem *newDataMemory(){
    MemoryItem *m = data_memory->getNewMemory();
    return m;
  }
private :
  /*---------------------------------------------------------------------------------*/
  void putWorkForCheckAllTasks(){
    list<IDuctteipTask *>::iterator it;
    it = task_list.begin();
    for(; it != task_list.end(); ){
      IDuctteipTask *task = (*it);
      if (task->canBeCleared()){
	task_list.erase(it);
	it = task_list.begin();
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
  }
  /*---------------------------------------------------------------------------------*/
  void putWorkForReceivedListener(IListener *listener){
    DuctTeipWork *work = new DuctTeipWork;
    work->listener  = listener;
    work->tag   = DuctTeipWork::ListenerWork;
    work->event = DuctTeipWork::Received;
    work->item  = DuctTeipWork::CheckListenerForData;
    work->host  = me ;
    work_queue.push_back(work);
  }
  /*---------------------------------------------------------------------------------*/
  void putWorkForSendingTask(IDuctteipTask *task){
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Added;
    work->item  = DuctTeipWork::SendTask;
    work->host  = task->getHost(); // ToDo : create_place, dest_place
    work_queue.push_back(work);
  }
  /*---------------------------------------------------------------------------------*/
  void putWorkForPropagateTask(IDuctteipTask *task){
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Added;
    work->item  = DuctTeipWork::CheckTaskForRun;
    work->host  = task->getHost(); // ToDo : create_place, dest_place
    work_queue.push_back(work);
  }
  /*---------------------------------------------------------------------------------*/
  void putWorkForNewTask(IDuctteipTask *task){
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
  void putWorkForReceivedTask(IDuctteipTask *task){
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
  void putWorkForSendListenerData(IListener *listener){
      DuctTeipWork *work = new DuctTeipWork;
      work->listener = listener;
      work->tag   = DuctTeipWork::ListenerWork;
      work->event = DuctTeipWork::DataReady;
      work->item  = DuctTeipWork::SendListenerData;
      work_queue.push_back(work);      
    
  }
  /*---------------------------------------------------------------------------------*/
  void putWorkForSingleDataReady(IData* data){
    DuctTeipWork *work = new DuctTeipWork;
    work->data = data;
    work->tag   = DuctTeipWork::DataWork;
    work->event = DuctTeipWork::DataUpgraded;
    work->item  = DuctTeipWork::CheckAfterDataUpgraded;
    work_queue.push_back(work);      
  }
  /*---------------------------------------------------------------------------------*/
  void receivedTask(MailBoxEvent *event){
    IDuctteipTask *task = new IDuctteipTask;
    int offset = 0 ;
    task->deserialize(event->buffer,offset,event->length);
    task->setHost ( me ) ;

    criticalSection(Enter); 
    task->setHandle( last_task_handle ++);
    task_list.push_back(task);
    putWorkForReceivedTask(task);
    criticalSection(Leave); 

  }
  /*---------------------------------------------------------------------------------*/
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
  /*---------------------------------------------------------------------------------*/
  bool isDuplicateListener(IListener * listener){
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
  bool  addListener(IListener *listener ){
    //    if (isDuplicateListener(listener))
    //      return false;
    int handle = last_listener_handle ++;
    listener_list.push_back(listener);
    listener->setHandle(handle);
    listener->setCommHandle(-1);
    return true;
  }
  /*---------------------------------------------------------------------------------*/
  void checkTaskDependencies(IDuctteipTask *task){
    list<DataAccess *> *data_list= task->getDataAccessList();
    list<DataAccess *>::iterator it;
    export(V_CHECK_DEP,O_TASK);
    export_info(", name:%s, ",task->getName().c_str());
    for (it = data_list->begin(); it != data_list->end() ; it ++) {
      DataAccess &data_access = *(*it);
      int host = data_access.data->getHost();
      if ( host != me ) {
	IListener * lsnr = new IListener((*it),host);
	if ( addListener(lsnr) ){
	  MessageBuffer *m=lsnr->serialize();
	  unsigned long comm_handle = mailbox->send(m->address,m->size,MailBox::ListenerTag,host);	
	  export(V_ADDED,O_LSNR);
	  lsnr->setCommHandle ( comm_handle);
	  export_end(V_ADDED);
	}
	else{
	  TRACE_LOCATION;
	  delete lsnr;
	  TRACE_LOCATION;
	}
      }
    }
    export_end(V_CHECK_DEP);
  }
  /*---------------------------------------------------------------------------------*/
  void executeTaskWork(DuctTeipWork * work){
    switch (work->item){
    case DuctTeipWork::SendTask:
      sendTask(work->task,work->host);
      break;
    case DuctTeipWork::CheckTaskForData:
      checkTaskDependencies(work->task);
      break;
    case DuctTeipWork::CheckTaskForRun:
      {
	TaskHandle task_handle = work->task->getHandle();
	if ( work->task->canRun() ) {
	  work->task->run();
	  return;  
	}
      }
      break;
    default:
      printf("error: undefined work : %d \n",work->item);
      break;
    }
  }
  /*---------------------------------------------------------------------------------*/
  void executeDataWork(DuctTeipWork * work){
    switch (work->item){
    case DuctTeipWork::CheckAfterDataUpgraded:
      list<IListener *>::iterator lsnr_it;
      list<IDuctteipTask *>::iterator task_it;
      for(lsnr_it = listener_list.begin() ; 
	  lsnr_it != listener_list.end()  ; 
	  ++lsnr_it){//Todo , not all of the listeners to be checked
	IListener *listener = (*lsnr_it);
	listener->checkAndSendData(mailbox);
	dumpTasks();
      }
      for(task_it = task_list.begin() ; 
	  task_it != task_list.end()  ;
	  ++task_it){//Todo , not all of the tasks
	IDuctteipTask *task = (*task_it);
	if (task->canRun()) {
	  task->run();
	  putWorkForDataReady(task->getDataAccessList());
	}
      }
      break;
    }
  }
  /*---------------------------------------------------------------------------------*/
  void sendListenerData(IListener *listener){

    IData *data = listener->getData();
    int offset = 0;
    data->serialize();
    unsigned long handle= mailbox->send(data->getHeaderAddress(),
					data->getPackSize() ,
					MailBox::DataTag,
					listener->getSource());
    listener->setCommHandle(handle);
    export(V_SEND,O_DATA);
    listener->dump();
    export_end(V_SEND);
  }
  /*---------------------------------------------------------------------------------*/
  void executeListenerWork(DuctTeipWork * work){
    TRACE_LOCATION;
    switch (work->item){
    case DuctTeipWork::CheckListenerForData:
      {
      IListener *listener = work->listener;
    TRACE_LOCATION;
      dumpTasks();
      listener->checkAndSendData(mailbox) ; 
    TRACE_LOCATION;
      dumpTasks();
    TRACE_LOCATION;
      }
      break;
    case DuctTeipWork::SendListenerData:
      sendListenerData(work->listener);
      break;
    }
    TRACE_LOCATION;
  }
  /*---------------------------------------------------------------------------------*/
  void executeWork(DuctTeipWork *work){
    switch (work->tag){
    case DuctTeipWork::TaskWork:      executeTaskWork(work);      break;
    case DuctTeipWork::ListenerWork:  executeListenerWork(work);  break;
    case DuctTeipWork::DataWork:      executeDataWork(work);      break;
    default: printf("work: tag:%d\n",work->tag);break;
    }
  }
  /*---------------------------------------------------------------------------------*/
  void doProcessMailBox(){
    MailBoxEvent event ;
    bool wait= false;
    MemoryItem *m = data_memory->getNewMemory();
    bool found =  mailbox->getEvent(m->getAddress(),&event,wait);
    if ( !found ){
      m->setState(MemoryItem::Ready);
      return ;
    }
    dumpTime();
    event.dump();
    if ( (MailBox::MessageTag)event.tag != MailBox::DataTag) {
      PRINT_IF(0)("mem InUse cleared,event:%d\n",event.tag);
      m->setState(MemoryItem::Ready);
    }
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
      break;
    case MailBox::DataTag:
      if (event.direction == MailBoxEvent::Received) {
	receivedData(&event,m);
      }
      else{
	IListener *listener = getListenerByCommHandle(event.handle);
	listener->getData()->dataIsSent(listener->getSource());
	m->setState(MemoryItem::Ready);
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
    }
    if ( event.direction  == MailBoxEvent::Received) {
      if (event.tag != MailBox::DataTag) {
	//delete event.buffer; //todo
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
  IListener *getListenerForData(IData *data){
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
  void dumpListeners(){
    
    list<IListener *>::iterator it;
    for (it = listener_list.begin(); it != listener_list.end(); it ++){
      (*it)->dump();
    }
  }
  /*---------------------------------------------------------------------------------*/
  void dumpAll(){
    printf("------------------------------------------------\n");
    dumpTasks();
    dumpListeners();
    printf("------------------------------------------------\n");
  }
  /*---------------------------------------------------------------------------------*/
  void removeListenerByHandle(int handle ) {
    list<IListener *>::iterator it;
    for ( it = listener_list.begin(); it != listener_list.end(); it ++){
      IListener *listener = (*it);
      if (listener->getHandle() == handle){
	criticalSection(Enter); 
	export(V_REMOVE,O_LSNR);
	export_end(V_REMOVE);
	listener_list.erase(it);
	criticalSection(Leave);
	return;
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
  void removeTaskByHandle(TaskHandle task_handle){
    list<IDuctteipTask *>::iterator it;
    criticalSection(Enter);
    for ( it = task_list.begin(); it != task_list.end(); it ++){
      IDuctteipTask *task = (*it);
      if (task->getHandle() == task_handle){
	task_list.erase(it);
	criticalSection(Leave);
	return;
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
  void criticalSection(int direction){
    if ( !net_comm->canMultiThread())
      return;
    if ( direction == Enter )
      pthread_mutex_lock(&thread_lock);
    else
      pthread_mutex_unlock(&thread_lock);
  }
  /*---------------------------------------------------------------------------------*/
  IDuctteipTask *getTaskByHandle(TaskHandle  task_handle){
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
  }
  /*---------------------------------------------------------------------------------*/
  IDuctteipTask *getTaskByCommHandle(unsigned long handle){
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
    printf("error:task not found by comm-handle:%ld\n",handle);
    return NULL;
  }
  /*---------------------------------------------------------------------------------*/
  IListener *getListenerByCommHandle ( unsigned long  comm_handle ) {
    list<IListener *>::iterator it;
    for (it = listener_list.begin(); it !=listener_list.end();it ++){
      IListener *listener=(*it);
      if ( listener->getCommHandle() == comm_handle)
	return listener;
    }
    printf("\nerror:listener not found by comm-handle %ld\n",comm_handle);
    return NULL;
  }
  /*---------------------------------------------------------------------------------*/
  void sendTerminateOK_old(){
    PRINT_IF(TERMINATE_FLAG) ( "send,term state:%d\n" , term_ok ) ;

    if ( me == 0 && term_ok == EVEN_INIT  ) {
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,1);
      term_ok = WAIT_FOR_FIRST;
    }      
    if ( me ==0 && term_ok ==FIRST_RECV ) {
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,1);
      term_ok = WAIT_FOR_SECOND;
    }
    if ( me ==0 && term_ok ==SECOND_RECV ) {
      term_ok = TERMINATE_OK;
    }
    if ( me != 0 && term_ok == FIRST_RECV ) {
      int dest = ( me +1 ) % net_comm->get_host_count();
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,dest);
      term_ok = WAIT_FOR_SECOND;
    }
    if ( me != 0 && term_ok == SECOND_RECV ) {
      int dest = ( me +1 ) % net_comm->get_host_count();
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,dest);
      term_ok = TERMINATE_OK;
    }
    PRINT_IF(TERMINATE_FLAG) ( "send,term state:%d\n" , term_ok ) ;

  }
  /*---------------------------------------------------------------------------------*/
  void receivedTerminateOK_old(){
    PRINT_IF(TERMINATE_FLAG) ( "recv,term state:%d\n" , term_ok ) ;
    if ( term_ok == WAIT_FOR_FIRST)
      term_ok = FIRST_RECV;
    if ( term_ok == WAIT_FOR_SECOND)
      term_ok = SECOND_RECV;
    PRINT_IF(TERMINATE_FLAG) ( "recv,term state:%d\n" , term_ok ) ;
    
  }
  /*---------------------------------------------------------------------------------*/
  inline bool IsOdd (int a){return ( (a %2 ) ==1);}
  inline bool IsEven(int a){return ( (a %2 ) ==0);}
  /*---------------------------------------------------------------------------------*/
  void sendTerminateOK(){
    bool odd_node  = IsOdd (me);
    bool even_node = IsEven(me);
    int dest;
    static bool sent = false;
    PRINT_IF(TERMINATE_FLAG) ("term-ok:%d\n",term_ok);
    if ( odd_node) {
      if ( term_ok == SECOND_RECV){
	dest = (me +1 ) %   net_comm->get_host_count();
	mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,dest);
	PRINT_IF(TERMINATE_FLAG)("term-ok sent to:%d\n",dest);
	mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,me-1);
	PRINT_IF(TERMINATE_FLAG)("term-ok sent to:%d,term-ok:true\n",me-1);
	term_ok = TERMINATE_OK;
      }
    }
    if ( even_node ) {
      if ( term_ok == WAIT_FOR_FIRST || term_ok ==EVEN_INIT || term_ok == WAIT_FOR_SECOND) {
	if ( !sent){
	  dest =  (me+1)%   net_comm->get_host_count() ;
	  PRINT_IF(TERMINATE_FLAG)("term-ok sent to:%d,term-ok:w41\n",dest);
	  mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,dest);
	  dest = me-1;
	  if ( dest <0 ) 
	    dest += net_comm->get_host_count() ;
	  mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,dest);
	  PRINT_IF(TERMINATE_FLAG)("term-ok sent to:%d,term-ok:w41\n",dest);
	  sent = true;
	}
	if ( term_ok == WAIT_FOR_FIRST ) 
	  term_ok = WAIT_FOR_SECOND;
	else if (term_ok == EVEN_INIT)
	  term_ok = WAIT_FOR_FIRST;
	else if ( term_ok == WAIT_FOR_SECOND ) 
	  term_ok = TERMINATE_OK;
      }
    }  
    
  }
  /*---------------------------------------------------------------------------------*/
  void receivedTerminateOK(int from){
    bool odd_node  = IsOdd (me);
    bool even_node = IsEven(me);

#define ARTIFITIAL_CANCEL 0
#if (ARTIFITIAL_CANCEL == 1)
    static int just_for_test = 0;
#endif

    if (odd_node){
      if ( term_ok == WAIT_FOR_FIRST ) {
	term_ok = WAIT_FOR_SECOND;	
      }
      else 
	if ( term_ok == WAIT_FOR_SECOND ) {
#if ( ARTIFITIAL_CANCEL == 1 ) 
	  if ( me == 1 && just_for_test == 0  ) {
	    just_for_test ++;
	    sendTerminateCancel();
	    return;
	  }
#endif
	  term_ok = SECOND_RECV;
	}
    }
    if ( even_node ) {
      if ( term_ok == EVEN_INIT  ) 
	term_ok = WAIT_FOR_FIRST;
      else if (  term_ok == WAIT_FOR_FIRST ) 
	term_ok = WAIT_FOR_SECOND;
      else if ( term_ok == WAIT_FOR_SECOND )
	term_ok = TERMINATE_OK;	
    }
    PRINT_IF(TERMINATE_FLAG)("term-ok recv from :%d,term-ok:%d\n",from,term_ok);
  }
  /*---------------------------------------------------------------------------------*/
  void sendTerminateCancel(){
    bool odd_node = IsOdd(me);
    bool even_node = IsEven(me);
    int dest;
    
    if ( term_ok == WAIT_FOR_FIRST || term_ok == EVEN_INIT ) {
      return;
    }
    if ( odd_node ) {
      dest = (me +1 ) %   net_comm->get_host_count();
      mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateCancelTag,dest);
      PRINT_IF(TERMINATE_FLAG)("term-cancel sent to:%d\n",dest);
      mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateCancelTag,me-1);
      term_ok = WAIT_FOR_FIRST;
      PRINT_IF(TERMINATE_FLAG)("term-cancel sent to:%d,term-ok:w41,dest:%d,is-even(dest):%d\n",me-1,dest,IsEven(dest));
    }
    if ( even_node ) {
	dest =  (me+1)%   net_comm->get_host_count() ;
	PRINT_IF(TERMINATE_FLAG)("term-cancel sent to:%d,term-ok:even-init\n",dest);
	mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateCancelTag,dest);
	dest = me-1;
	if ( dest <0 ) 
	  dest += net_comm->get_host_count() ;
	mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateCancelTag,dest);
	PRINT_IF(TERMINATE_FLAG)("term-cancel sent to:%d,term-ok:even-init\n",dest);
	term_ok = EVEN_INIT;
    }
    
  }
  /*---------------------------------------------------------------------------------*/
  void receivedTerminateCancel( int from ) {
    bool odd_node = IsOdd (me); 
    bool even_node= IsEven(me);
    int n = net_comm->get_host_count();

    PRINT_IF(TERMINATE_FLAG)("term-cancel recv from:%d,term-ok:%d\n",from,term_ok);
    if ( even_node && term_ok == EVEN_INIT )
      return;
    if ( odd_node &&  term_ok == WAIT_FOR_FIRST)
      return;

    int dest = me + me - from;
    if ( IsEven(n) && me ==0) {
      dest = n-1;
    }
    if ( IsEven(n) && me == (n-1) ) {
      dest = me -1;
    }
    mailbox->send( (byte*)&term_ok,sizeof(term_ok),MailBox::TerminateCancelTag,dest);
    PRINT_IF(TERMINATE_FLAG)("term-cancel sent to:%d,term-ok:%d\n",dest,term_ok);


  }
};


#endif //__ENGINE_HPP__
