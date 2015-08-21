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
  for ( int i=0;i<2;i++)
    for ( int j=0;j<2;j++)
      find[i][j]=decline[i][j]=accept[i][j]=0;
}
/*---------------------------------------------------------------*/
void DLB::updateDLBProfile(){
  dlb_profile.tot_failure+=dlb_failure;
  dlb_profile.max_loc_fail=
    (dlb_profile.max_loc_fail >dlb_failure) ?
    dlb_profile.max_loc_fail :dlb_failure  ;
}
/*---------------------------------------------------------------*/
void DLB::restartDLB(int s){    
  
  if ( dlb_state == TASK_IMPORTED || dlb_state == TASK_EXPORTED){
    if(0)printf("state = %d(Imp=2,Exp=3), =>no restart.\n",dlb_state);
    return;
  }
  if ( s==0 &&(dlb_stage == DLB_FINDING_IDLE ||dlb_stage == DLB_FINDING_BUSY)){
    return;
  }
  updateDLBProfile();
  dlb_prev_state = dlb_state;
  if ( s <0){
    dlb_stage = DLB_NONE;
    dlb_state = getDLBStatus();
  }
  if ( dlb_state != dlb_prev_state){
    dlb_failure =0;
  }
}
/*---------------------------------------------------------------*/
void DLB::goToSilent(){
  dlb_failure =0;
  dlb_silent_start = UserTime();
  LOG_INFO(LOG_DLBX,"%ld\n",dlb_silent_start);
  dlb_stage = DLB_SILENT;
}
/*---------------------------------------------------------------*/
bool DLB::passedSilentDuration(){
  ulong silent = (ulong)UserTime() - (ulong)dlb_silent_start;
  bool passed = silent > SILENT_PERIOD;
  if(passed){
    LOG_INFO(LOG_DLBX,"silent-dur:%ld, start:%ld\n",silent,(ulong)dlb_silent_start);
    dlb_silent_start = 0 ;
  }
  return passed;
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
  fprintf(stderr,"					Me	    Others\n");
  fprintf(stderr,"				 Idle	 Busy	 Idle	 Busy\n");
  fprintf(stderr,"\t\t find\t\t:%d\t %d\t %d\t %d\t \n",
	  find[IDX_IDLE][IDX_ME],
	  find[IDX_BUSY][IDX_ME],
	  find[IDX_IDLE][IDX_OTHERS],
	  find[IDX_BUSY][IDX_OTHERS]
	  );
  fprintf(stderr,"\t\t decline\t:%d\t %d\t %d\t %d\t \n",
	  decline[IDX_IDLE][IDX_ME],
	  decline[IDX_BUSY][IDX_ME],
	  decline[IDX_IDLE][IDX_OTHERS],
	  decline[IDX_BUSY][IDX_OTHERS]
	  );
  fprintf(stderr,"\t\t accept\t:%d\t %d\t %d\t %d\t \n",
	  accept[IDX_IDLE][IDX_ME],
	  accept[IDX_BUSY][IDX_ME],
	  accept[IDX_IDLE][IDX_OTHERS],
	  accept[IDX_BUSY][IDX_OTHERS]
	  );
}
/*---------------------------------------------------------------*/
int DLB::getDLBStatus(){
  long actives = getActiveTasksCount();
  if ( (dtEngine.running_tasks.size()-actives )> DLB_BUSY_TASKS){
    LOG_INFO(LOG_DLB,"task#:%ld active:%ld\n",dtEngine.running_tasks.size(),actives);
    return DLB_BUSY;
  }
  return DLB_IDLE;
}
/*---------------------------------------------------------------*/
void DLB::doDLB(int st){

  if (!dtEngine.cfg->getDLB())
    return;
  if (dtEngine.net_comm->get_host_count()==1) 
    return;
  if ( dtEngine.task_list.size() ==0 && dtEngine.running_tasks.size() ==0 ) 
    return;

  if ( dlb_stage == DLB_SILENT ){
    if ( !passedSilentDuration() ) 
      return;
    else{
      LOG_INFO(LOG_DLB,"I'm not Silent any more.=> DLB_NONE.\n");
      dlb_profile.tot_silent+=SILENT_PERIOD;
      LOG_INFO(LOG_DLBX,"%ld\n",dlb_silent_start);
      dlb_silent_start=0;
      LOG_INFO(LOG_DLBX,"%ld\n",dlb_silent_start);  
      dlb_stage = DLB_NONE;
    }
  }
    
  if (dlb_failure > FAILURE_MAX ) {
    LOG_INFO(LOG_DLBX,"Too many failures(%ld), go to Silent.\n",dlb_failure);
    goToSilent();
    LOG_INFO(LOG_DLBX,"%ld\n",dlb_silent_start);
    return;
  }
  if ( dlb_stage != DLB_NONE )
    return;
  if ( dlb_state == TASK_IMPORTED || dlb_state == TASK_EXPORTED)
    return;
  LOG_INFO(LOG_DLB,"Not in any specific state. Get status and test again.\n");
  dlb_profile.tot_tick++;
  if ( st<=0)
    dlb_state =  getDLBStatus();
  if ( dlb_state == DLB_BUSY ) {
    LOG_INFO(LOG_DLBX,"find idle\n");
    findIdleNode();
  }
  else 
    if ( dlb_state == DLB_IDLE ) {
      LOG_INFO(LOG_DLBX,"find busy\n");
      findBusyNode();
    }
}

