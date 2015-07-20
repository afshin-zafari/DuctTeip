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
  DuctteipLog::~DuctteipLog(){
  }
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
    if ( !isSequentialPairEvent(e) ) 
      return ;
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
    if ( e == ProgramExecution ) {
      LOG_INFO(LOG_PROFILE,"ProgExec , st:%ld end:%ld\n",stats[e].start,t ) ;
    }

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
    if ( N < BIG_SIZE){
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
    if ( N < BIG_SIZE){
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
    if ( N < BIG_SIZE || event == ProgramExecution){
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
    if ( N < BIG_SIZE){
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
  void DuctteipLog::addEvent(string  text,int event){
    stringstream ss; 
    if ( N < BIG_SIZE){
      ss <<  getEventName(event)  << " " << text;
      EventInfo * event_item = new EventInfo (event,ss.str());
      //event_item->text = text;
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
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
  void DuctteipLog::addEventStart(IDuctteipTask * task,int event){addEvent(task,event);} 
  void DuctteipLog::addEventStart(IData *data,int event){addEvent(data,event);} 
  void DuctteipLog::addEventStart(engine *eng,int event){addEvent(eng,event);}  
  void DuctteipLog::addEventStart(IListener *listener,int event){addEvent(listener,event);}  
  void DuctteipLog::addEventStart(string  text,int event){addEvent(text,event);}  

  void DuctteipLog::addEventEnd(IDuctteipTask * task,int event){addEvent(task,event);} 
  void DuctteipLog::addEventEnd(IData *data,int event){addEvent(data,event);} 
  void DuctteipLog::addEventEnd(engine *eng,int event){addEvent(eng,event);}  
  void DuctteipLog::addEventEnd(IListener *listener,int event){addEvent(listener,event);} 
  void DuctteipLog::addEventEnd(string  text,int event){addEvent(text,event);} 
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
  bool DuctteipLog::isSequentialPairEvent(int e){
    if ( 
	e == TaskDefined  ||
	e == ReadTask  ||
	e == CheckedForListener ||
	e == CheckedForTask ||
	e == CheckedForRun ||
	e == SuperGlueTaskDefine ||
	e == MPITestSent ||
	e == MPIReceive ||
	e == MPIProbed ||
	e == MailboxGetEvent ||
	e == LastReceiveCompleted ||
	e == AnySendCompleted ||
	e == MailboxProcessed ||
	e == CheckedForTerminate ||
	e == ProgramExecution ||
	e == CommFinish ||
	e == WorkProcessed
	 )
      return true;
    return false;
  }
/*----------------------------------------------------------------------------*/
  bool DuctteipLog::isPairEvent(int e){
    if ( 
	e == DataDefined  ||
	e == DataPartitioned  ||
	e == TaskDefined  ||
	e == ReadTask  ||
	e == Populated ||
	e == CheckedForListener ||
	e == CheckedForTask ||
	e == CheckedForRun ||
	e == SuperGlueTaskDefine ||
	e == Executed ||
	e == MPITestSent ||
	e == MPIReceive ||
	e == MPIProbed ||
	e == MailboxGetEvent ||
	e == LastReceiveCompleted ||
	e == AnySendCompleted ||
	e == MailboxProcessed ||
	e == CheckedForTerminate ||
	e == ProgramExecution ||
	e == CommFinish ||
	e == WorkProcessed 
	 )
      return true;
    return false;
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::filterEvent(EventInfo *e){
    static bool after_populate = false;
    
    if (e->event_id  == CheckedForTerminate || 
	e->event_id  == CheckedForRun 
	/*
	  ||
	  e->event_id  == SuperGlueTaskCount ||
	  e->event_id  == WorkProcessed ||
	  e->event_id  == MPITestSent ||
	  e->event_id  == MPIReceive ||
	  e->event_id  == MPIProbed ||
	  e->event_id  == LastReceiveCompleted ||
	  e->event_id  == AnySentCompleted ||
	  e->event_id  == MailboxGetEvent ||
	  e->event_id  == MailboxProcessed
	*/
	){
      e->event_id = -1;
    }
    return ;
    
    if ( e->event_id  == Populated){
      after_populate = true;
      e->event_id = -1;
    }
  }
/*----------------------------------------------------------------------------*/
  void DuctteipLog::mergePairEvents(){
    unsigned long merged_count =0;
    list<EventInfo *>::iterator it,it2;
    fprintf(stderr,"log merge,events.count :%ld\n",events_list.size());
    
    for ( it = events_list.begin();it !=  events_list.end(); it++) {
      EventInfo *e1 = (*it);
      if (e1->event_id <0) continue;
      if ( isPairEvent(e1->event_id)){
	it2 = it;
	while  (it2 != events_list.end() ) {
	  it2 ++;
	  if ( it2 == events_list.end() ) break;
	  EventInfo *e2=(*it2);
	  if (e1->event_id == e2->event_id && e1->handle == e2->handle) {
	    e2->event_id =-1;
	    updateStatistics(e1->event_id,e2->start - e1->start);
	    e1->length = e2->start - e1->start;
	    merged_count ++;
	    break;
	  }
	}
      }
      else
	updateStatistics(e1->event_id,e1->length);
      
    }
  }
/*----------------------------------------------------------------------------*/
   void DuctteipLog::dump(long sg_task_count){
    char s[20];
    list<EventInfo *>::iterator it;
    sprintf(s,"dt_log_file-%2.2d.txt",me);

    dumpLoads();

    ofstream log_file(s);
    TimeUnit t = getTime();
    if ( N < BIG_SIZE){
      for (it = events_list.begin();it != events_list.end();it ++){
	EventInfo *e =(*it);
	filterEvent(e);
	e->dump(log_file);
      }
    }
    
    t = getTime() - t;
    stats[EventsWork].count = events_list.size();
    stats[EventsWork].total = t / SCALE;
    stats[EventsWork].minimum = stats[EventsWork].maximum = 0;
    
    stats[SkipOverhead].count /= SCALE;    
    dumpSimulationResults(sg_task_count);
    if ( !simulation)
      dumpStatistics();
    if ( me == 0)
      fprintf(stderr,"[****] Time= %lf, N = %ld , NB= %ld , nb= %ld , p= %ld, q = %ld, gf= %lf\n",
	     stats[ProgramExecution].total,N,NB,nb,p,q,double(N*N*N/3e9/stats[ProgramExecution].total));
    log_file.close();
  }
/*----------------------------------------------------------------------------*/
   void DuctteipLog::dumpStatistics(){
     return ; 
     char dash[10]="---------";
     fprintf(stderr,"@stats Node\tEvent\t\t Count\t Sum.Duration\t Average\t Min\t\t Max\t\t\n");
     for ( int i=0; i < NumberOfEvents ; i ++){
       if (stats[i].minimum == MIN_INIT_VAL ) 
	 stats[i].minimum =0;
       if ( !isPairEvent(i) && i != EventsWork){
	fprintf(stderr,"@stats %d %22.22s %6ld\t %10s \t %10s \t %10s \t %10s\n",
	       me,getEventName(i).c_str(),stats[i].count,dash,dash,dash,dash);
      }
      else
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
	   N,nodes,p,q,NB,nb,me,sg_task_count,stats[TaskDefined].count,
	   stats[DataSent].count,stats[DataSent].count*DataPackSize, 
	   stats[DataReceived].count,stats[DataReceived].count*DataPackSize, 
	   cores,N/NB/nb,stats[ProgramExecution].total);
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

void addLogEventStart(string s , int e ) {  dt_log.addEventStart(s,e);}
void addLogEventEnd  (string s , int e ) {  dt_log.addEventEnd  (s,e);}

/*----------------------------------------------------------------------------*/
Timer::Timer(int e, const char *f, int l, const char *fx):event(e),line(l){
  st = getTime();
  strcpy(file,f );
  strcpy(func,fx);
  dt_log.addEventNumber(1,e);
}
/*----------------------------------------------------------------------------*/
Timer::~Timer(){
  end = getTime();
  long dur = (end-st)/1000000;
  fprintf(stderr,"%20s,%4d, %-32s, tid:%9X, %6ld ::",file,line,func,(ulong)pthread_self(),UserTime()); 
  fprintf(stderr,"event:%22s, dur:%ld\n",dt_log.getEventName(event).c_str(),dur);
  dt_log.addEventSummary(event,st,dur);
}

