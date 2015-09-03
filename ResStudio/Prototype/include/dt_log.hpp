#ifndef __DT_LOG_HPP__ 
#define __DT_LOG_HPP__
#include <iomanip>
#include "dt_task.hpp"
#include "config.hpp"




#define SG_SCHEDULE 1
class engine;
class IListener;
extern int me;
#ifdef MPI_WALL_TIME
static const double SCALE=1000000000.0;
#else
#  if SG_SCHEDULE==1
static const double SCALE=1.0;
#  else
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
extern Config config;
/*----------------------------------------------------------------------------*/

/*============================================================================*/
 struct EventInfo {
 public:
   int      proc_id,thread_id,event_id;
   TimeUnit start,length;
   string   text;
   ulong    handle;
   /*++++++++++++++*/
   EventInfo();
   EventInfo(int id_,const string &info,ulong h = 0);
   
   EventInfo & operator =(EventInfo &rhs);
   
   void dump(ofstream &log_file);
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
  list<EventInfo*> events_list,summary_list;
  long DataPackSize, ListenerPackSize, TaskPackSize;
  long FLOPS_add,FLOPS_mul,FLOPS_nlin;
  long *commStats[3];
  int  nodes;
  static const  int BIG_SIZE=216010;
public:
  //unsigned long N,NB,nb,p,q,cores;
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
    TaskFinished,
    DLB,
    NumberOfEvents
  };
  Statistics stats[NumberOfEvents];
/*----------------------------------------------------------------------------*/
  ~DuctteipLog();
  DuctteipLog();
  void updateStatisticsPair(int e );
  string  getEventName(int event ) ;
  void addEvent(IDuctteipTask * task,int event,int dest=-1);
  void addEvent(IData *data,int event,int dest=-1);
  void addEvent(engine *eng,int event);
  void addEvent(IListener *listener,int event,int dest =-1);
  void addEvent(int event);
  void addEventSummary(int e, TimeUnit st, TimeUnit dur);
  void addEventNumber (unsigned long num, int e ) ;
  void setParams(int nodes_, long, long, long );
  void updateCommStats(int event, int dest );
/*----------------------------------------------------------------------------*/
  void addEventStart(IDuctteipTask * task,int event);
  void addEventStart(IData *data,int event);
  void addEventStart(engine *eng,int event);
  void addEventStart(IListener *listener,int event);
  void addEventStart(string  text,int event);

  void addEventEnd(IDuctteipTask * task,int event);
  void addEventEnd(IData *data,int event);
  void addEventEnd(engine *eng,int event);
  void addEventEnd(IListener *listener,int event);
  void addEventEnd(string  text,int event);
  void updateStatistics(int e,TimeUnit length);
  void dump(long sg_task_count);
  void dumpStatistics();
  void logExportTask(long count);
  void logImportTask(long count);
  void logLoadChange(long ld);
  void dumpLoad(list<load*> lst,char *s);
  void dumpLoads();
  void dumpSimulationResults(long sg_task_count);
  void logLoad(long run, long imp, long exp);
  double getStattime(int e);
};
/*----------------------------------------------------------------------------*/
class Timer{
public:
  int event,line;
  ulong *tot;
  char file[100],func[100];
  TimeUnit st,end;
   Timer(int e,const char *f, int l, const char *fx);
  Timer(ulong &,int e,const char *f, int l, const char *fx);
  ~Timer();
};
#if BUILD == RELEASE
#define TIMER(e)
#define TIMERX(a,e) 
#else
#define TIMER(e)    Timer _((int)(e),__FILE__,__LINE__,__FUNCTION__);
#define TIMERX(a,e) Timer a((int)(e),__FILE__,__LINE__,__FUNCTION__);
#define TIMERACC(a,e) Timer _(a,(int)(e),__FILE__,__LINE__,__FUNCTION__);
#endif

#define LOG_EVENT(e) dt_log.addEvent(e)
#define LOG_EVENTX(e,a) dt_log.addEvent(a,e)
#define LOG_METRIC(m) dt_log.addEventNumber(1,m)
#define LOG_LOAD dt_log.logLoad(dtEngine.running_tasks.size(),dtEngine.import_tasks.size(),dtEngine.export_tasks.size())

#define LOG_LOADZ dt_log.logLoad(0,0,0)
extern DuctteipLog dt_log;
#endif //__DT_LOG_HPP__
