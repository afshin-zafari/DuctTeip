#include "dlb.hpp"
/*---------------------------------------------------------------*/
void DLB::initDLB(){
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
void DLB::updateDLBProfile(){
  dlb_profile.tot_failure+=dlb_failure+dlb_glb_failure;
  dlb_profile.max_loc_fail=
    (dlb_profile.max_loc_fail >dlb_failure) ?
    dlb_profile.max_loc_fail :dlb_failure  ;
}
/*---------------------------------------------------------------*/
void DLB::restartDLB(int s){    
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
void DLB::goToSilent(){
  // log_event start silent
  dlb_failure =0;
  dlb_silent_start = getClockTime(MICRO_SECONDS);
  dlb_stage = DLB_SILENT;
  PRINT_IF(DLB_DEBUG)("stage set to SILENT.\n");
}
/*---------------------------------------------------------------*/
bool DLB::passedSilentDuration(){
  return ((unsigned long)getClockTime(MICRO_SECONDS) > (unsigned long)SILENT_PERIOD ) ;
}
/*---------------------------------------------------------------*/
void DLB::dumpDLB(){
  fprintf(stderr,"DLB Stats:\n");
  fprintf(stderr,"try:%ld fail:%ld tick:%ld decline:%ld\n",
	  dlb_profile.tot_try,
	  dlb_profile.tot_failure,
	  dlb_profile.tot_tick,
	  dlb_glb_failure);
  fprintf(stderr,"loc fail:%ld cost:%ld silence:%ld\n",
	  dlb_profile.max_loc_fail,
	  dlb_profile.tot_cost/1000000L,
	  dlb_profile.tot_silent); 
  fprintf(stderr,"DLB ex task:%ld ex data:%ld im task:%ld im data:%ld max para:%ld or %ld\n",
	  dlb_profile.export_task,
	  dlb_profile.export_data,
	  dlb_profile.import_task,
	  dlb_profile.import_data,
	  dlb_profile.max_para,
	  dlb_profile.max_para2);
}
/*---------------------------------------------------------------*/
int DLB::getDLBStatus(){
  PRINT_IF(0&&DLB_DEBUG)("task#:%ld active:%ld\n",dtEngine.running_tasks.size(),getActiveTasksCount());
  if ( (dtEngine.running_tasks.size()-getActiveTasksCount() )> DLB_BUSY_TASKS){
    PRINT_IF(DLB_DEBUG)("BUSY\n");
    return DLB_BUSY;
  }
  PRINT_IF(DLB_DEBUG)("IDLE\n");
  return DLB_IDLE;
}
/*---------------------------------------------------------------*/
void DLB::doDLB(int st){
  TIME_DLB;
  if (!dtEngine.cfg->getDLB())
    return;
  if (dtEngine.net_comm->get_host_count()==1) 
    return;
  if ( dtEngine.task_list.size() ==0 && dtEngine.running_tasks.size() ==0 ) 
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
void DLB::findBusyNode(){
  if ( dlb_stage == DLB_FINDING_BUSY ) 
    return;
  dlb_profile.tot_try++;
  dlb_stage = DLB_FINDING_BUSY ;
  dlb_node = getRandomNodeEx();    
  // log_event find_busy (dlb_node)
  PRINT_IF(DLB_DEBUG)("send FIND_BUSY to :%d\n",dlb_node);
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindBusyTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::findIdleNode(){
  if ( dlb_stage == DLB_FINDING_IDLE ) 
    return;
  dlb_profile.tot_try++;
  dlb_stage = DLB_FINDING_IDLE ;
  dlb_node = getRandomNodeEx();    
  // log_event find_idle (dlb_node)
  PRINT_IF(DLB_DEBUG)("send FIND_IDLE to :%d\n",dlb_node);
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindIdleTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::received_FIND_IDLE(int p){
  if ( dlb_state == DLB_IDLE ) {
    acceptImportTasks(p);
    return;
  }
  PRINT_IF(DLB_DEBUG)("I'm not IDLE(%d) or not Silent(%d). Decline Import tasks.\n",dlb_state,dlb_stage);
  declineImportTasks(p);
}
void DLB::receivedImportRequest(int p){ received_FIND_IDLE(p);}
/*---------------------------------------------------------------*/
void DLB::received_FIND_BUSY(int p ) {
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
void DLB::receivedExportRequest(int p){received_FIND_BUSY(p);}
/*---------------------------------------------------------------*/
void DLB::received_DECLINE(int p){
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
void DLB::receivedDeclineMigration(int p){received_DECLINE(p); }
/*---------------------------------------------------------------*/
IDuctteipTask *DLB::selectTaskForExport(){
  list<IDuctteipTask*>::iterator it;
  for(it=dtEngine.running_tasks.begin();it != dtEngine.running_tasks.end();it++){
    IDuctteipTask *t = *it;
    if(0)printf(" %d,%d ",t->getState(),t->isExported());
    if (t->isExported()) continue;
    if (t->isFinished()) continue;
    if (t->isRunning()) continue;
    if (t->isUpgrading()) continue;
    if (t->canBeCleared()) continue;
    dtEngine.export_tasks.push_back(t);
    LOG_LOAD;

    if(DLB_DEBUG)printf("Task is moved from running list to export:\n");t->dump();
    dtEngine.running_tasks.erase(it);
    LOG_LOAD;
    if(DLB_DEBUG)printf("RUNNING Tasks.:%ld\n",dtEngine.running_tasks.size());
    return t;
  }
  if(DLB_DEBUG)printf("\n");
  return NULL;
}
/*---------------------------------------------------------------*/
bool DLB::isFinishedEarlier(IDuctteipTask *task){
  TimeUnit loc_fin = task->getExpFinish();
  ulong comm_size=  task->getMigrateSize();
  double bw=dtEngine.net_comm->getBandwidth();
  double remote_fin =   comm_size / bw + dtEngine.avg_durations[task->getKey()] ;
  if ( remote_fin < loc_fin) 
    return true;
  return false;
}
/*---------------------------------------------------------------*/
void DLB::exportTasks(int p){
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

      dtEngine.mailbox->send(data->getHeaderAddress(),
			     data->getPackSize(),
			     MailBox::MigrateDataTag,
			     p);
    }
  }while( t !=NULL);
}
/*---------------------------------------------------------------*/
void DLB::importData(MailBoxEvent *event){
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
  for(it=dtEngine.import_tasks.begin();it != dtEngine.import_tasks.end(); it ++){
    IDuctteipTask *t = *it;
    if ( t->canRun('i')){
      if(DLB_DEBUG)printf("Task can run:\n");t->dump();
      t->run();	
    }
  }
}
/*---------------------------------------------------------------*/
void DLB::receiveTaskOutData(MailBoxEvent *event){
  dlb_profile.import_data++;
  IData *data = importedData(event,event->getMemoryItem());
  data->dumpCheckSum('r');
  data->dump(' ');
  list<IDuctteipTask *>::iterator it;
  for(it=dtEngine.export_tasks.begin();it != dtEngine.export_tasks.end(); it ++){
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
	    it = dtEngine.export_tasks.erase(it);
	    LOG_LOAD;
	    if(0)printf("rcvd task-out-data:%s,%s t_data:%s\n",
			task->getName().c_str(),
			data->getName().c_str(),
			t_data->getName().c_str());
	    task->setState(IDuctteipTask::CanBeCleared);
	    data->checkAfterUpgrade(dtEngine.running_tasks,dtEngine.mailbox,'r');
	    dtEngine.putWorkForCheckAllTasks();
	    return;
	  }
	}	
      }
    }
  }

}
/*---------------------------------------------------------------*/
void DLB::receivedMigrateTask(MailBoxEvent *event){
  dlb_profile.import_task++;    
  importedTask(event);
  PRINT_IF(DLB_DEBUG)("received tasks from %d\n",event->host);
  dlb_stage = DLB_NONE;
  PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
  if (dlb_state == DLB_BUSY) {
    fprintf(stderr,"error : DLB_State == BUSY but received TASKS.\n");
  }
  if(DLB_DEBUG)printf("%d\n",__LINE__);
  restartDLB();     
}
/*---------------------------------------------------------------*/
void DLB::received_ACCEPTED(int p){
  TIME_DLB;
  dlb_stage = DLB_NONE;
  PRINT_IF(DLB_DEBUG)("stage reset to NONE %d.\n",__LINE__);
  // log_event accepted (from=p)
  if(DLB_DEBUG)printf("Node %d accepted my export-req.\n",p);
  if ( dlb_state == DLB_IDLE ) {
    fprintf(stderr,"error : DLB_State == IDLE  but received ACCEPTED.\n");
    return;
  }
  if ( dlb_substate == EARLY_ACCEPT ) {
    if(DLB_DEBUG)printf("I'm in EARLY ACCEPT.\n");
    if ( dlb_node == p ){
      fprintf(stderr,"%d\n",__LINE__);
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
void DLB::declineImportTasks(int p){    
  dlb_glb_failure ++;
  PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::declineExportTasks(int p){    
  dlb_glb_failure ++;
  PRINT_IF(DLB_DEBUG)("send decline  to :%d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::acceptImportTasks(int p){    
  // log_event accept import  (p)
  dlb_substate = ACCEPTED;
  dlb_state = TASK_IMPORTED;
  PRINT_IF(DLB_DEBUG)("send Accept  to :%d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::AcceptMigrateTag,p);
}
/*---------------------------------------------------------------------------------*/
void DLB::importedTask(MailBoxEvent *event){
  IDuctteipTask *task = new IDuctteipTask;
  int offset = 0 ;
  task->deserialize(event->buffer,offset,event->length);
  task->setHost ( me ) ;
  task->setImported(true);
  task->createSyncHandle();
  dtEngine.criticalSection(engine::Enter); 

  task->setHandle( dtEngine.last_task_handle ++);
  dtEngine.import_tasks.push_back(task);
  LOG_LOAD;
  
  LOG_EVENT(DuctteipLog::TaskImported);

  dtEngine.criticalSection(engine::Leave); 
  if(0)fprintf(stderr,"Task is imported:%s\n",task->getName().c_str());
  task->dump();

}

/*---------------------------------------------------------------*/
bool DLB::isInList(vector<int>&L,int v){
  vector<int>::iterator it;
  for (it=L.begin() ;it!=L.end();it++) {
    if (*it == v ) 
      return true;
  }
  return false;
}
int DLB::getRandomNodeEx(){
  vector<int> &L=dlb_fail_list;
  vector<int>::iterator it;
  //    for (it=L.begin() ;it!=L.end();it++) printf(" %d ",*it);printf("<-\n");
  int np = dtEngine.net_comm->get_host_count();
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
int DLB::getRandomNodeOld(int exclude){
  srand(time(NULL));
  int np = dtEngine.net_comm->get_host_count();
  int p=me;
  while  ( p== me || p == exclude)
    p = rand() % np ;
  return p;
}
/*---------------------------------------------------------------------------------*/
void DLB:: exportTask(IDuctteipTask* task,int destination){
  MessageBuffer *m = task->serialize();
  unsigned long ch =  dtEngine.mailbox->send(m->address,m->size,MailBox::MigrateTaskTag,destination);
  task->setCommHandle (ch);
  fprintf(stderr,"Task is exported:%s\n",task->getName().c_str());
  task->dump();
}
/*--------------------------------------------------------------------------*/
IData * DLB::importedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;

  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }

  dh.deserialize(event->buffer,offset,event->length);
  IData *data = glbCtx.getDataByHandle(&dh);
  void dumpData(double *, int ,int, char);
  if (0){
    double sum = 0.0,*contents=(double *)(event->buffer+192);
    long size = (event->length -192)/sizeof(double);
    for ( long i=0; i< size; i++)
      sum += contents[i];
    printf("+++sum i , -----,%lf adr:%p,%p,%p\n",
	   sum,contents,event->memory->getAddress(),event->buffer);
  }

  if ( 0 ) {
    printf("Buffer:%p Header size:%d\n",event->buffer,data->getHeaderSize());
    double *A=(double *)(event->buffer+data->getHeaderSize());
    for ( int i=0;i<5;i++)
      for (int j=0;j<5;j++){
	dumpData(A+i*12*12+j*5*12*12,12,12,'I');
      }
  }
  if(0)printf("ev.mem dump:\n   ");
  event->memory->dump();
  if(0)printf("data-before.mem dump:\n   ");
  mi = data->getDataMemory();
  if ( mi != NULL ) {
    mi->dump();
    memcpy(data->getContentAddress(),event->buffer+data->getHeaderSize(),data->getContentSize());
  }
  else{
    if(0)printf("NULL\n");
    data->setDataMemory(event->memory);
    data->setContentSize(event->length-data->getHeaderSize());
    mi = data->getDataMemory();
    data->prepareMemory();
  }
  if (0 ) {
    MemoryItem *mi2 = data->getDataMemory();
    printf("data-after.mem dump:\n   ");
    mi2->dump();
  }
  if ( mi == NULL) {
    fprintf(stderr,"preparememory.///////////////////////////////////////////////// \n");
    data->prepareMemory();
  }
  if(0)printf("imported data %s, mem:%p len:%d size:%d sz2:%d\n",data->getName().c_str(),
	      data->getContentAddress(),event->length,
	      data->getContentSize(),event->length-data->getHeaderSize());

  offset=0;
  data->deserialize(event->buffer,offset,event->length,mi,header_only);
  if(0)printf("Data is imported:\n");data->dump('N');
  return data;
}
/*---------------------------------------------------------------------------------*/
void  DLB::checkImportedTasks(){

  list<IDuctteipTask *>::iterator task_it;
  for(task_it = dtEngine.import_tasks.begin(); task_it != dtEngine.import_tasks.end(); ){
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

	  dtEngine.mailbox->send(data->getHeaderAddress(),
			data->getPackSize(),
			MailBox::MigratedTaskOutDataTag,
			data->getHost());	  
	}	
      }
      task_it = dtEngine.import_tasks.erase(task_it);
      if (dlb_state == TASK_IMPORTED && dtEngine.import_tasks.size() ==0){
	dlb_state=DLB_STATE_NONE;
	dlb_stage= DLB_NONE;
	if(0)printf("import task list emptied.\n");
      }
      LOG_LOAD;
    }
    else{
      task_it++;
    }
  }
}
/*---------------------------------------------------------------------------------*/
void  DLB::checkExportedTasks(){
  if (dlb_state == TASK_EXPORTED && dtEngine.export_tasks.size() ==0){
    printf("export task list emptied.\n");
    dlb_state=DLB_STATE_NONE;
    dlb_stage= DLB_NONE;
  }
  return;


  list<IDuctteipTask *>::iterator task_it;
  for(task_it = dtEngine.export_tasks.begin(); task_it != dtEngine.export_tasks.end(); ){
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
      task_it = dtEngine.export_tasks.erase(task_it);
      LOG_LOAD;
    }
    task_it++;
  }
}
/*---------------------------------------------------------------------------------*/
long DLB::getActiveTasksCount(){
  list<IDuctteipTask *>::iterator it;
  long count = 0 ;
  if(dtEngine.running_tasks.size() && 0 )
    printf("WaitForData=2, Running, Finished, UpgradingData, CanBeCleared\n");
  for(it= dtEngine.running_tasks.begin(); it != dtEngine.running_tasks.end(); it ++){
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
  dlb_profile.max_para = (dtEngine.running_tasks.size() > dlb_profile.max_para )?
    dtEngine.running_tasks.size():dlb_profile.max_para ;
  dlb_profile.max_para2 = (count > dlb_profile.max_para2 )?count:dlb_profile.max_para2 ;
  if(DLB_DEBUG)printf("\n");
  if(DLB_DEBUG)printf("-RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,
		      dtEngine.running_tasks.size()
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  if ( dlb_state != TASK_IMPORTED && dlb_state!= TASK_EXPORTED){
    if ( dlb_stage != DLB_FINDING_BUSY && dlb_stage != DLB_FINDING_IDLE){
      dlb_state = ( (dtEngine.running_tasks.size()-count)>DLB_BUSY_TASKS)?DLB_BUSY:DLB_IDLE;
      if(DLB_DEBUG)printf("%d\n",__LINE__);
      restartDLB(0);
      dtEngine.doDLB(0);
    }
  }
  if(DLB_DEBUG)printf("+RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,dtEngine.running_tasks.size()
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  return count;
}

DLB dlb;
