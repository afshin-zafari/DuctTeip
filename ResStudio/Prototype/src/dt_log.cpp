#include "dt_log.hpp"
#include "listener.hpp"
DuctteipLog dt_log;
/*============================================================================*/
   EventInfo::EventInfo(){}
/*----------------------------------------------------------------------------*/
   EventInfo::EventInfo(int id_,const string &info,ulong h){
    event_id = id_;
    proc_id = me ; 
    thread_id = pthread_self(); 
    start = getTime();
    length = 0;
    handle =h;
    text.assign(info);
  }
/*----------------------------------------------------------------------------*/
 EventInfo &EventInfo::operator =(EventInfo &rhs){
    event_id = rhs.event_id;
    proc_id = rhs.proc_id ; 
    thread_id = rhs.thread_id;
    start = rhs.start;
    length = rhs.length;
    text = rhs.text;
    handle = rhs.handle;
    return *this;
 }
/*----------------------------------------------------------------------------*/
  void EventInfo::dump(ofstream &log_file){
    if ( event_id < 0 )       return;
    //    printf("dump ev len:%ld\n",length);
    log_file << proc_id << ' ' 
	     << thread_id << ": "
	     << start << ' ' 
      	     << setfill('0') << setw(9) 
	     << length << ' ' 
      	     << setfill('0') << setw(4) 
	     << handle << ' ' 
      	     << setfill('0') << setw(2) 
      	     << event_id << ' '
	     << text << ' '
	     << endl;

  }
/*----------------------------------------------------------------------------*/
  DuctteipLog::~DuctteipLog(){  }
