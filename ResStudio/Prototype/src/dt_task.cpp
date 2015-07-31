#include "dt_task.hpp"
#include "data.hpp"
#include "glb_context.hpp"
#include "context.hpp"


#define SUBTASK 0
/*--------------------------------------------------------------------------*/
IDuctteipTask::IDuctteipTask (IContext *context,
			      string _name,
			      unsigned long _key,
			      int _host, 
			      list<DataAccess *> *dlist):
  host(_host),data_list(dlist){
    
  parent_context = context;    
  key = _key;
  if (_name.size() ==0  )
    _name.assign("task");
  setName(_name);
  comm_handle = 0 ;    
  state = WaitForData; 
  type = NormalTask;
  message_buffer = new MessageBuffer ( getPackSize(),0);
  sg_handle = new Handle<Options>;
  pthread_mutexattr_init(&task_finish_ma);
  pthread_mutexattr_settype(&task_finish_ma,PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&task_finish_mx,&task_finish_ma);
  exported = imported = false;
#if SUBTASK ==1
  sg_task = new KernelTask(this);
#endif

}
/*--------------------------------------------------------------------------*/
IDuctteipTask::~IDuctteipTask(){
  if ( message_buffer != NULL ) 
    delete message_buffer;
  if ( sg_handle != NULL ) 
    delete sg_handle;
  pthread_mutex_destroy(&task_finish_mx);
  pthread_mutexattr_destroy(&task_finish_ma);
}  
/*--------------------------------------------------------------------------*/
IDuctteipTask::IDuctteipTask():name(""),host(-1){
  state = WaitForData;
  type = NormalTask;
  message_buffer = NULL;
  sg_handle = NULL;
  exported = imported = false;
  //    sg_task = new KernelTask(this);    
}
/*--------------------------------------------------------------------------*/
void IDuctteipTask::createSyncHandle(){sg_handle = new Handle<Options>;}
/*--------------------------------------------------------------------------*/
Handle<Options> *IDuctteipTask::getSyncHandle(){return sg_handle;}
/*--------------------------------------------------------------------------*/
DuctTeip_Data *IDuctteipTask::getArgument(int index){
  return (DuctTeip_Data *)getDataAccess(index);
}
/*--------------------------------------------------------------------------*/
IData *IDuctteipTask::getDataAccess(int index){
  if (index <0 || index >= data_list->size()){
    return NULL;
  }
  list<DataAccess *> :: iterator it;
  for ( it = data_list->begin(); it != data_list->end() && index >0 ; it ++,index--){
  }
  return (*it)->data;
}
/*--------------------------------------------------------------------------*/
void    IDuctteipTask::setHost(int h )    { host = h ;  }
int     IDuctteipTask::getHost()          { return host;}
string  IDuctteipTask::getName()          { return name + '-'+ to_string(handle);}
ulong   IDuctteipTask::getKey(){ return key;}
void    IDuctteipTask::setHandle(TaskHandle h)     { handle = h;}
TaskHandle IDuctteipTask::getHandle()                 {return handle;}
void          IDuctteipTask::setCommHandle(ulong h) { comm_handle = h;   }
unsigned long IDuctteipTask::getCommHandle()                { return comm_handle;}

/*--------------------------------------------------------------------------*/
list<DataAccess *> *IDuctteipTask::getDataAccessList() { return data_list;  }

