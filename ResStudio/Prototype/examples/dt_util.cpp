#include "dt_util.hpp"
/*----------------------------------------------------------------------------*/
SyncTask::SyncTask(IDuctteipTask *task_):dt_task(task_) {
  //LOG_INFO(LOG_MULTI_THREAD,"sync task ctor,task:%s\n",dt_task->getName().c_str());
  registerAccess(ReadWriteAdd::write, *dt_task->getSyncHandle());
  ostringstream os;
  os << setfill('0') << setw(4) <<dt_task->getHandle() << " sg_sync ";
  log_name = os.str();
}
/*----------------------------------------------------------------------------*/
SyncTask::SyncTask(Handle<Options> **h,int M,int N,IDuctteipTask *task_):dt_task(task_) {
  for(int i =0;i<M;i++)
    for (int j=0;j<N;j++)
      registerAccess(ReadWriteAdd::write, h[i][j]);
  //LOG_INFO(LOG_MULTI_THREAD,"sync on all handles:%d,%d for:%s\n",M,N,dt_task->getName().c_str());
}
/*----------------------------------------------------------------------------*/
void SyncTask::run(TaskExecutor<Options> &te) {  }
/*----------------------------------------------------------------------------*/
SyncTask::~SyncTask(){
  //LOG_INFO(LOG_MULTI_THREAD,"[%ld] : sg_sync task for:%s.\n",pthread_self(),
//	   dt_task->getName().c_str());
  dt_task->setFinished(true);
}
/*----------------------------------------------------------------------------*/
string SyncTask::get_name(){ return log_name;}
/*----------------------------------------------------------------------------*/
