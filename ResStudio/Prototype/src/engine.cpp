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
  memory_policy = ENGINE_ALLOCATION;
  runMultiThread=true;
  dlb.initDLB();
  thread_model = 0;
  task_submission_finished=false;
  data_memory = NULL;
  thread_manager = NULL;
  LOG_INFO(LOG_MULTI_THREAD,"mpi tick :%f\n",MPI_Wtick());
}
/*---------------------------------------------------------------------------------*/
template < typename T> 
void engine::set_superglue(ThreadManager<T> *sg){
  thread_manager = sg;
}
/*---------------------------------------------------------------------------------*/
void engine::initComm(){
  net_comm->initialize();
  resetTime();
  me = net_comm->get_host_id();
}
/*---------------------------------------------------------------------------------*/
engine::~engine(){
  if (net_comm)
    delete net_comm;
  net_comm = NULL;
  //  if ( thread_manager)    delete thread_manager;
  thread_manager = NULL;
  if (data_memory)
    delete data_memory;
  data_memory = NULL;
}
/*---------------------------------------------------------------------------------*/
int engine::getLocalNumBlocks(){return local_nb;}

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
  printf("Program finished at %ld.\n",UserTime());
  
  LOG_INFO(LOG_MULTI_THREAD,"Non-Busy time:%lf\n",wasted_time/1000000.0);
  if (true || cfg->getYDimension() == 2400){
    char s[20];
    sprintf(s,"sg_log_file-%2.2d.txt",me);
    Log<Options>::dump(s);
  }
  long tc=-1;//thread_manager->getTaskCount();

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
void engine::dumpTime(char c){}
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
void engine::set_memory_policy(int p){memory_policy= p;}
/*---------------------------------------------------------------------------------*/
void engine::setConfig(Config *cfg_,bool sg){
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
  LOG_INFO(LOG_MULTI_THREAD,"DataPackSize:%ld=%d*%d*8+%d+%d\n",dps,ny,nx,dh.getPackSize(),4*dv.getPackSize());
  if ( memory_policy == ALL_USER_ALLOCATED)
    data_memory = new MemoryManager (  nb * mb ,1 );
  else
    data_memory = new MemoryManager (  nb * mb ,dps );
  LOG_INFO(LOG_MULTI_THREAD,"data meory:%p\n",data_memory);
  //int ipn = cfg->getIPN();
  if ( !sg)
    thread_manager = new ThreadManager<Options> ( num_threads );//,0* (me % ipn )  * 16/ipn) ;
  
  show_affinity();
  dlb.initDLB();
}
/*---------------------------------------------------------------------------------*/
void engine::updateDurations(IDuctteipTask *task){
  long k = task->getKey();
  double dur = task->getDuration()/1000000.0;
  avg_durations[k]=  (avg_durations[k] * cnt_durations[k]+dur)/(cnt_durations[k]+1);
  cnt_durations[k]=cnt_durations[k]+1;
}
/*---------------------------------------------------------------------------------*/
long engine::getAverageDur(long k){
  return avg_durations[k];
}
/*---------------------------------------------------------------------------------*/
void engine::dumpAll(){
  printf("------------------------------------------------\n");
  dumpTasks();
  dumpListeners();
  printf("------------------------------------------------\n");
}
/*---------------------------------------------------------------*/
void engine::resetTime(){
  start_time = getTime();
}
/*--------------------------------------------------------------------------*/
void engine::start ( int argc , char **argv,bool sg){
  int P,p,q;

  printf("Engine start\n");
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
  setConfig(&config,sg);
  doProcess();
}
/*---------------------------------------------------------------*/
void engine::doDLB(int st){
  static long last_count;
  return;
  long c =  running_tasks.size();
  if ( c && c!= last_count )    return;
  LOG_INFO(0&LOG_DLB,"task count last:%ld, cur:%ld\n",last_count ,c);
  last_count = c;
  dlb.doDLB(st);
}