/*--------------------------------------------------------------------------*/
void IDuctteipTask::dumpDataAccess(list<DataAccess *> *dlist){
  if ( !DUMP_FLAG)
    return;
  list<DataAccess *>::iterator it;
  for (it = dlist->begin(); it != dlist->end(); it ++) {
    printf("#daxs:%s @%d \n req-version:", (*it)->data->getName().c_str(),(*it)->data->getHost());
    (*it)->required_version.dump();
    printf("for data \n");
    (*it)->data->dump('N');
  }
  printf ("\n");
}
/*--------------------------------------------------------------------------*/
bool IDuctteipTask::isFinished(){ return (state == Finished);}
int  IDuctteipTask::getState  (){ return state;}
void IDuctteipTask::setState  (int s) { state = s;}
/*--------------------------------------------------------------------------*/
void  IDuctteipTask::setName(string n ) {
  name = n ;
}
/*--------------------------------------------------------------------------*/
int IDuctteipTask::getPackSize(){
  return
    sizeof(TaskHandle) +
    sizeof(int)+
    //data_list->size() // todo
    3* (sizeof(DataAccess)+sizeof(int)+sizeof(bool)+sizeof(byte));

}
/*--------------------------------------------------------------------------*/
bool IDuctteipTask::canBeCleared() { return state == CanBeCleared;}
bool IDuctteipTask::isUpgrading()  { return state == UpgradingData;}
/*--------------------------------------------------------------------------*/
bool IDuctteipTask::canRun(char c){
  list<DataAccess *>::iterator it;
  bool dbg=  (c !=' ')|| DLB_MODE;
  bool result=true;
  char stats[8]="WRFUC";
  TimeUnit t= getTime();
  static TimeUnit tot=0;
  char xi=isExported()?'X':(isImported()?'I':' ');
  if ( isExported() ) {
    LOG_INFO(LOG_TASKS,"Task is exported.\n");
    return false;
  }
  if ( state == Finished ) {
    LOG_INFO(LOG_TASKS,"Task already finished.\n");
    return false;
  }
  if ( state >= Running ){
    LOG_INFO(LOG_TASKS,"Task is still running.\n");
    return false;
  }
  if (0 && dbg)printf("**task %s dep :  state:%d\n",	   getName().c_str(),	   state);
  string s1,s2;
  s1=getName();s2="            ";
  for ( it = data_list->begin(); it != data_list->end(); ++it ) {
    if(DLB_MODE){
      s1+=" - "+(*it)->data->getName()+" , "+(*it)->required_version.dumpString();
      s2+="             " +(*it)->data->getRunTimeVersion((*it)->type).dumpString();
      /*
	printf("  **data:%s \n",  (*it)->data->getName().c_str());
	printf("  cur-version:%s\n",(*it)->data->getRunTimeVersion((*it)->type).dumpString().c_str());
	printf("  req-version:%s\n",(*it)->required_version.dumpString().c_str());
      */
    }
      
      
    if ( (*it)->data->getRunTimeVersion((*it)->type) != (*it)->required_version ) {
      /*
	LOG_INFO(LOG_MULTI_THREAD ,"(%c)Task %s data %s is not ready,rt:ver:%s  rq-ver:%s.\n",
	c,
	getName().c_str(),
	(*it)->data->getName().c_str(),
	(*it)->data->getRunTimeVersion((*it)->type).dumpString().c_str(),
	(*it)->required_version.dumpString().c_str());
      */
      result=false;      
    }
  }
  if (DLB_MODE){
    if(dbg)printf("%c(%c)%s %c%c\n%s %ld\n",c,result?'+':'-',s1.c_str(),stats[state-2],xi,s2.c_str(),getTime());
    else   printf("%c[%c]%s %c%c\n%s %ld\n",c,result?'+':'-',s1.c_str(),stats[state-2],xi,s2.c_str(),getTime());
  }
  tot += getTime() - t;
  //    LOG_INFO(LOG_MULTI_THREAD ,"Local cost %ld\n",tot);
  return result;
}
/*--------------------------------------------------------------------------*/
void IDuctteipTask::setExported(bool f) { exported=f;}
/*--------------------------------------------------------------------------*/
bool IDuctteipTask::isExported(){return exported;}
/*--------------------------------------------------------------------------*/
void IDuctteipTask::setImported(bool f) { imported=f;}
/*--------------------------------------------------------------------------*/
bool IDuctteipTask::isImported(){return imported;}
bool IDuctteipTask::isRunning(){return state == Running ;}
/*--------------------------------------------------------------------------*/
void IDuctteipTask::dump(char c){
  if ( !DUMP_FLAG)
    return;
  //if ( c!='i') return;
  printf("#task:%s key:%lx ,no.of data:%ld state:%d expo:%d impo:%d\n ",
	 name.c_str(),key,data_list->size(),state,exported,imported);
  if (type == PropagateTask)
    prop_info->dump();
  else
    dumpDataAccess(data_list);
}
/*--------------------------------------------------------------*/
IDuctteipTask::IDuctteipTask(PropagateInfo *p){
  prop_info = p;
  type = PropagateTask;
  data_list = new list<DataAccess*>;
  DataAccess *daxs = new DataAccess;
  daxs->data = glbCtx.getDataByHandle(&p->data_handle);  
  daxs->required_version = p->fromVersion;
  daxs->required_version.setContext( p->fromCtx);
  daxs->type = IData::READ;
  data_list->push_back(daxs);  
}
/*--------------------------------------------------------------*/
MessageBuffer *IDuctteipTask::serialize(){
  int offset =0 ;
  serialize(message_buffer->address,offset,message_buffer->size);
  return message_buffer;
}
/*--------------------------------------------------------------*/
int IDuctteipTask::serialize(byte *buffer,int &offset,int max_length){
  int count =data_list->size();
  list<DataAccess *>::iterator it;
  copy<unsigned long>(buffer,offset,key);
  ContextHandle handle=*parent_context->getContextHandle();
  copy<ContextHandle>(buffer,offset,handle);
  copy<int>(buffer,offset,count);
  for ( it = data_list->begin(); it != data_list->end(); ++it ) {
    (*it)->data->getDataHandle()->serialize(buffer,offset,max_length);
    (*it)->required_version.serialize(buffer,offset,max_length);
    byte type = (*it)->type;
    copy<byte>(buffer,offset,type);
  }
  return 0;
}
/*--------------------------------------------------------------*/
void IDuctteipTask::deserialize(byte *buffer,int &offset,int max_length){
  paste<unsigned long>(buffer,offset,&key);
  size_t len = sizeof(unsigned long);
  ContextHandle handle ;
  paste<ContextHandle>(buffer,offset,&handle);
  parent_context = glbCtx.getContextByHandle(handle);
  name=parent_context->getTaskName(key);
  int count ;
  paste<int>(buffer,offset,&count);
  data_list = new list<DataAccess *>;
  for ( int i=0; i<count; i++){
    DataAccess *data_access = new DataAccess;
    DataHandle *data_handle = new DataHandle;
    data_handle->deserialize(buffer,offset,max_length);
    IData *data = glbCtx.getDataByHandle(data_handle);
    data_access->required_version.deserialize(buffer,offset,max_length);
    paste<byte>(buffer,offset,&data_access->type);
    data_access->data = data;
    data_list->push_back(data_access);
  }
}
/*--------------------------------------------------------------*/
void IDuctteipTask::run(){
  if ( state == Finished ) 
    return;
  if ( state == Running ) 
    return;
  state = Running;
  start = getTime();
  exp_fin = dtEngine.getAverageDur(getKey())+start;
#if SUBTASK ==1
  dtEngine.getThrdManager()->submit(sg_task);
#else
  parent_context->runKernels(this);
#endif
}
/*--------------------------------------------------------------*/
void IDuctteipTask::setFinished(bool flag){
  if ( !flag ) 
    return ;
  state = IDuctteipTask::Finished;
  end = getTime();
  parent_context->taskFinished(this,end-start);

  LOG_EVENTX(DuctteipLog::TaskFinished,this);
  //  LOG_INFO(LOG_MULTI_THREAD,"%s\n",getName().c_str());
  dtEngine.signalWorkReady(this);
}
/*--------------------------------------------------------------*/
void IDuctteipTask::upgradeData(char c){
  list<DataAccess *>::iterator it;
  if ( type == PropagateTask ) 
    return;
  for ( it = data_list->begin(); it != data_list->end(); ++it ) {
    
    LOG_INFO(LOG_TASKS,"data:%s,%Ld\n",
	     (*it)->data->getName().c_str(),
	     dtEngine.elapsedTime(TIME_SCALE_TO_MILI_SECONDS));
    (*it)->data->incrementRunTimeVersion((*it)->type);
    /*
      LOG_INFO(LOG_MULTI_THREAD,"%s, data:%s, rt-ver:%s\n",
      getName().c_str(),
      (*it)->data->getName().c_str(),
      (*it)->data->getRunTimeVersion((*it)->type).dumpString().c_str());
    */
    
    LOG_EVENTX(DuctteipLog::RunTimeVersionUpgraded,(*it)->data);    

  }
  state = CanBeCleared;
  dtEngine.putWorkForDataReady(data_list);
}
/*--------------------------------------------------------------*/
KernelTask::KernelTask(IDuctteipTask *t):dt_task(t){
  register_access(ReadWriteAdd::read, *dt_task->getSyncHandle());
}
/*--------------------------------------------------------------*/
KernelTask::~KernelTask(){
  dt_task->setFinished(true);
}
/*--------------------------------------------------------------*/
void KernelTask::run(TaskExecutor<Options> &te){
  dt_task->setTaskExecutor(&te);
  dt_task->getParentContext()->runKernels(dt_task);
}
/*--------------------------------------------------------------*/
string KernelTask::get_name(){
  return dt_task->getName() + "Kernel";
}
/*--------------------------------------------------------------*/
IContext *IDuctteipTask::getParentContext(){return parent_context;}
/*--------------------------------------------------------------*/
TaskExecutor<Options> *IDuctteipTask::getTaskExecutor(){return te;}
/*--------------------------------------------------------------*/
void IDuctteipTask::setTaskExecutor(TaskExecutor<Options> *tt){te=tt;}
/*--------------------------------------------------------------*/
TaskBase<Options>*IDuctteipTask::getKernelTask(){return sg_task;}
/*--------------------------------------------------------------*/
TimeUnit IDuctteipTask::getExpFinish(){  return exp_fin;}
/*--------------------------------------------------------------*/
ulong  IDuctteipTask::getMigrateSize(){  
  return 0L;
}
/*--------------------------------------------------------------*/
TimeUnit IDuctteipTask::getDuration(){return end-start;}
