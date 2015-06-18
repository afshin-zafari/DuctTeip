#ifndef __DT_LOG_HPP__
#define __DT_LOG_HPP__
#include <iomanip>
#include "listener.hpp"

#define SG_SCHEDULE 1
class engine;
extern int me;
#ifdef MPI_WALL_TIME
#  pragma message("Scale=1e9")
static const double SCALE=1000000000.0;
#else
#  if SG_SCHEDULE==1
#    pragma message("Scale=1.0")
static const double SCALE=1.0;
#  else
#    pragma message("Scale=3e9")
static const double SCALE=3.0e9;
#  endif
#endif
static const double MIN_INIT_VAL=1000000.0;
typedef struct {
  double average,maximum,minimum,total;
  unsigned long count;
  TimeUnit start;
}Statistics;
typedef unsigned long ulong;
extern int simulation;
/*----------------------------------------------------------------------------*/

/*============================================================================*/
 struct EventInfo {
 public:
   int proc_id,thread_id,event_id;
   TimeUnit start,length;
   string text;
   ulong handle;
/*----------------------------------------------------------------------------*/
   EventInfo(){}
/*----------------------------------------------------------------------------*/
   EventInfo(int id_,const string &info,ulong h = 0){
    event_id = id_;
    proc_id = me ; 
    thread_id = pthread_self(); 
    start = getTime();
    length = 0;
    handle =h;
    text.assign(info);
  }
/*----------------------------------------------------------------------------*/
 EventInfo & operator =(EventInfo &rhs){
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
  void dump(ofstream &log_file){
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
};
/*----------------------------------------------------------------------------*/
class DuctteipLog{
private:
  string log_filename;
  struct load {
    TimeUnit t; long val;
    load(TimeUnit _t, long l):t(_t),val(l){}
  };
  list<load*> load_list,export_list,import_list;
  list<EventInfo*> events_list;
  long DataPackSize, ListenerPackSize, TaskPackSize;
  long FLOPS_add,FLOPS_mul,FLOPS_nlin;
  long *commStats[3];
  int  nodes;
  static const  int BIG_SIZE=216010;
public:
  unsigned long N,NB,nb,p,q,cores;
  enum event_t {
    DataDefined,                  // paired, synch.
    DataPartitioned,              // paired, synch.
    Populated,                    // paired, synch.    
    TaskDefined,                  // paired, synch.
    ReadTask,                     // paired, synch.
    TaskSent,                     // single, asynch.
    ListenerDefined,              // paired, synch.
    ListenerSent,                 // single, asynch.
    TaskReceived,                 // single, synch.
    ListenerReceived,             // single, synch.
    DataReceived,                 // single, synch.
    DataSendCompleted,            // single, synch.
    TaskSendCompleted,            // single, synch.
    ListenerSendCompleted,        // single, synch.
    CheckedForListener,           // paired, synch.
    CheckedForTask,               // paired, synch.
    CheckedForRun,                // paired, synch.
    SuperGlueTaskDefine,          // paired, synch.
    Executed,                     // paired, asynch.
    RunTimeVersionUpgraded,       // single, synch.
    Woken,                        // single, synch.
    DataSent,                     // single, asynch.
    SkipOverhead,                 // paired, synch.
    MPITestSent,                  // paired, synch.
    MPIReceive,                   // paired, synch.
    MPIProbed,                    // paired, synch.
    AnySendCompleted,             // paired, synch.
    LastReceiveCompleted,         // paired, synch.
    MailboxGetEvent,              // paired, synch.
    MailboxProcessed,             // paired, synch.
    WorkProcessed,                // paired, synch.
    CheckedForTerminate,          // paired, synch.
    CommFinish,                   // paired, synch.
    ProgramExecution,             // paired, synch.
    EventsWork,                   // it's a local counter
    TaskExported,
    TaskImported,
    DataExported,
    DataImported,
    NumberOfEvents
  };
  Statistics stats[NumberOfEvents];
/*----------------------------------------------------------------------------*/
  ~DuctteipLog(){
  }
/*----------------------------------------------------------------------------*/
  DuctteipLog(){
    FLOPS_add = FLOPS_mul = FLOPS_nlin = 0;
    for ( int i =0 ; i < NumberOfEvents; i ++){
      stats[i].minimum = MIN_INIT_VAL;
      stats[i].maximum = stats[i].total = stats[i].count = 0;
      stats[i].start=-1;
    }
  }
/*----------------------------------------------------------------------------*/
  void updateStatisticsPair(int e ){
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
      printf("ProgExec , st:%ld end:%ld\n",stats[e].start,t ) ;
    }

  }
/*----------------------------------------------------------------------------*/
  string  getEventName(int event ) {
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
  void addEvent(IDuctteipTask * task,int event,int dest=-1){
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
  void addEvent(IData *data,int event,int dest=-1){
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
  void addEvent(engine *eng,int event){
    stringstream ss; 
    if ( N < BIG_SIZE || event == ProgramExecution){
      ss <<  getEventName(event)  << " ";
      EventInfo * event_item = new EventInfo (event,ss.str());
      events_list.push_back(event_item);
      updateStatisticsPair(event);
    }
    if ( event == ProgramExecution ) {
      printf ( "ProgExec index:%ld , dur:%lf\n",events_list.size(),stats[ProgramExecution].total ) ;
    }
  } 
/*----------------------------------------------------------------------------*/
  void addEvent(IListener *listener,int event,int dest =-1){
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
  void addEvent(string  text,int event){
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
  void addEventNumber (unsigned long num, int e ) {
    if (e >= NumberOfEvents) return;
    stats[e].count += num;
    stats[e].total = stats[e].minimum = stats[e].maximum = 0;
  }
/*----------------------------------------------------------------------------*/
  void setParams(int nodes_,long DataPackSize_, 
		 long ListenerPackSize_,long TaskPackSize_){
    DataPackSize = DataPackSize_;
    ListenerPackSize = ListenerPackSize_;
    TaskPackSize = TaskPackSize_;
    printf("@stats Msg Sizes D,T,L: %ld,%ld,%ld\n",DataPackSize,TaskPackSize,ListenerPackSize);
    nodes = nodes_;
    for ( int i =0; i< 3; i++){
      commStats[i] = new long[nodes];
      for ( int j =0; j< nodes; j++)
	commStats[i][j] = 0;
    }
  }
/*----------------------------------------------------------------------------*/
  void updateCommStats(int event, int dest ){
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
  void addEventStart(IDuctteipTask * task,int event){addEvent(task,event);} 
  void addEventStart(IData *data,int event){addEvent(data,event);} 
  void addEventStart(engine *eng,int event){addEvent(eng,event);}  
  void addEventStart(IListener *listener,int event){addEvent(listener,event);}  
  void addEventStart(string  text,int event){addEvent(text,event);}  

  void addEventEnd(IDuctteipTask * task,int event){addEvent(task,event);} 
  void addEventEnd(IData *data,int event){addEvent(data,event);} 
  void addEventEnd(engine *eng,int event){addEvent(eng,event);}  
  void addEventEnd(IListener *listener,int event){addEvent(listener,event);} 
  void addEventEnd(string  text,int event){addEvent(text,event);} 
/*----------------------------------------------------------------------------*/
  void updateStatistics(int e,TimeUnit length){
    double len = ((double)length)/SCALE;
    if ( e < 0 || e>= NumberOfEvents) return;
    stats[e].count ++;
    stats[e].minimum = ((stats[e].minimum > len)?len:stats[e].minimum);
    stats[e].maximum = ((stats[e].maximum < len)?len:stats[e].maximum);
    stats[e].total += len;
}
/*----------------------------------------------------------------------------*/
  bool isSequentialPairEvent(int e){
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
  bool isPairEvent(int e){
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
  void filterEvent(EventInfo *e){
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
  void mergePairEvents(){
    unsigned long merged_count =0;
    list<EventInfo *>::iterator it,it2;
    printf("log merge,events.count :%ld\n",events_list.size());
    
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
    PRINT_IF(1)("log merge,merged events:%ld\n",merged_count);
  }
/*----------------------------------------------------------------------------*/
   void dump(long sg_task_count){
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
      printf("[****] Time= %lf, N = %ld , NB= %ld , nb= %ld , p= %ld, q = %ld, gf= %lf\n",
	     stats[ProgramExecution].total,N,NB,nb,p,q,double(N*N*N/3e9/stats[ProgramExecution].total));
    log_file.close();
  }
/*----------------------------------------------------------------------------*/
   void dumpStatistics(){
     char dash[10]="---------";
     printf("@stats Node\tEvent\t\t Count\t Sum.Duration\t Average\t Min\t\t Max\t\t\n");
     for ( int i=0; i < NumberOfEvents ; i ++){
       if (stats[i].minimum == MIN_INIT_VAL ) 
	 stats[i].minimum =0;
       if ( !isPairEvent(i) && i != EventsWork){
	printf("@stats %d %22.22s %6ld\t %10s \t %10s \t %10s \t %10s\n",
	       me,getEventName(i).c_str(),stats[i].count,dash,dash,dash,dash);
      }
      else
	printf("@stats %d %22.22s %6ld\t %9.2lf \t %9.2lf \t %9.2lf \t %9.2lf\n",
	       me,getEventName(i).c_str(),
	       stats[i].count,
	       stats[i].total,
	       (stats[i].count ==0)?0.0:(double)stats[i].total/stats[i].count,
	       stats[i].minimum,
	       stats[i].maximum);
     }
   
     for ( int i=0; i< nodes; i++){
       if ( i == me ) continue;
       printf("@stats %d     SentDataSize %7ld ToNode %d\n",me,commStats[0][i],i);
       printf("@stats %d SentListenerSize %7ld ToNode %d\n",me,commStats[1][i],i);
       printf("@stats %d     SentTaskSize %7ld ToNode %d\n",me,commStats[2][i],i);
     }
   }
/*----------------------------------------------------------------------------*/
  void logExportTask(long count){
    load * l = new load(getTime(),count);
    export_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void logImportTask(long count){
    load * l = new load(getTime(),count);
    import_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void logLoadChange(long ld){
    load * l = new load(getTime(),ld);
    load_list.push_back(l);
  }
/*----------------------------------------------------------------------------*/
  void dumpLoad(list<load*> lst,char *s){
    list<load *>::iterator it;
    ofstream log_file(s);
    for (it = lst.begin();it != lst.end();it ++){
      load  *l =(*it);
      log_file << l->t << ' ' << l->val << endl;
    }
    log_file.close();    
  }
/*----------------------------------------------------------------------------*/
  void dumpLoads(){
    char s[20];
    sprintf(s,"dt_load_file-%2.2d.txt",me);
    dumpLoad(load_list,s);
    sprintf(s,"dt_export_file-%2.2d.txt",me);
    dumpLoad(export_list,s);
    sprintf(s,"dt_import_file-%2.2d.txt",me);
    dumpLoad(import_list,s);    
  }
/*----------------------------------------------------------------------------*/
  void dumpSimulationResults(long sg_task_count){
    printf("\n@Simulation: N=%ld,P=%d, p=%ld,q=%ld,B=%ld,b=%ld,k=%d,"\
	   "t=%ld,T=%ld,s=%ld,S=%ld,r=%ld,R=%ld,c=%ld,z=%ld,i=%lf\n",
	   N,nodes,p,q,NB,nb,me,sg_task_count,stats[TaskDefined].count,
	   stats[DataSent].count,stats[DataSent].count*DataPackSize, 
	   stats[DataReceived].count,stats[DataReceived].count*DataPackSize, 
	   cores,N/NB/nb,stats[ProgramExecution].total);
  }
/*----------------------------------------------------------------------------*/

};
extern DuctteipLog dt_log;
void addLogEventStart(string s , int e ) {  dt_log.addEventStart(s,e);}
void addLogEventEnd  (string s , int e ) {  dt_log.addEventEnd  (s,e);}
#endif //__DT_LOG_HPP__
