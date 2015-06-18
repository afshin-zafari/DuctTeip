#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

#define logEvent(a,b) 



typedef unsigned char byte;
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <algorithm>
#include <vector>
#include <iterator>

#include <dirent.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>


#include "config.hpp"
#include "task.hpp"
#include "listener.hpp"
#include "mailbox.hpp"
#include "mpi_comm.hpp"
#include "memory_manager.hpp"
#include "dt_log.hpp"
extern int me;

struct PropagateInfo;
DuctteipLog dt_log;
extern Config config;

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
    UpgradeData,
    CheckAfterDataUpgraded,
    SendListenerData
  };
  IDuctteipTask *task;
  IData *data;
  IListener *listener;
  int state,tag,event,host,item;
  //  DuctTeipWork(){ }
  DuctTeipWork &operator =(DuctTeipWork *_work){
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
  void dump(){
    printf("work dump: tag:%d , item:%d\n",tag,item);
    printf("work dump:  ev:%d , stat:%d\n",event,state);
    printf("work dump: host:%d\n",host);
  }
};
/*===================================================================*/
class engine
{
private:
  list<IDuctteipTask*>  task_list,running_tasks,export_tasks,import_tasks;
  bool                  runMultiThread;
  byte           *prop_buffer;
  int             term_ok;
  int             prop_buffer_length,num_threads,local_nb;
  MailBox        *mailbox;
  INetwork         *net_comm;
  list<DuctTeipWork *> work_queue;
  pthread_t thread_id;
  pthread_mutex_t thread_lock;
  pthread_mutexattr_t mutex_attr;
  pthread_attr_t attr;
  long last_task_handle,last_listener_handle;
  list<IListener *> listener_list;
  TimeUnit start_time;
  ClockTimeUnit start_clktime;
  ThreadManager<Options> *thread_manager;
  Config *cfg;
  MemoryManager *data_memory;
  long skip_term,skip_work,skip_mbox,time_out;
  enum { Enter, Leave};
  enum {
    EVEN_INIT     ,
    WAIT_FOR_FIRST  ,
    FIRST_RECV    ,
    WAIT_FOR_SECOND ,
    SECOND_RECV   ,
#if TERMINATE_TREE ==1
    TERMINATE_INIT=100,
    LEAF_TERM_OK  =101,
    ONE_CHILD_OK  =102,
    ALL_CHILDREN_OK=103,
    SENT_TO_PARENT=104,
#endif
    TERMINATE_OK
  };
public:
  /*---------------------------------------------------------------------------------*/
  engine(){
    net_comm = new MPIComm;
    mailbox = new MailBox(net_comm);
    initComm();
    last_task_handle = 0;
#if TERMINATE_TREE == 0
    if ( IsEven(me) ) 
      term_ok = EVEN_INIT;
    if ( IsOdd(me) ) 
      term_ok = WAIT_FOR_FIRST;
#else
    term_ok = TERMINATE_INIT;
#endif 
    runMultiThread=true;
    initDLB();
  }
  /*---------------------------------------------------------------------------------*/
  void start ( int argc , char **argv);
  /*---------------------------------------------------------------------------------*/
  void initComm(){
    net_comm->initialize();
    resetTime();
    me = net_comm->get_host_id();
  }
  /*---------------------------------------------------------------------------------*/

  ~engine(){
    delete net_comm;
    delete thread_manager;
    delete data_memory;
  }
  /*---------------------------------------------------------------------------------*/
  void setSkips(long st,long sw,long sm,long to){
    skip_term = st;
    skip_work = sw;
    skip_mbox = sm;
    time_out = to;
  }
  /*---------------------------------------------------------------------------------*/
  ThreadManager<Options> * getThrdManager() {return thread_manager;}
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
    dt_log.addEventStart ( task,DuctteipLog::TaskDefined);
    task_list.push_back(task);
    if (task_host != me ) {
      putWorkForSendingTask(task);
    }
    else {
      putWorkForNewTask(task);
    }
    criticalSection(Leave) ;
    
