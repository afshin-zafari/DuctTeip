#include "dlb.hpp"
/*---------------------------------------------------------------*/
void DLB::initDLB(){
  SILENT_PERIOD = config.getSilenceDuration();
  DLB_BUSY_TASKS = config.getDLBThreshold();
  dlb_state = DLB_STATE_NONE;
  dlb_substate = DLB_NONE;
  dlb_stage = DLB_NONE;
  dlb_node = -1;
  dlb_failure = dlb_glb_failure=0;
  dlb_silent_start = dlb_silent_cnt = dlb_silent_tot = 0 ;
  dlb_tot_time=0;
  last_msg_time = UserTime();
  dlb_profile.tot_silent=0;
  dlb_profile.export_task=
    dlb_profile.export_data=
    dlb_profile.import_task=
    dlb_profile.import_data=
    dlb_profile.max_para   =
    dlb_profile.max_para2  =0;
  srand(time(NULL));
  for ( int i=0;i<6;i++)
    for ( int j=0;j<2;j++)
      dlb_hist[i][j]=0;
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
  last_msg_time = UserTime();
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
  dlb_silent_cnt++;
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
  fprintf(stderr,"\t\t sent\tdecl.\tacc.\t\trcvd\tdecl.\tacc.\n");
  fprintf(stderr,"Find Idle:\t%d\t%d\t%d \t\t %d\t%d\t%d\n",
	  dlb_hist[IDX_FINDI][IDX_SEND],
	  dlb_hist[IDX_DECLI][IDX_RECV],
	  dlb_hist[IDX_ACCPI][IDX_RECV],

	  dlb_hist[IDX_FINDI][IDX_RECV],
	  dlb_hist[IDX_DECLI][IDX_SEND],
	  dlb_hist[IDX_ACCPI][IDX_SEND]);
  fprintf(stderr,"Find Busy:\t%d\t%d\t%d \t\t %d\t%d\t%d\n",
	  dlb_hist[IDX_FINDB][IDX_SEND],
	  dlb_hist[IDX_DECLB][IDX_RECV],
	  dlb_hist[IDX_ACCPB][IDX_RECV],

	  dlb_hist[IDX_FINDB][IDX_RECV],
	  dlb_hist[IDX_DECLB][IDX_SEND],
	  dlb_hist[IDX_ACCPB][IDX_SEND]);
  fprintf(stderr,"SilentCnt :%ld, SilentDur (ms): %ld\n",dlb_silent_cnt,dlb_silent_tot);
  fprintf(stderr,"Total Time(ms): %ld\n",dlb_tot_time/1000000);
}
/*---------------------------------------------------------------*/
int DLB::getDLBStatus(){
  long exportables = getExportableTasksCount();
  if ( exportables>0){
    LOG_INFO(LOG_DLB,"task#:%ld export#:%ld\n",dtEngine.running_tasks.size(),exportables);
    return DLB_BUSY;
  }
  return DLB_IDLE;
}
/*---------------------------------------------------------------*/
void DLB::doDLB(int st){

  LOG_INFO(LOG_DLB,"Enter\n");
  if (!dtEngine.cfg->getDLB())
    return;
  if (dtEngine.net_comm->get_host_count()==1) 
    return;
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  if ( dtEngine.task_list.size() ==0 && dtEngine.running_tasks.size() ==0 ) {
    LOG_INFO(LOG_DLB,"No task at all.\n");
    return;
  }

  LOG_INFO(LOG_DLB,"\n");
  if ( dlb_stage == DLB_SILENT ){
    if ( !passedSilentDuration() ) 
      return;
    else{
      LOG_INFO(LOG_DLB,"I'm not Silent any more.=> DLB_NONE.\n");
      dlb_profile.tot_silent+=SILENT_PERIOD;
      LOG_INFO(LOG_DLB,"%ld\n",dlb_silent_start);
      dlb_silent_tot +=UserTime() - dlb_silent_start;
      dlb_silent_start=0;
      LOG_INFO(LOG_DLB,"%ld\n",dlb_silent_start);  
      dlb_stage = DLB_NONE;
    }
  }
    
  LOG_INFO(LOG_DLB,"is Not silent\n");
  if (dlb_failure > FAILURE_MAX ) {
    LOG_INFO(LOG_DLB,"Too many failures(%ld), go to Silent.\n",dlb_failure);
    goToSilent();
    LOG_INFO(LOG_DLB,"%ld\n",dlb_silent_start);
    return;
  }
  ulong t= (ulong)UserTime();
  if ( (t-last_msg_time)>SILENT_PERIOD){
    LOG_INFO(LOG_DLBX,"last_msg:%ld, dur:%ld?\n",last_msg_time ,(t-last_msg_time));
    restartDLB();
    return;
  }
  LOG_INFO(LOG_DLB,"stage:%d is None:%d?\n",dlb_stage ,DLB_NONE);
  if ( dlb_stage != DLB_NONE )
    return;
  LOG_INFO(LOG_DLB,"state:%d is X:%d/Imp:%d?\n",dlb_state,TASK_EXPORTED ,TASK_IMPORTED);
  if ( dlb_state == TASK_IMPORTED || dlb_state == TASK_EXPORTED)
    return;
  LOG_INFO(LOG_DLB,"Not in any specific state. Get status and test again.\n");
  dlb_profile.tot_tick++;
  if ( st<=0){
    dlb_state =  getDLBStatus();
  }
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
  if ( dlb_stage == DLB_FINDING_BUSY ) 
    return;
  dlb_node = getRandomNodeEx(DLB_FINDING_BUSY);
  if ( dlb_node <0 || dlb_node == me )
    return;
  dlb_profile.tot_try++;
  dlb_hist[IDX_FINDB][IDX_SEND]++;
  dlb_stage = DLB_FINDING_BUSY ;
  last_msg_time = UserTime();
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindBusyTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::findIdleNode(){
  if ( dlb_stage == DLB_FINDING_IDLE ) 
    return;
  dlb_node = getRandomNodeEx(DLB_FINDING_IDLE);    
  if ( dlb_node <0 || dlb_node == me )
    return;
  dlb_profile.tot_try++;
  dlb_hist[IDX_FINDI][IDX_SEND]++;
  last_msg_time = UserTime();
  dlb_stage = DLB_FINDING_IDLE ;
  LOG_INFO(LOG_DLBX,"send FIND_IDLE to :%d\n",dlb_node);
  dtEngine.mailbox->send((byte*)&dlb_node,1,MailBox::FindIdleTag,dlb_node);
}
/*---------------------------------------------------------------*/
void DLB::received_FIND_IDLE(int p){
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  last_msg_time = UserTime();
  dlb_hist[IDX_FINDI][IDX_RECV]++;
  if ( dlb_state == DLB_IDLE ) {
    acceptImportTasks(p);
    return;
  }
  declineImportTasks(p);
}
void DLB::receivedImportRequest(int p){ received_FIND_IDLE(p);}
/*---------------------------------------------------------------*/
void DLB::received_FIND_BUSY(int p ) {
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  last_msg_time = UserTime();
  dlb_hist[IDX_FINDB][IDX_RECV]++;
  dlb_substate = DLB_NONE;
  if ( dlb_state != DLB_BUSY ){
    LOG_INFO(LOG_DLB,"I'm Idle and cannot export any task.\n");
    declineExportTasks(p);
    return;
  }
  if ( dlb_state == DLB_BUSY){
    LOG_INFO(LOG_DLB,"I'm BUSY  and already sent FIND_IDLE to %d(not %d).\n",dlb_node,p);
    if (dlb_node !=-1 && dlb_node != p){
      dlb_hist[IDX_ACCPB][IDX_SEND]++;
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  last_msg_time = UserTime();
  if ( dlb_state == DLB_IDLE)
    dlb_hist[IDX_DECLB][IDX_RECV]++;
  else{
    dlb_hist[IDX_DECLI][IDX_RECV]++;
    dlb_failure ++;
  }
  dlb_stage = DLB_NONE;
  LOG_INFO(LOG_DLBX,"restart\n");
  restartDLB();    
}
void DLB::receivedDeclineMigration(int p){received_DECLINE(p); }
/*---------------------------------------------------------------*/
IDuctteipTask *DLB::selectTaskForExport(double &from){
  list<IDuctteipTask*>::iterator it;
  for(it=dtEngine.running_tasks.begin();it != dtEngine.running_tasks.end();it++){
    IDuctteipTask *t = *it;
    if (t->isExported()) continue;
    if (t->isFinished()) continue;
    if (t->isRunning()) continue;
    if (t->isUpgrading()) continue;
    if (t->canBeCleared()) continue;
    /*
    if ( config.getDLBSmart()){
      if ( !isFinishedEarlier(t,from)){
	from +=(double)dtEngine.avg_durations[t->getKey()];
	continue;
      }
    }
    */
    dtEngine.export_tasks.push_back(t);
    dt_log.logExportTask(dtEngine.export_tasks.size());

    dtEngine.running_tasks.erase(it);
    dt_log.logLoadChange(dtEngine.running_tasks.size());
    return t;
  }
  return NULL;
}
/*---------------------------------------------------------------*/
bool DLB::isFinishedEarlier(IDuctteipTask *task,double from){
  double  loc_fin = from+(double)dtEngine.avg_durations[task->getKey()];
  ulong comm_size=  task->getMigrateSize();
  double bw=dtEngine.net_comm->getBandwidth();
  if ( bw ==0)
    return false;
  double remote_fin =   (double)UserTime()+comm_size / bw + dtEngine.avg_durations[task->getKey()] ;
  LOG_INFO(LOG_DLB_SMART,"locfin:%.2lf, comm-size:%ld, bw:%.2lf, avg-dur:%.2lf, fr:%.2lf\n",
	   loc_fin,comm_size,bw,dtEngine.avg_durations[task->getKey()],from);
  LOG_INFO(LOG_DLB_SMART,"remfin:%.2lf, comm-time:%.2lf rem<loc?:%d\n",
	   remote_fin,comm_size/bw,remote_fin<loc_fin);
  if ( remote_fin < loc_fin) 
    return true;
  return false;
}
/*---------------------------------------------------------------*/
void DLB::exportTasks(int p){
  IDuctteipTask *t;
  double from;
  from = (double)UserTime();
  do{
    t=selectTaskForExport(from);
    if( t == NULL){
      dlb_state = DLB_STATE_NONE;
      LOG_INFO(LOG_DLBX,"no task for export,restart\n");
      restartDLB();
      return;
    }
    dlb_profile.export_task++;
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  dlb_profile.import_data++;
  int offset =0 ;
  DataHandle dh;
  dh.deserialize(event->buffer,offset,event->length);
  IData *data = glbCtx.getDataByHandle(&dh);


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
	    if ( config.getDLBSmart()){
	      if ( task->isRunning()){
		LOG_INFO(LOG_DLB_SMART,"out-data of an exported task is returned, but the task ran locally!\n");
		return;
	      }
	    }
	    data = importedData(event,event->getMemoryItem());// copy the contents from the incoming message
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  dlb_profile.import_task++;    
  dlb_hist[IDX_ACCPB][IDX_RECV]++;
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  dlb_stage = DLB_NONE;
  dlb_hist[IDX_ACCPI][IDX_RECV]++;
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
  dlb_hist[IDX_DECLI][IDX_SEND]++;
  LOG_INFO(LOG_DLBX,"from %d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::declineExportTasks(int p){    
  dlb_glb_failure ++;
  dlb_hist[IDX_DECLB][IDX_SEND]++;
  LOG_INFO(LOG_DLB,"from %d\n",p);
  dtEngine.mailbox->send((byte*)&p,1,MailBox::DeclineMigrateTag,p);
}
/*---------------------------------------------------------------*/
void DLB::acceptImportTasks(int p){    
  dlb_substate = ACCEPTED;
  dlb_state = TASK_IMPORTED;
  dlb_hist[IDX_ACCPI][IDX_SEND]++;
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

  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
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
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
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
long DLB::getExportableTasksCount(){

  list<IDuctteipTask *>::iterator it;
  long count = 0 ;
  long size = dtEngine.running_tasks.size();
  if (size==0){
    return count;
  }
  for(it= dtEngine.running_tasks.begin(); it != dtEngine.running_tasks.end(); it ++){
    IDuctteipTask *task = *it;
    if ( task->isRunning()    ) continue;
    if ( task->isFinished()   ) continue;
    if ( task->isUpgrading()  ) continue;
    if ( task->canBeCleared() ) continue;
    if ( task->isExported()   ) continue;
    count ++;  
  }
  LOG_INFO(LOG_DLBX,"Exportable  Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,size
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  return (count-DLB_BUSY_TASKS);
}
/*---------------------------------------------------------------------------------*/
long DLB::getActiveTasksCount(){

  list<IDuctteipTask *>::iterator it;
  long count = 0 ;
  long size = dtEngine.running_tasks.size();
  TIMERACC(dlb_tot_time,DuctteipLog::DLB);
  if (size==0){
    return count;
  }
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
  LOG_INFO(LOG_DLBX," RUNNING Tasks count=%ld from %ld st:%c,%d sg:%c,%d\n",count,size
		      ,"IBXMN"[dlb_state%5],dlb_state,"IBSN"[dlb_stage%4],dlb_stage );
  return count;
}
/*---------------------------------------------------------------------------------*/

DLB dlb;