/*---------------------------------------------------------------*/
void DLB::findBusyNode(){
  //  if ( dlb_state == DLB_SILENT)    return;
  if ( dlb_stage == DLB_FINDING_BUSY ) 
    return;
  dlb_node = getRandomNodeEx(DLB_FINDING_BUSY);
  if ( dlb_node <0 || dlb_node == me )
    return;
  dlb_profile.tot_try++;
  find[IDX_BUSY][IDX_ME]++;
  dlb_stage = DLB_FINDING_BUSY ;
  PRINT_IF(DLB_DEBUG)("send FIND_BUSY to :%d\n",dlb_node);
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindBusyTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::findIdleNode(){
  //  if ( dlb_state == DLB_SILENT)    return;
  if ( dlb_stage == DLB_FINDING_IDLE ) 
    return;
  dlb_node = getRandomNodeEx(DLB_FINDING_IDLE);    
  if ( dlb_node <0 || dlb_node == me )
    return;
  dlb_profile.tot_try++;
  find[IDX_IDLE][IDX_ME]++;
  dlb_stage = DLB_FINDING_IDLE ;
  LOG_INFO(LOG_DLBX,"send FIND_IDLE to :%d\n",dlb_node);
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindIdleTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::received_FIND_IDLE(int p){
  find[IDX_IDLE][IDX_OTHERS]++;
  if ( dlb_state == DLB_IDLE ) {
    acceptImportTasks(p);
    return;
  }
  declineImportTasks(p);
}
void DLB::receivedImportRequest(int p){ received_FIND_IDLE(p);}
/*---------------------------------------------------------------*/
void DLB::received_FIND_BUSY(int p ) {
  TIME_DLB;
  find[IDX_BUSY][IDX_OTHERS]++;
  dlb_substate = DLB_NONE;
  if ( dlb_state != DLB_BUSY ){
    LOG_INFO(LOG_DLB,"I'm Idle and cannot export any task.\n");
    declineExportTasks(p);
    return;
  }
  if ( dlb_state == DLB_BUSY){
    LOG_INFO(LOG_DLB,"I'm BUSY  and already sent FIND_IDLE to %d(not %d).\n",dlb_node,p);
    if (dlb_node !=-1 && dlb_node != p){
      exportTasks(p);
      LOG_INFO(LOG_DLBX,"restart\n");
      restartDLB();
      dlb_substate = EARLY_ACCEPT;
    }
  }
}
void DLB::receivedExportRequest(int p){received_FIND_BUSY(p);}
/*---------------------------------------------------------------*/
void DLB::received_DECLINE(int p){
  TIME_DLB;
  dlb_failure ++;
  decline[IDX_BUSY][IDX_OTHERS]++;
  dlb_stage = DLB_NONE;
  LOG_INFO(LOG_DLBX,"restart\n");
  restartDLB();    
}
void DLB::receivedDeclineMigration(int p){received_DECLINE(p); }
/*---------------------------------------------------------------*/
IDuctteipTask *DLB::selectTaskForExport(){
  list<IDuctteipTask*>::iterator it;
  for(it=dtEngine.running_tasks.begin();it != dtEngine.running_tasks.end();it++){
    IDuctteipTask *t = *it;
    if (t->isExported()) continue;
    if (t->isFinished()) continue;
    if (t->isRunning()) continue;
    if (t->isUpgrading()) continue;
    if (t->canBeCleared()) continue;
    dtEngine.export_tasks.push_back(t);
    dt_log.logExportTask(dtEngine.export_tasks.size());

    dtEngine.running_tasks.erase(it);
    dt_log.logLoadChange(dtEngine.running_tasks.size());
    return t;
  }
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
  IDuctteipTask *t;
  do{
    t=selectTaskForExport();
    if( t == NULL){
      dlb_state = DLB_STATE_NONE;
      LOG_INFO(LOG_DLBX,"no task for export,restart\n");
      restartDLB();
      return;
    }
    dlb_profile.export_task++;
    accept[IDX_BUSY][IDX_ME]++;
    dlb_state = TASK_EXPORTED;
    t->dump();
    t->setExported( true);
    exportTask(t,p);
    list<DataAccess *> *data_list=t->getDataAccessList() ;
    list<DataAccess *> :: iterator it;
    for ( it = data_list->begin(); it != data_list->end()  ; it ++){
      IData * data = (*it)->data;
      if ( data->isExportedTo(p) )
	continue;
      data->setExportedTo(p);
      dlb_profile.export_data++;

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
  data->dumpCheckSum('i');
  data->dump(' ');
  void dumpData(double *,int,int,char);
  list<IDuctteipTask *>::iterator it;
  for(it=dtEngine.import_tasks.begin();it != dtEngine.import_tasks.end(); it ++){
    IDuctteipTask *t = *it;
    if ( t->canRun('i')){
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
      t_data->dump(' ');
      if ( (*data_it)->type == IData::WRITE ){
	if (*t_data->getDataHandle() == *data->getDataHandle()){
	  if (t_data->getRunTimeVersion(IData::WRITE) == data->getRunTimeVersion(IData::WRITE) ) {
	    task->upgradeData('r');
	    it = dtEngine.export_tasks.erase(it);
	    dt_log.logExportTask(dtEngine.import_tasks.size());
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
  LOG_INFO(LOG_DLBX,"received tasks from %d\n",event->host);
  dlb_stage = DLB_NONE;
  if (dlb_state == DLB_BUSY) {
    LOG_INFO(LOG_DLBX,"BUSY, but received TASKS from %d.\n",event->host);
  }
  LOG_INFO(LOG_DLBX,"restart\n");
  restartDLB();     
}
/*---------------------------------------------------------------*/
void DLB::received_ACCEPTED(int p){
  TIME_DLB;
  dlb_stage = DLB_NONE;
  accept[IDX_BUSY][IDX_OTHERS]++;
  if ( dlb_state == DLB_IDLE ) {
    LOG_INFO(LOG_DLBX,"requested task from %d but received ACCEPTED from %d.\n",dlb_node,p);
    return;
  }
  if ( dlb_substate == EARLY_ACCEPT ) {
    if ( dlb_node == p ){
      LOG_INFO(LOG_DLBX,"restart\n");
      restartDLB();
      return;
    }      
  }
  exportTasks(p);
  LOG_INFO(LOG_DLBX,"restart\n");
  restartDLB();
}
/*---------------------------------------------------------------*/
void DLB::declineImportTasks(int p){    
  dlb_glb_failure ++;
  decline[IDX_IDLE][IDX_ME]++;
  LOG_INFO(LOG_DLBX,"from %d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::declineExportTasks(int p){    
  dlb_glb_failure ++;
  decline[IDX_BUSY][IDX_ME]++;
  LOG_INFO(LOG_DLBX,"from %d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::acceptImportTasks(int p){    
  dlb_substate = ACCEPTED;
  dlb_state = TASK_IMPORTED;
  accept[IDX_IDLE][IDX_ME]++;
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
  dt_log.logImportTask(dtEngine.import_tasks.size());
    
  LOG_EVENT(DuctteipLog::TaskImported);

  dtEngine.criticalSection(engine::Leave); 
}

/*---------------------------------------------------------------*/
bool DLB::isInList(int p,int req){
  vector<Failure *> &L=dlb_fail_list;
  vector<Failure *>::iterator it;
  for (it=L.begin() ;it!=L.end();it++) {
    Failure *f=(*it);
    if (f->node == p ) {
      if ( f->request == req ) {
	if ( (UserTime()-f->timestamp) > 10 ){ // 10 m-sec
	  L.erase(it);
	  return false;
	}
	else
	  return true;
      }
      else{
	L.erase(it);
	return false;
      }
    } 
  }
  return false;
}
/*---------------------------------------------------------------*/
int DLB::getRandomNodeEx(int req){
  int np = dtEngine.net_comm->get_host_count();
  int p;
  while ( true ){
    p = rand() % np ;
    if ( !isInList (p,req)  )
      break;
  }    
  dlb_fail_list.push_back(new Failure(p,UserTime(),req) );    
  LOG_INFO(LOG_DLBX,"node:%d, req:%d\n",p,req);
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
  LOG_INFO(LOG_DLB,"task:%s\n",task->getName().c_str());
  task->dump();
}
/*--------------------------------------------------------------------------*/
IData * DLB::importedData(MailBoxEvent *event,MemoryItem *mi){
  int offset =0 ;
  const bool header_only = true;
  const bool all_content = false;
  DataHandle dh;


  dh.deserialize(event->buffer,offset,event->length);
  IData *data = glbCtx.getDataByHandle(&dh);
  void dumpData(double *, int ,int, char);
  event->memory->dump();
  mi = data->getDataMemory();
  if ( mi != NULL ) {
    mi->dump();
    memcpy(data->getContentAddress(),event->buffer+data->getHeaderSize(),data->getContentSize());
  }
  else{
    data->setDataMemory(event->memory);
    data->setContentSize(event->length-data->getHeaderSize());
    mi = data->getDataMemory();
    data->prepareMemory();
  }
  if ( mi == NULL) {
    fprintf(stderr,"error:preparememory. \n");
    data->prepareMemory();
  }
  offset=0;
  data->deserialize(event->buffer,offset,event->length,mi,header_only);
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
      }
      dt_log.logImportTask(dtEngine.import_tasks.size());
    }
    else{
      task_it++;
    }
  }
}
/*---------------------------------------------------------------------------------*/
void  DLB::checkExportedTasks(){
  if (dlb_state == TASK_EXPORTED && dtEngine.export_tasks.size() ==0){
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
	LOG_INFO(LOG_DLB,"task:%d,%s - data:%s, rtv:%s, reqv:%s \n",
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
    task_it++;
  }
}
/*---------------------------------------------------------------------------------*/
long DLB::getActiveTasksCount(){
  list<IDuctteipTask *>::iterator it;
  long count = 0 ;
  for(it= dtEngine.running_tasks.begin(); it != dtEngine.running_tasks.end(); it ++){
    IDuctteipTask *task = *it;
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
  LOG_INFO(LOG_DLB,"-RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,
		      dtEngine.running_tasks.size()
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  if ( dlb_state != TASK_IMPORTED && dlb_state!= TASK_EXPORTED){
    if ( dlb_stage != DLB_FINDING_BUSY && dlb_stage != DLB_FINDING_IDLE ){
      dlb_state = ( (dtEngine.running_tasks.size()-count)>DLB_BUSY_TASKS)?DLB_BUSY:DLB_IDLE;
      LOG_INFO(LOG_DLBX," RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,
	       dtEngine.running_tasks.size()
	       ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
      LOG_INFO(LOG_DLB,"restart,stage:%d, state:%d\n",dlb_stage,dlb_state);
      restartDLB(0);
      doDLB(0);
    }
  }
  LOG_INFO(LOG_DLB,"+RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,dtEngine.running_tasks.size()
	   ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  return count;
}
/*---------------------------------------------------------------------------------*/

DLB dlb;