    if ( !runMultiThread ) {
      doProcessMailBox();
      doProcessWorks();
    }
    dt_log.addEventEnd ( task,DuctteipLog::TaskDefined);
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
  void exportTask(IDuctteipTask* task,int destination);
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
    PRINT_IF(POSTPRINT)("a new postLsnr is substituted.\n");
    mailbox->prepareListenerReceive(listener->getPackSize(),host);
    version.dump();
    listener->getData()->listenerAdded(listener,host,version);
    criticalSection(Enter); 
    listener->setHandle( last_listener_handle ++);
    listener->setCommHandle(-1);
    dt_log.addEvent ( listener,DuctteipLog::ListenerReceived);
    listener->dump();
    listener_list.push_back(listener);
    putWorkForReceivedListener(listener);
    criticalSection(Leave); 

  }
  /*---------------------------------------------------------------------------------*/
  void receivedData(MailBoxEvent *event,MemoryItem*);
  IData *importedData(MailBoxEvent *event,MemoryItem*);
  /*---------------------------------------------------------------------------------*/
  void waitForTaskFinish(){
    static long skip_wait=0;
    /*
    if ( ++skip_wait < skip_work){
      return ;
    }
    skip_wait=0;
    */
    
    
    if ( !checkRunningTasks() ) 
      return;
    if ( running_tasks.size() >=0 ) {

      PRINT_IF(OVERSUBSCRIBED)("rt size:%ld,time:%Ld\n",running_tasks.size(),getTime());
      list<IDuctteipTask *> ::iterator it;
      for(it= running_tasks.begin(); it != running_tasks.end(); it ++){
	IDuctteipTask *task=(*it);
	timespec timeout,t;
	//clock_gettime(CLOCK_REALTIME,&timeout);
	timeout.tv_sec =0;
	timeout.tv_nsec+=1;

	if ( task->getState() != IDuctteipTask::Running ) 
	  continue;
	PRINT_IF(OVERSUBSCRIBED)("mutex for task:%s,%ld timedlock,time:%Ld\n",task->getName().c_str(),task->getHandle(),getTime());

	int ret = 0;
	ret = pthread_mutex_timedlock(task->getTaskFinishMutex(),&timeout);

	PRINT_IF(OVERSUBSCRIBED)("mutex for task:%s,%ld timedlock,time:%Ld,return value :%d\n",task->getName().c_str(),task->getHandle(),getTime(),ret);

	if (  ret == ETIMEDOUT ){
	  PRINT_IF(OVERSUBSCRIBED)("timedout lock for task:%s,time:%Ld, check ext running task.\n",task->getName().c_str(),getTime());
	  continue; 
	}
	if ( pthread_mutex_trylock(task->getTaskFinishMutex()) ==0 ) {
	  PRINT_IF(1)("lock successfull,then task:%s is finished,time:%Ld\n",task->getName().c_str(),getTime());
	  if ( checkRunningTasks() )
	    return;
	}
	
      }

    }
  }
  
  /*---------------------------------------------------------------------------------*/
  void finalize(){
    if ( runMultiThread ) {
      pthread_mutex_destroy(&thread_lock);
      pthread_mutexattr_destroy(&mutex_attr);
      printf("before join %Ld, thread-id:%ld\n",getTime(),pthread_self());      
      pthread_join(thread_id,NULL);
      printf("after join %Ld, thread-id:%ld\n",getTime(),pthread_self());
    }
    else
      doProcessLoop((void *)this);
    globalSync();
    if(cfg->getDLB())
      dumpDLB();
  }
  /*---------------------------------------------------------------------------------*/
  void globalSync(){
    dt_log.addEventEnd(this,DuctteipLog::ProgramExecution);
    dt_log.logLoadChange(0);
    dt_log.logImportTask(0);
    dt_log.logExportTask(0);

    dt_log.addEventStart(this,DuctteipLog::CommFinish);
    net_comm->finish();
    dt_log.addEventEnd(this,DuctteipLog::CommFinish);
    if (true || cfg->getYDimension() == 2400){
      char s[20];
      sprintf(s,"sg_log_file-%2.2d.txt",me);
      Log<Options>::dump(s);
    }
    long tc=thread_manager->getTaskCount();
    dt_log.dump(tc);


  }
  /*---------------------------------------------------------------------------------*/
  TimeUnit elapsedTime(int scale){
    TimeUnit t  = getTime();
    return ( t - start_time)/scale;
  }
  /*---------------------------------------------------------------------------------*/
  void dumpTime(char c=' '){
    if (DUMP_FLAG)
      printf ("%c time:%Ld\n",c,elapsedTime(TIME_SCALE_TO_MILI_SECONDS));
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
	checkRunningTasks();
	it=task_list.erase(it);
	if(0)printf("TL:%ld\n",task_list.size());
      }
      else
	it ++;
    }
    //dumpTime();printf("taskn:%ld\n",task_list.size());
    return task_list.size();
  }
  /*---------------------------------------------------------------------------------*/
  bool canTerminate(){
    static long skip = 0;
    static TimeUnit t,cost=0;
    t = getTime();
    if ( skip < skip_term){
      skip ++;
      cost += getTime() - t;
      return false;
    }
    dt_log.addEventNumber(cost ,DuctteipLog::SkipOverhead);
    skip =  cost = 0 ;
    dt_log.addEventStart(this,DuctteipLog::CheckedForTerminate);
    if (elapsedTime(TIME_SCALE_TO_SECONDS)>time_out){
      if (checkRunningTasks(1) >0 || work_queue.size() >0){	
	dt_log.addEventEnd(this,DuctteipLog::CheckedForTerminate);
	if(0)printf("cRT:%ld\n",checkRunningTasks(1));
	return false;
      }
      TimeUnit Duration = elapsedTime(TIME_SCALE_TO_SECONDS);
      dt_log.addEventEnd(this,DuctteipLog::CheckedForTerminate);
      printf("\n error: timeout\n");
      return true;
    }
    if ( !net_comm->canTerminate() ) {
      dt_log.addEventEnd(this,DuctteipLog::CheckedForTerminate);
      return false;
    }
    if ( term_ok == TERMINATE_OK ) {
      dt_log.addEventEnd(this,DuctteipLog::CheckedForTerminate);
      return true;
    }
    
    
    PRINT_IF(0)("task count:%ld unf-lsnr:%d\n",getUnfinishedTasks(),isAnyUnfinishedListener());
    if (last_task_handle > 0 && getUnfinishedTasks() < 1){
      if ( net_comm->get_host_count() ==1 ) 
	return true;
      PRINT_IF(0)("task count:%ld last_lsnr_hdl:%ld ,unf-lsnr:%d\n",
		  getUnfinishedTasks(),last_listener_handle,isAnyUnfinishedListener());
      if (last_listener_handle > 0 && !isAnyUnfinishedListener() ){
	PRINT_IF(0)("task count:%ld unf-lsnr:%d, => sendTerminateOK\n",getUnfinishedTasks(),isAnyUnfinishedListener());
	sendTerminateOK();
      }
    }

    dt_log.addEventEnd(this,DuctteipLog::CheckedForTerminate);
    return false;
  }
  /*---------------------------------------------------------------------------------*/
  int  getTaskCount(){    return task_list.size();  }
  /*---------------------------------------------------------------------------------*/
  void show_affinity() {
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
        fprintf(stderr, "tid %d affinity %s\n", atoi(dirp->d_name), ss.str().c_str());
    }
    closedir(dp);
  }
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
    IDuctteipTask t;
    IListener l;

    long dps = ny * nx * sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
    dt_log.setParams(cfg->getProcessors(),dps,l.getPackSize(),t.getPackSize());
    if ( simulation ) {
      dps = sizeof(double) + dh.getPackSize() + 4*dv.getPackSize();
    }
    data_memory = new MemoryManager (  nb * mb/3 ,dps );
    int ipn = cfg->getIPN();
    printf("eng.ipn=%d, nt=%d\n",ipn,num_threads);
        thread_manager = new ThreadManager<Options> ( num_threads ,0* (me % ipn )  * 16/ipn) ;
    //    thread_manager = new ThreadManager<Options> ( num_threads , (me % 1 )  * 4) ;
    show_affinity();
    dt_log.N = cfg->getXDimension();
    dt_log.NB = nb;
    dt_log.nb = local_nb;
    dt_log.p = cfg->getP_pxq();
    dt_log.q = cfg->getQ_pxq();
    dt_log.cores = num_threads;

  }
  /*---------------------------------------------------------------------------------*/
  void prepareReceives(){
    IListener lsnr;
    mailbox->prepareListenerReceive(lsnr.getPackSize());
#if TERMINATE_TREE==0
    mailbox->prepareTerminateReceive();
#else
    const int TREE_BASE=2;
    int n,nodes[TREE_BASE];
    int parent = getParentNodeInTree(me);
    getChildrenNodesInTree(me,nodes,&n);
    
    mailbox->prepareTerminateReceive(parent,nodes[0],nodes[1]);
#endif


  }
  /*---------------------------------------------------------------------------------*/
  void doProcess(){

    prepareReceives();

    dt_log.addEventStart(this,DuctteipLog::ProgramExecution);
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
      printf("multiple  thread run, thread-id:%ld,result of setsched:%d,%d\n",pthread_self(),r1,r2);
    }
    else{
      printf("single thread run\n");
    }
  }
  /*---------------------------------------------------------------------------------*/
  static 
  void *doProcessLoop(void *p){
    engine *_this = (engine *)p;
    printf("do process loop started, thread-id:%ld\n",pthread_self());
    //    threadInfo(0);
    while(true){
      //nanoSleep();
      _this->waitForTaskFinish();
      _this->doProcessMailBox();
      _this->doProcessWorks();
      _this->doDLB();
      if ( _this->canTerminate() )
	break;
    }
    printf("do process loop finished, thread-id:%ld\n",pthread_self());

   pthread_exit(NULL);
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
  void putWorkForFinishedTask(IDuctteipTask * task){
    DuctTeipWork *work = new DuctTeipWork;
    work->task  = task;
    work->tag   = DuctTeipWork::TaskWork;
    work->event = DuctTeipWork::Finished;
    work->item  = DuctTeipWork::UpgradeData;
    work->host  = task->getHost();
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
    dt_log.addEvent(task,DuctteipLog::TaskReceived);

    criticalSection(Leave); 

  }
  /*---------------------------------------------------------------------------------*/
  void importedTask(MailBoxEvent *event){
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
    
    dt_log.addEvent(task,DuctteipLog::TaskImported);

    criticalSection(Leave); 
    if(1)printf("Task is imported:%s\n",task->getName().c_str());
    task->dump();

  }
  /*---------------------------------------------------------------------------------*/
  long int checkRunningTasks(int v=0){

    list<IDuctteipTask *>::iterator it;
    int cnt =0 ;
    long sc =0;
    PRINT_IF(OVERSUBSCRIBED)("rt cnt :%ld\n",running_tasks.size());
    for(it = running_tasks.begin(); it != running_tasks.end(); ){
      IDuctteipTask *task=(*it);
      PRINT_IF(0)("rt :%s,fin:%d,st:%d\n",task->getName().c_str(),task->isFinished(),task->getState());
      if(task->canBeCleared()){
	it = running_tasks.erase(it);	
	dt_log.logLoadChange(running_tasks.size());

	if(0)printf("RUNNING Tasks-:%ld\n",running_tasks.size());
	continue;
      }
      if (task->isFinished() ) {
	PRINT_IF(0)("rt :%s\n",task->getName().c_str());
	putWorkForFinishedTask(task);
	task->setState(IDuctteipTask::UpgradingData);
	cnt ++;
	PRINT_IF(OVERSUBSCRIBED)("task:%s erased from rt-list\n",task->getName().c_str());
      }
      else
	it++;
    }
    checkMigratedTasks();
    sc = cnt+import_tasks.size()+export_tasks.size()+work_queue.size();
    if ( sc && 0    )printf("Task queue size : %d,%ld,%ld,%ld %ld\n",cnt,
			  import_tasks.size(),export_tasks.size(),
			  work_queue.size(),task_list.size());
    return cnt+import_tasks.size()+export_tasks.size();
  }
  /*---------------------------------------------------------------------------------*/
  void  checkMigratedTasks(){
    checkImportedTasks();
    checkExportedTasks();
  }
  /*---------------------------------------------------------------------------------*/
  void  checkImportedTasks(){

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
	  printf("import task list emptied.\n");
	}
	dt_log.logImportTask(import_tasks.size());
      }
      else{
	task_it++;
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
  void  checkExportedTasks(){
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
  void doProcessWorks(){
    static int skip=0;
    static TimeUnit t,cost = 0;
    t = getTime();
    
    
    if ( skip < skip_work){
      skip ++;
      cost += getTime() - t;
      return ;      
    }
    if ( work_queue.size() < 1 ) {
      //putWorkForCheckAllTasks();
      return;
    }
    criticalSection(Enter); 
    dt_log.addEventNumber(cost,DuctteipLog::SkipOverhead);
    skip =cost = 0 ;
    
    dt_log.addEventStart(this,DuctteipLog::WorkProcessed);
    
    DuctTeipWork *work = work_queue.front();
    work_queue.pop_front();
    executeWork(work);    
    dt_log.addEventEnd(this,DuctteipLog::WorkProcessed);
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
  bool addListener(IListener *listener ){
    if (isDuplicateListener(listener))
          return false;
    int handle = last_listener_handle ++;
    listener_list.push_back(listener);
    listener->setHandle(handle);
    listener->setCommHandle(-1);
    dt_log.addEvent(listener,DuctteipLog::ListenerDefined);

    return true;
  }
  /*---------------------------------------------------------------------------------*/
  void checkTaskDependencies(IDuctteipTask *task){
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
	  if(0)printf("before prepare memory data:%p\n",data);
	  data->allocateMemory();
	  data->prepareMemory();
	  mailbox->prepareDataReceive(data->getDataMemory(),host,data->getDataHandleID());
	  dt_log.addEvent(lsnr,DuctteipLog::ListenerSent,host);
	  lsnr->setCommHandle ( comm_handle);
	}
	else{
	  delete lsnr;
	}
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
public:
  void runFirstActiveTask(){
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
private:
  /*---------------------------------------------------------------------------------*/
  long getActiveTasksCount(){
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
  void executeTaskWork(DuctTeipWork * work){
    switch (work->item){
    case DuctTeipWork::UpgradeData:
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
	dt_log.addEventStart(work->task,DuctteipLog::CheckedForRun);
	//printf("RUNNING Tasks?\n");
	if ( work->task->canRun('W') ) {
	  dt_log.addEventEnd(work->task,DuctteipLog::CheckedForRun);
	  running_tasks.push_back(work->task);
	  dt_log.logLoadChange(running_tasks.size());
	  if(0)printf("RUNNING Tasks#:%ld IMPORTED TASKS:%ld EXPORTED TASKS:%ld\n",
		 running_tasks.size(),import_tasks.size(),
		 export_tasks.size());
	  if (cfg->getDLB()){
	    if (getActiveTasksCount() < DLB_BUSY_TASKS ){
	      dt_log.addEventStart(work->task,DuctteipLog::Executed);
	      work->task->run();
	    }
	  }
	  else{
	    dt_log.addEventStart(work->task,DuctteipLog::Executed);
	    work->task->run();
	  }
	  return;  
	}
	dt_log.addEventEnd(work->task,DuctteipLog::CheckedForRun);
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
      list<IListener *>          d_listeners = work->data->getListeners();
      list<IDuctteipTask *>      d_tasks     = work->data->getTasks();
      //printf("after data upgrade, t_list:%ld\n",d_tasks.size()); work->data->dump('N');
      dt_log.addEventStart(work->data,DuctteipLog::CheckedForListener);
      for(lsnr_it = d_listeners.begin() ; 
	  lsnr_it != d_listeners.end()  ; 
	  ++lsnr_it){
	IListener *listener = (*lsnr_it);
	listener->checkAndSendData(mailbox);
	//dumpTasks();
      }
      dt_log.addEventEnd(work->data,DuctteipLog::CheckedForListener);
      dt_log.addEventStart(work->data,DuctteipLog::CheckedForTask);
      for(task_it = d_tasks.begin() ; 
	  task_it != d_tasks.end()  ;
	  ++task_it){
	IDuctteipTask *task = (*task_it);
	dt_log.addEventStart(task,DuctteipLog::CheckedForRun);
	if(0)printf("$data %s -> task:%s,stat=%d.\n",work->data->getName().c_str(),
	       task->getName().c_str(),task->getState());
	if (task->canRun('D')) {
	  dt_log.addEventEnd(task,DuctteipLog::CheckedForRun);
	  running_tasks.push_back(task);	  
	  dt_log.logLoadChange(running_tasks.size());
	  if(0)printf("RUNNING Tasks#:%ld IMPORTED TASKS:%ld EXPORTED TASKS:%ld\n",
		 running_tasks.size(),import_tasks.size(),
		 export_tasks.size());
	  if (cfg->getDLB()){
	    if(0)printf("##### running tasks :%ld\n",running_tasks.size() );
	  }	  
	  else{
	    dt_log.addEventStart(task,DuctteipLog::Executed);
	    task->run();
	  }
	  
	}
	else
	  dt_log.addEventEnd(task,DuctteipLog::CheckedForRun);
      }
      dt_log.addEventEnd(work->data,DuctteipLog::CheckedForTask);
      if (cfg->getDLB()){
	runFirstActiveTask();
      }
      break;
    }
  }
  /*---------------------------------------------------------------------------------*/
  void executeListenerWork(DuctTeipWork * work){
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
    bool wait= false,completed,found;
    static long int skip=0;
    static TimeUnit t,cost = 0;
    if (net_comm->get_host_count() == 1) 
      return ;
    t = getTime();
    if ( skip < skip_mbox){
      skip ++;
      cost += getTime() - t;
      return;
    }
    dt_log.addEventNumber(cost,DuctteipLog::SkipOverhead);
    skip = cost = 0;
    event.host =-1;
    event.tag = -1;
    dt_log.addEventStart(this,DuctteipLog::MailboxGetEvent);
    found =  mailbox->getEvent(data_memory,&event,&completed,wait);
    dt_log.addEventEnd(this,DuctteipLog::MailboxGetEvent);

    PRINT_IF(IRECV) ( "mbox proc after getvent,comp:%d,found:%d\n", completed , found); 
    if ( !found ){
      return ;
    }
    if (found &&  !completed ) {
      if ( event.tag != MailBox::MigrateTaskTag &&
	   event.tag != MailBox::MigrateDataTag &&
	   event.tag != MailBox::MigratedTaskOutDataTag 
	   ){
	printf("//////////////\n");
	return;
      }
    }
	  if (0){
	    if ( event.direction == MailBoxEvent::Received)
	    if ( event.tag == MailBox::MigrateDataTag ||
		 event.tag == MailBox::MigratedTaskOutDataTag ){
	      printf("+++sum i , ---------,ev.buf:%p ev.len:%d ev.tag:%d\n",event.buffer,event.length,event.tag);
	      double sum = 0.0,*contents=(double *)(event.buffer+192);
	      long size = (event.length-192)/sizeof(double);
	      for ( long i=0; i< size; i++)
		sum += contents[i];
	      printf("+++sum i , ---------,%lf adr:%p\n",sum,contents);
	    }
	  }
    processEvent(event);
  }
  /*---------------------------------------------------------------------------------*/
  void processEvent(MailBoxEvent &event){
    dt_log.addEventStart(this,DuctteipLog::MailboxProcessed);
    PRINT_IF(IRECV)("mbox after getEvent, found and completed.\n");
    dumpTime();
    event.dump();
    if ( (MailBox::MessageTag)event.tag != MailBox::DataTag) {
      PRINT_IF(IRECV)("mem InUse cleared,event:%d\n",event.tag);
      //m->setState(MemoryItem::Ready);
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
	  dt_log.addEvent(task,DuctteipLog::TaskSendCompleted);
	  TaskHandle task_handle = task->getHandle();
	  removeTaskByHandle(task_handle);
	}
	break;
    case MailBox::ListenerTag:
      if (event.direction == MailBoxEvent::Received) {
	receivedListener(&event);
      }
      else { 
	//printf("lsnr sent\n");
	IListener *listener = getListenerByCommHandle(event.handle);
	dt_log.addEvent(listener,DuctteipLog::ListenerSendCompleted); 
      }
      break;
    case MailBox::DataTag:
      if (event.direction == MailBoxEvent::Received) {
	PRINT_IF(0)("data received\n");
	receivedData(&event,event.getMemoryItem());
      }
      else{
	//printf("data sent\n");
	IListener *listener = getListenerByCommHandle(event.handle);
	dt_log.addEvent(listener->getData(),DuctteipLog::DataSendCompleted);
	listener->getData()->dataIsSent(listener->getSource());
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
	  if (0){
	    double sum = 0.0,*contents=(double *)(event.buffer+192);
	    long size = (event.length-192)/sizeof(double);
	    for ( long i=0; i< size; i++)
	      sum += contents[i];
	    printf("+++sum i , ---------,%lf adr:%p\n",sum,contents);
	  }
	importData(&event);
      }
      break;
    case MailBox::MigratedTaskOutDataTag:
      if ( event.direction == MailBoxEvent::Received ) {
	  if (0){
	    double sum = 0.0,*contents=(double *)(event.buffer+192);
	    long size = (event.length-192)/sizeof(double);
	    for ( long i=0; i< size; i++)
	      sum += contents[i];
	    printf("+++sum r , ---------,%lf adr:%p\n",sum,contents);
	  }
	receiveTaskOutData(&event);
      }
      break;

    }
    if ( event.direction  == MailBoxEvent::Received) {
      if (event.tag != MailBox::DataTag) {
	//delete event.buffer; //todo
      }
    }
    dt_log.addEventEnd(this,DuctteipLog::MailboxProcessed);
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
	if(0)printf("TL:%ld\n",task_list.size());
	criticalSection(Leave);
	return;
      }
    }
  }
  /*---------------------------------------------------------------------------------*/
  void criticalSection(int direction){
    if ( !runMultiThread)
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
    return NULL;
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

  /*---------------------------------------------------------------*/
  void resetTime(){
    start_time = getTime();
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
#if TERMINATE_TREE==0
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
    PRINT_IF(TERMINATE_FLAG)("term ok recv %d\n",from);
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
#endif // TERMINATE_TREE
  /*---------------------------------------------------------------------------------*/
  void createThreads(){
    /*
      init(admin)                          mt lk
      signal(admin,start)                  crt thrd
      sync(admin,start)                    cond wt
      
      init(mailbox)
      signal(mailbox,start)
      sync(mailbox,start)
      
      init(tgen) 
      signal(tgen,start)
      sync(tgen,start)

      //admin thread--------------------------------
      sync (main,started)
      notify(admin,started)
      while(true){//
      
         if (!finished(tgen)){
            syncTimed(tgen)                  cond timed wt
	    if(!timeOut){doProcessWork()...};
         }
	 if (true) {
	    syncTimed(mailbox)
	    if(!timeOut){doProcessMailbox()...}
	 } 
      }
      //tgen thread --------------------------------
         sync (main,started)
         signal(tgen,started)
         context.task_gen()
            AddTask(){
              enterCriticalSection(admin,task_list)
	      ...
              exitCriticalSection(admin,task_list)
              signal(admin,new_task)
            }

    */
  }
#if TERMINATE_TREE==1
  /*=================== Nodes Tree ===========================
                            0
                1                        2
         3            5            4             6
      7     9     11     13     8     10     12     14
    15 17 19 21 23  25 27  29 16 18 20  22 24  26 28  30
  ============================================================
   */
  int getParentNodeInTree(int node){
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
  void getChildrenNodesInTree(int node,int *nodes,int *count){
    
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
  bool amILeafInTree(){
    const int TREE_BASE=2; // binary tree
    int n,nodes[TREE_BASE];    
    getChildrenNodesInTree(me,nodes,&n);
    PRINT_IF(TERMINATE_FLAG)("is leaf %d?%d\n ",me,n==0);
    if ( n==0)
      return true;
    return false;
  }
  void sendTerminateOK(){
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
  void receivedTerminateOK(int from){
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
  void receivedTerminateCancel(int from){}
  void sendTerminateCancel(){}
#endif
  /*======================================================================*/
  struct DLB_Statistics{
    unsigned long tot_try,
      tot_failure,
      tot_tick,
      max_loc_fail,export_task,export_data,import_task,import_data,max_para,max_para2;
    ClockTimeUnit tot_cost,
      tot_silent;      
  };
  DLB_Statistics dlb_profile;
  int dlb_state,dlb_prev_state,dlb_substate,dlb_stage,dlb_node;
  unsigned long dlb_failure,dlb_glb_failure;
  ClockTimeUnit dlb_silent_start;
  enum DLB_STATE{
    DLB_IDLE,
    DLB_BUSY,
    TASK_EXPORTED,
    TASK_IMPORTED,
    DLB_STATE_NONE
  };
  enum DLB_SUBSTATE{
    ACCEPTED,
    EARLY_ACCEPT
  };
  enum DLB_STAGE{
    DLB_FINDING_IDLE,
    DLB_FINDING_BUSY,
    DLB_SILENT,
    DLB_NONE
  };
  enum Limits{
    SILENT_PERIOD=100,
    FAILURE_MAX=5
  };
  struct TimeDLB
  {
    ClockTimeUnit t;
    engine *e;
    TimeDLB(engine *_e):e(_e){t = getClockTime(MICRO_SECONDS);}
    ~TimeDLB(){
      e->dlb_profile.tot_cost += getClockTime(MICRO_SECONDS) - t;
    }
  };
  int failures[FAILURE_MAX];
#define TIME_DLB TimeDLB(this)
  /*---------------------------------------------------------------*/
  void initDLB(){
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
  void updateDLBProfile(){
    dlb_profile.tot_failure+=dlb_failure+dlb_glb_failure;
    dlb_profile.max_loc_fail=
      (dlb_profile.max_loc_fail >dlb_failure) ?
       dlb_profile.max_loc_fail :dlb_failure  ;
  }
  /*---------------------------------------------------------------*/
  void restartDLB(int s=-1){    
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
  void goToSilent(){
    // log_event start silent
    dlb_failure =0;
    dlb_silent_start = getClockTime(MICRO_SECONDS);
    dlb_stage = DLB_SILENT;
    PRINT_IF(DLB_DEBUG)("stage set to SILENT.\n");
  }
  bool passedSilentDuration(){
    return ((unsigned long)getClockTime(MICRO_SECONDS) > (unsigned long)SILENT_PERIOD ) ;
  }
  /*---------------------------------------------------------------*/
  void dumpDLB(){
    printf("DLB Stats:\n");
    printf("try:%ld fail:%ld tick:%ld decline:%ld\n",
	   dlb_profile.tot_try,
	   dlb_profile.tot_failure,
	   dlb_profile.tot_tick,
	   dlb_glb_failure);
    printf("loc fail:%ld cost:%ld silence:%ld\n",
	   dlb_profile.max_loc_fail,
	   dlb_profile.tot_cost/1000000L,
	   dlb_profile.tot_silent); 
    printf("DLB ex task:%ld ex data:%ld im task:%ld im data:%ld max para:%ld or %ld\n",
	   dlb_profile.export_task,
	   dlb_profile.export_data,
	   dlb_profile.import_task,
	   dlb_profile.import_data,
	   dlb_profile.max_para,
	   dlb_profile.max_para2);
  }
  /*---------------------------------------------------------------*/
  int getDLBStatus(){
    PRINT_IF(0&&DLB_DEBUG)("task#:%ld active:%ld\n",running_tasks.size(),getActiveTasksCount());
    if ( (running_tasks.size()-getActiveTasksCount() )> DLB_BUSY_TASKS){
      PRINT_IF(DLB_DEBUG)("BUSY\n");
      return DLB_BUSY;
    }
    PRINT_IF(DLB_DEBUG)("IDLE\n");
    return DLB_IDLE;
  }
  /*---------------------------------------------------------------*/
  void doDLB(int st=-1){
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
  void findBusyNode(){
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
  void findIdleNode(){
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
  void received_FIND_IDLE(int p){
    TIME_DLB;
    // log_event received find_idle
    if ( dlb_state == DLB_IDLE ) {
      acceptImportTasks(p);
      return;
    }
    PRINT_IF(DLB_DEBUG)("I'm not IDLE(%d) or not Silent(%d). Decline Import tasks.\n",dlb_state,dlb_stage);
    declineImportTasks(p);
  }
  void receivedImportRequest(int p){ received_FIND_IDLE(p);}
  /*---------------------------------------------------------------*/
  void received_FIND_BUSY(int p ) {
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
  void receivedExportRequest(int p){received_FIND_BUSY(p);}
  /*---------------------------------------------------------------*/
  void received_DECLINE(int p){
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
  void receivedDeclineMigration(int p){received_DECLINE(p); }
  /*---------------------------------------------------------------*/
  IDuctteipTask *selectTaskForExport(){
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
  void exportTasks(int p){
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
  void importData(MailBoxEvent *event){
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
  void receiveTaskOutData(MailBoxEvent *event){
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
  void receivedMigrateTask(MailBoxEvent *event){
    dlb_profile.import_task++;    
    importedTask(event);
    PRINT_IF(DLB_DEBUG)("received tasks from %d\n",event->host);
    dlb_stage = DLB_NONE;
    PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
    if (dlb_state == DLB_BUSY) {
      printf("error : DLB_State == BUSY but received TASKS.\n");
    }
    if(DLB_DEBUG)printf("%d\n",__LINE__);
    restartDLB();     
  }
  /*---------------------------------------------------------------*/
  void received_ACCEPTED(int p){
    TIME_DLB;
    dlb_stage = DLB_NONE;
    PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
    // log_event accepted (from=p)
    if(DLB_DEBUG)printf("Node %d accepted my export-req.\n",p);
    if ( dlb_state == DLB_IDLE ) {
       printf("error : DLB_State == IDLE  but received ACCEPTED.\n");
       return;
    }
    if ( dlb_substate == EARLY_ACCEPT ) {
      if(DLB_DEBUG)printf("I'm in EARLY ACCEPT.\n");
      if ( dlb_node == p ){
	printf("%d\n",__LINE__);
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
  void declineImportTasks(int p){    
    dlb_glb_failure ++;
    // log_event decline import (p)
    PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
    mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
  }
  /*---------------------------------------------------------------*/
  void declineExportTasks(int p){    
    dlb_glb_failure ++;
    // log_event decline export (p)
    PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
    mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
  }
  /*---------------------------------------------------------------*/
  void acceptImportTasks(int p){    
    // log_event accept import  (p)
    dlb_substate = ACCEPTED;
    dlb_state = TASK_IMPORTED;
    PRINT_IF(DLB_DEBUG)("send Accept  to :%d\n",p);
    mailbox->send((byte*)&p,1,MailBox::AcceptMigrateTag,p);
  }
  /*---------------------------------------------------------------*/
private:
  vector<int> dlb_fail_list;
public:
  bool isInList(vector<int>&L,int v){
    vector<int>::iterator it;
    for (it=L.begin() ;it!=L.end();it++) {
      if (*it == v ) 
	return true;
    }
    return false;
  }
  int getRandomNodeEx(){
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
  int getRandomNodeOld(int exclude =-1){
    srand(time(NULL));
    int np = net_comm->get_host_count();
    int p=me;
    while  ( p== me || p == exclude)
      p = rand() % np ;
    return p;
  }
  /*---------------------------------------------------------------------------------*/



};


#endif //__ENGINE_HPP__
