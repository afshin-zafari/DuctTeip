#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

#define logEvent(a,b) 



typedef unsigned char byte;
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <algorithm>
#include <vector>
#include <iterator>

#include <dirent.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>


#include "config.hpp"
#include "dt_task.hpp"
#include "listener.hpp"
#include "mailbox.hpp"
#include "mpi_comm.hpp"
#include "memory_manager.hpp"
#include "dt_log.hpp"
extern int me;

struct PropagateInfo;
extern DuctteipLog dt_log;
extern Config config;

struct DuctTeipWork{
public:
  enum WorkTag{
    TaskWork,
    ListenerWork,
    DataWork
  };
  enum WorkEvent{
    Ready,
    Sent,
    Finished,
    DataSent,
    DataReceived,
    DataUpgraded,
    DataReady,
    Received,
    Added
  };
  enum TaskState{
    Initialized,
    WaitForData,
    Running
  };
  enum WorkItem{
    CheckTaskForData,
    CheckTaskForRun,
    SendTask,
    CheckListenerForData,
    UpgradeData,
    CheckAfterDataUpgraded,
    SendListenerData
  };
  IDuctteipTask *task;
  IData *data;
  IListener *listener;
  int state,tag,event,host,item;
  //  DuctTeipWork(){ }
  DuctTeipWork &operator =(DuctTeipWork *_work);
  void dump();
};
/*===================================================================*/
class engine
{
private:
  list<IDuctteipTask*>  task_list,running_tasks,export_tasks,import_tasks;
  bool                  runMultiThread;
  byte                  *prop_buffer;
  int             	term_ok;
  int             	prop_buffer_length,num_threads,local_nb;
  MailBox        	*mailbox;
  INetwork         	*net_comm;
  list<DuctTeipWork *> 	work_queue;
  pthread_t 		thread_id,mbsend_tid,mbrecv_tid;
  pthread_mutex_t 	thread_lock,work_ready_mx;
  pthread_mutexattr_t 	mutex_attr;
  pthread_cond_t 	work_ready_cv;
  pthread_attr_t 	attr;
  long                  last_task_handle,last_listener_handle;
  list<IListener *> 	listener_list;
  TimeUnit 		start_time,wasted_time;
  ClockTimeUnit 	start_clktime;
  SuperGlue<Options>    *thread_manager;
  Config 		*cfg;
  MemoryManager 	*data_memory;
  long 			skip_term,skip_work,skip_mbox,time_out;
  int 			thread_model;
  bool 			task_submission_finished;
  enum { Enter, Leave};
  enum {
    EVEN_INIT     ,
    WAIT_FOR_FIRST  ,
    FIRST_RECV    ,
    WAIT_FOR_SECOND ,
    SECOND_RECV   ,
    TERMINATE_INIT=100,
    LEAF_TERM_OK  =101,
    ONE_CHILD_OK  =102,
    ALL_CHILDREN_OK=103,
    SENT_TO_PARENT=104,
    TERMINATE_OK
  };
public:
  enum { MAIN_THREAD, ADMIN_THREAD,MBSEND_THREAD,MBRECV_THREAD,TASK_EXEC_THREAD};
  /*---------------------------------------------------------------------------------*/
  engine();
  void start ( int argc , char **argv);
  void initComm();
  ~engine();
  void setSkips(long st,long sw,long sm,long to);
  SuperGlue<Options> * getThrdManager() ;
  int getLocalNumBlocks();
  TaskHandle  addTask(IContext * context,
		      string task_name,
		      unsigned long key, 
		      int task_host, 
		      list<DataAccess *> *data_access);
  void dumpTasks();
  void sendPropagateTask(byte *buffer, int size, int dest_host) ;
  void addPropagateTask(PropagateInfo *P);
  void receivePropagateTask(byte *buffer, int len);
  void sendTask(IDuctteipTask* task,int destination);
  void exportTask(IDuctteipTask* task,int destination);
  /*---------------------------------------------------------------------------------*/
  void receivedListener(MailBoxEvent *event);
  void receivedData(MailBoxEvent *event,MemoryItem*);
  IData *importedData(MailBoxEvent *event,MemoryItem*);
  /*---------------------------------------------------------------------------------*/
  void waitForTaskFinish();
  void finalize();
  void globalSync();
  TimeUnit elapsedTime(int scale);
  void dumpTime(char c=' ');
  bool isAnyUnfinishedListener();
  long getUnfinishedTasks();
  bool canTerminate();
  int  getTaskCount();
  void show_affinity();
  void setConfig(Config *cfg_);
  void prepareReceives();
  void doProcess();
  static void *doProcessLoop  (void *);
  static void *runMBSendThread(void *);
  static void *runMBRecvThread(void *);
  void signalWorkReady(IDuctteipTask *p=NULL);
  bool mb_canTerminate(int send_or_recv);
  void putWorkForDataReady(list<DataAccess *> *data_list);
  MemoryItem *newDataMemory();
private :
  void putWorkForCheckAllTasks();
  void putWorkForReceivedListener(IListener *listener);
  void putWorkForSendingTask(IDuctteipTask *task);
  void putWorkForPropagateTask(IDuctteipTask *task);
  void putWorkForNewTask(IDuctteipTask *task);
  void putWorkForReceivedTask(IDuctteipTask *task);
  void putWorkForFinishedTask(IDuctteipTask * task);
  void putWorkForSingleDataReady(IData* data);
  void receivedTask(MailBoxEvent *event);
  void importedTask(MailBoxEvent *event);
  long int checkRunningTasks(int v=0);
  void  checkMigratedTasks();
  void  checkImportedTasks();
  void  checkExportedTasks();
  void doProcessWorks();
  bool isDuplicateListener(IListener * listener);
  bool addListener(IListener *listener );
  void checkTaskDependencies(IDuctteipTask *task);
public:
  void runFirstActiveTask();
private:
  long getActiveTasksCount();
  void executeTaskWork(DuctTeipWork * work);
  void executeDataWork(DuctTeipWork * work);
  void executeListenerWork(DuctTeipWork * work);
  void executeWork(DuctTeipWork *work);
  void doProcessMailBox();
  void processEvent(MailBoxEvent &event);
  IListener *getListenerForData(IData *data);
  void dumpListeners();
  void dumpAll();
  void removeListenerByHandle(int handle ) ;
  void removeTaskByHandle(TaskHandle task_handle);
  void criticalSection(int direction);
  IDuctteipTask *getTaskByHandle(TaskHandle  task_handle);
  IDuctteipTask *getTaskByCommHandle(unsigned long handle);
  IListener *getListenerByCommHandle ( unsigned long  comm_handle ) ;
  void resetTime();
  void sendTerminateOK_old();
  void receivedTerminateOK_old();
    inline bool IsOdd (int a);
    inline bool IsEven(int a);
  /*---------------------------------------------------------------------------------*/
  void createThreads();
  int getParentNodeInTree(int node);
  void getChildrenNodesInTree(int node,int *nodes,int *count);
  bool amILeafInTree();
  void sendTerminateOK();
  void receivedTerminateOK(int from);
  void receivedTerminateCancel(int from);
  void sendTerminateCancel();
  /*======================================================================*/
  struct DLB_Statistics{
    unsigned long tot_try,
      tot_failure,
      tot_tick,
      max_loc_fail,export_task,export_data,import_task,import_data,max_para,max_para2;
    ClockTimeUnit tot_cost,
      tot_silent;      
  };
  DLB_Statistics dlb_profile;
  int dlb_state,dlb_prev_state,dlb_substate,dlb_stage,dlb_node;
  unsigned long dlb_failure,dlb_glb_failure;
  ClockTimeUnit dlb_silent_start;
  enum DLB_STATE{
    DLB_IDLE,
    DLB_BUSY,
    TASK_EXPORTED,
    TASK_IMPORTED,
    DLB_STATE_NONE
  };
  enum DLB_SUBSTATE{
    ACCEPTED,
    EARLY_ACCEPT
  };
  enum DLB_STAGE{
    DLB_FINDING_IDLE,
    DLB_FINDING_BUSY,
    DLB_SILENT,
    DLB_NONE
  };
  enum Limits{
    SILENT_PERIOD=100,
    FAILURE_MAX=5
  };
  struct TimeDLB
  {
    ClockTimeUnit t;
    engine *e;
    TimeDLB(engine *_e):e(_e){t = getClockTime(MICRO_SECONDS);}
    ~TimeDLB(){
      e->dlb_profile.tot_cost += getClockTime(MICRO_SECONDS) - t;
    }
  };
  int failures[FAILURE_MAX];
#define TIME_DLB TimeDLB(this)
  /*---------------------------------------------------------------*/
  bool passedSilentDuration();
  void initDLB();
  void updateDLBProfile();
  void restartDLB(int s=-1);    
  void goToSilent();
  void dumpDLB();
  int getDLBStatus();
  void doDLB(int st=-1);
  void findBusyNode();
  void findIdleNode();
  void received_FIND_IDLE(int p);
  void receivedImportRequest(int p); 
  void received_FIND_BUSY(int p ) ;
  void receivedExportRequest(int p);
  void received_DECLINE(int p);
  void receivedDeclineMigration(int p);
  IDuctteipTask *selectTaskForExport();
  void exportTasks(int p);
  void importData(MailBoxEvent *event);
  void receiveTaskOutData(MailBoxEvent *event);
  void receivedMigrateTask(MailBoxEvent *event);
  void received_ACCEPTED(int p);
  void declineImportTasks(int p);    
  void declineExportTasks(int p);    
  /*---------------------------------------------------------------*/
  void acceptImportTasks(int p);    
  /*---------------------------------------------------------------*/
private:
  vector<int> dlb_fail_list;
public:
  bool isInList(vector<int>&L,int v);
  int getRandomNodeEx();
  int getRandomNodeOld(int exclude =-1);
  void setThreadModel(int);
  int  getThreadModel();
  long getThreadId(int);
  MemoryManager *getMemoryManager();
  void doSelfTerminate();
  void waitForWorkReady();
};

#define  DuctTeip_Submit(k,args...) AddTask((IContext*)this,#k,k,args)
void AddTask ( IContext *,string ,unsigned long,IData *,IData *d2,IData *d3);
void AddTask ( IContext *,string ,unsigned long,IData *                    );
void AddTask ( IContext *,string ,unsigned long,IData *,IData *d2          );

extern engine dtEngine;
#endif //__ENGINE_HPP__