/*----------------------------------------------------------------------------*/
  DuctteipLog::DuctteipLog(){
    FLOPS_add = FLOPS_mul = FLOPS_nlin = 0;
    for ( int i =0 ; i < NumberOfEvents; i ++){
      stats[i].minimum = MIN_INIT_VAL;
      stats[i].maximum = stats[i].total = stats[i].count = 0;
      stats[i].start=-1;
    }
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::updateStatisticsPair(int e ){
    if ( e != ProgramExecution ) 
      return;
    TimeUnit t;
    if (stats[e].start ==-1 ){
      stats[e].start=getTime();
    }
    else {
      t = getTime();
      stats[e].total += (t - stats[e].start)/SCALE;
      stats[e].start=-1;
      stats[e].count ++;
    }
    LOG_INFO(LOG_PROFILE,"ProgExec , st:%ld end:%ld\n",stats[e].start,t ) ;

  }
/*----------------------------------------------------------------------------*/
  string  DuctteipLog::getEventName(int event ) {
#define KeyNamePair(a) case a: return  #a 
    switch ( event )  {
      KeyNamePair(DataDefined);
      KeyNamePair(DataPartitioned);
      KeyNamePair(TaskDefined);
      KeyNamePair(ReadTask);
      KeyNamePair(ListenerDefined);
      KeyNamePair(Populated);
      KeyNamePair(DataSent);
      KeyNamePair(TaskSent);
      KeyNamePair(ListenerSent);
      KeyNamePair(DataSendCompleted);
      KeyNamePair(TaskSendCompleted);
      KeyNamePair(ListenerSendCompleted);
      KeyNamePair(RunTimeVersionUpgraded);
      KeyNamePair(DataReceived);
      KeyNamePair(TaskReceived);
      KeyNamePair(ListenerReceived);
      KeyNamePair(CheckedForListener);
      KeyNamePair(CheckedForTask);
      KeyNamePair(CheckedForRun);
      KeyNamePair(SuperGlueTaskDefine);
      KeyNamePair(Executed);
      KeyNamePair(Woken);
      KeyNamePair(SkipOverhead);
      KeyNamePair(MPITestSent);
      KeyNamePair(MailboxGetEvent);
      KeyNamePair(AnySendCompleted);
      KeyNamePair(LastReceiveCompleted);
      KeyNamePair(MPIReceive);
      KeyNamePair(MPIProbed);
      KeyNamePair(MailboxProcessed);
      KeyNamePair(WorkProcessed);
      KeyNamePair(CheckedForTerminate);
      KeyNamePair(CommFinish);
      KeyNamePair(ProgramExecution);
      KeyNamePair(EventsWork);
      KeyNamePair(TaskExported);
      KeyNamePair(TaskImported);
      KeyNamePair(DataExported);
      KeyNamePair(DataImported);
    }    
    return "";
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEvent(IDuctteipTask * task,int event,int dest){
    stringstream ss; 
    if ( config.N < BIG_SIZE){
      ss <<  getEventName(event)  << ' ' << task->getName();
      EventInfo * event_item = new EventInfo (event,ss.str(),task->getHandle());
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
    if ( event == TaskSent){
      updateCommStats(TaskSent,dest);
    }
  } 
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEvent(IData *data,int event,int dest){
    stringstream ss; 
    if ( config.N < BIG_SIZE){
      ss <<  getEventName(event)  << " " << data->getName();
      DataHandle *dh = data->getDataHandle();
      ulong h = 0 ;
      if ( dh != NULL && event !=DataDefined) 
	h = dh->data_handle;
      EventInfo * event_item = new EventInfo (event,ss.str(),h);
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
    if ( event == DataSent || event == DataReceived ) {
      updateCommStats(event,dest);
    }
  } 
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEvent(engine *eng,int event){ 
    stringstream ss; 
    if ( config.N < BIG_SIZE || event == ProgramExecution){
      ss <<  getEventName(event)  << " ";
      EventInfo * event_item = new EventInfo (event,ss.str());
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
    if ( event == ProgramExecution ) {
      LOG_INFO(LOG_PROFILE, "ProgExec index:%ld , dur:%lf\n",events_list.size(),stats[ProgramExecution].total ) ;
    }
  } 
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEvent(IListener *listener,int event,int dest ){
    stringstream ss; 
    if ( config.N < BIG_SIZE){
      ss <<  getEventName(event)  << " for_Data " << listener->getData()->getName();
      DataHandle *dh = listener->getData()->getDataHandle();
      ulong h = 0 ;
      if ( dh != NULL ) 
	h = dh->data_handle;
      EventInfo * event_item = new EventInfo (event,ss.str(),h ) ;
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
    if ( event == ListenerSent ) {
      updateCommStats(ListenerSent,dest);
    }
  } 
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEvent(int event){
    stringstream ss(""); 
    EventInfo * event_item = new EventInfo (event,ss.str());
    events_list.push_back(event_item);
  } 

/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEventNumber (unsigned long num, int e ) {
    if (e >= NumberOfEvents) return;
    stats[e].count += num;
    stats[e].total = stats[e].minimum = stats[e].maximum = 0;
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::setParams(int nodes_,long DataPackSize_, 
		 long ListenerPackSize_,long TaskPackSize_){
    DataPackSize = DataPackSize_;
    ListenerPackSize = ListenerPackSize_;
    TaskPackSize = TaskPackSize_;
    fprintf(stderr,"@stats Msg Sizes D,T,L: %ld,%ld,%ld\n",DataPackSize,TaskPackSize,ListenerPackSize);
    nodes = nodes_;
    for ( int i =0; i< 3; i++){
      commStats[i] = new long[nodes];
      for ( int j =0; j< nodes; j++)
	commStats[i][j] = 0;
    }
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::updateCommStats(int event, int dest ){
    if ( event == DataSent || event == DataReceived) {
      commStats[0][dest] += DataPackSize;
      stats[event].count ++;
    }
    if ( event == ListenerSent ) {
      commStats[1][dest] += ListenerPackSize;
    }
    if ( event == TaskSent ) {
      commStats[2][dest] += TaskPackSize;
    }
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::addEventStart(engine *eng,int event){addEvent(eng,event);}  
  void DuctteipLog::addEventEnd(engine *eng,int event){addEvent(eng,event);}  
/*----------------------------------------------------------------------------*/
  void DuctteipLog::updateStatistics(int e,TimeUnit length){
    double len = ((double)length)/SCALE;
    if ( e < 0 || e>= NumberOfEvents) return;
    stats[e].count ++;
    stats[e].minimum = ((stats[e].minimum > len)?len:stats[e].minimum);
    stats[e].maximum = ((stats[e].maximum < len)?len:stats[e].maximum);
    stats[e].total += len;
}
/*----------------------------------------------------------------------------*/
   void DuctteipLog::dump(long sg_task_count){
    char s[20];
    list<EventInfo *>::iterator it;
    sprintf(s,"dt_log_file-%2.2d.txt",me);

    dumpLoads();

    ofstream log_file(s);
    TimeUnit t = getTime();
    
    t = getTime() - t;
    stats[EventsWork].count = events_list.size();
    stats[EventsWork].total = t / SCALE;
    stats[EventsWork].minimum = stats[EventsWork].maximum = 0;
    
    stats[SkipOverhead].count /= SCALE;    
    if ( !config.simulation)
      dumpStatistics();
    else
      dumpSimulationResults(sg_task_count);
    log_file.close();
  }
/*----------------------------------------------------------------------------*/
double DuctteipLog::getStattime(int e){
  if ( e < NumberOfEvents)
      return stats[e].total;
  return 0.0;
}
/*----------------------------------------------------------------------------*/
   void DuctteipLog::dumpStatistics(){
     return ; 
     char dash[10]="---------";
     fprintf(stderr,"@stats Node\tEvent\t\t Count\t Sum.Duration\t Average\t Min\t\t Max\t\t\n");
     for ( int i=0; i < NumberOfEvents ; i ++){
       if (stats[i].minimum == MIN_INIT_VAL ) 
	 stats[i].minimum =0;
       fprintf(stderr,"@stats %d %22.22s %6ld\t %9.2lf \t %9.2lf \t %9.2lf \t %9.2lf\n",
	       me,getEventName(i).c_str(),
	       stats[i].count,
	       stats[i].total,
	       (stats[i].count ==0)?0.0:(double)stats[i].total/stats[i].count,
	       stats[i].minimum,
	       stats[i].maximum);
     }
   
     for ( int i=0; i< nodes; i++){
       if ( i == me ) continue;
       fprintf(stderr,"@stats %d     SentDataSize %7ld ToNode %d\n",me,commStats[0][i],i);
       fprintf(stderr,"@stats %d SentListenerSize %7ld ToNode %d\n",me,commStats[1][i],i);
       fprintf(stderr,"@stats %d     SentTaskSize %7ld ToNode %d\n",me,commStats[2][i],i);
     }
   }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::logExportTask(long count){
    load * l = new load(getTime(),count);
    export_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::logImportTask(long count){
    load * l = new load(getTime(),count);
    import_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::logLoadChange(long ld){
    load * l = new load(getTime(),ld);
    load_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::dumpLoad(list<load*> lst,char *s){
    list<load *>::iterator it;
    ofstream log_file(s);
    for (it = lst.begin();it != lst.end();it ++){
      load  *l =(*it);
      log_file << l->t << ' ' << l->val << endl;
    }
    log_file.close();    
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::dumpLoads(){
    char s[40];
    sprintf(s,"dt_load_file-%2.2d.txt",me);
    dumpLoad(load_list,s);
    sprintf(s,"dt_export_file-%2.2d.txt",me);
    dumpLoad(export_list,s);
    sprintf(s,"dt_import_file-%2.2d.txt",me);
    dumpLoad(import_list,s);    
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::dumpSimulationResults(long sg_task_count){
    fprintf(stderr,"\n@Simulation: N=%ld,P=%d, p=%ld,q=%ld,B=%ld,b=%ld,k=%d," \
	   "t=%ld,T=%ld,s=%ld,S=%ld,r=%ld,R=%ld,c=%ld,z=%ld,i=%lf\n",
	   config.N,config.P,config.p,config.q,config.Nb,config.nb,me,sg_task_count,stats[TaskDefined].count,
	   stats[DataSent].count,stats[DataSent].count*DataPackSize, 
	   stats[DataReceived].count,stats[DataReceived].count*DataPackSize, 
	   config.nt,config.N/config.Nb/config.nb,stats[ProgramExecution].total);
  }
/*----------------------------------------------------------------------------*/
void  DuctteipLog::addEventSummary(int e, TimeUnit st, TimeUnit dur){
  EventInfo *ee = new EventInfo;
  ee->event_id  = e;
  ee->proc_id   = me;
  ee->thread_id = pthread_self();
  ee->start     = st;
  ee->length    = dur;
  summary_list.push_back(ee);  
}
/*----------------------------------------------------------------------------*/
void DuctteipLog::logLoad(long run, long imp, long exp){
  logLoadChange(run);
  logImportTask(imp);
  logExportTask(exp);  
}
/*----------------------------------------------------------------------------*/

Timer::Timer(ulong &sum,int e, const char *f, int l, const char *fx):event(e),line(l){
  tot = &sum;
  st = getTime();
  strcpy(file,f );
  strcpy(func,fx);
  dt_log.addEventNumber(1,e);
}
/*----------------------------------------------------------------------------*/
Timer::Timer(int e, const char *f, int l, const char *fx):event(e),line(l){
  tot= NULL;
  st = getTime();
  strcpy(file,f );
  strcpy(func,fx);
  dt_log.addEventNumber(1,e);
}
/*----------------------------------------------------------------------------*/
Timer::~Timer(){
  end = getTime();
  *tot += end - st;
  long dur;
  if ( tot ==NULL)
    dur = (end-st)/1000;      
  else
    dur = *tot;
  //  fprintf(stderr,"%20s,%4d, %-32s, tid:%9X, %6ld ::",file,line,func,(ulong)pthread_self(),UserTime()); 
  //  fprintf(stderr,"event:%22s, dur:%ld\n",dt_log.getEventName(event).c_str(),dur);
  dt_log.addEventSummary(event,st,dur);
}

