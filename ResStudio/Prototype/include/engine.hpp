#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

#define logEvent(a,b) 



typedef unsigned char byte;
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <algorithm>
#include <vector>
#include <map>
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
#include "dlb.hpp"

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
class engine{
private:
  friend class DLB;
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
  //  long 			skip_term,skip_work,skip_mbox,
  long time_out;
  int 			thread_model;
  bool 			task_submission_finished;
  map<long,double>      avg_durations;
  map<long,long>        cnt_durations;
  ulong                 dps;
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
  enum Fence{Enter,Leave};
  enum { MAIN_THREAD, ADMIN_THREAD,MBSEND_THREAD,MBRECV_THREAD,TASK_EXEC_THREAD};
  /*---------------------------------------------------------------------------------*/
  engine();
  void start ( int argc , char **argv);
  void initComm();
  ~engine();
  SuperGlue<Options> * getThrdManager() ;
  int getLocalNumBlocks();
  list<IDuctteipTask*>  &getRunningTasksList(){return running_tasks;}
  MailBox *getMailBox(){return mailbox;}
  void register_task(IDuctteipTask*);
  TaskHandle  addTask(IContext * context,
		      string task_name,
		      ulong  key, 
		      int task_host, 
		      list<DataAccess *> *data_access);
  void dumpTasks();
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
  void doProcess();
  static void *doProcessLoop  (void *);
  static void *runMBSendThread(void *);
  static void *runMBRecvThread(void *);
  void signalWorkReady(IDuctteipTask *p=NULL);
  bool mb_canTerminate(int send_or_recv);
  void putWorkForDataReady(list<DataAccess *> *data_list);
  MemoryItem *newDataMemory();
  void runFirstActiveTask();
  long getAverageDur(long k);
  void setThreadModel(int);
  int  getThreadModel();
  long getThreadId(int);
  MemoryManager *getMemoryManager();
  void doSelfTerminate();
  void waitForWorkReady();
  void doDLB(int st=-1);
  void updateDurations(IDuctteipTask *task);
  IDuctteipTask *getTaskByHandle(TaskHandle  task_handle);
  void putWorkForSingleDataReady(IData* data);
  ulong getDataPackSize();
  IDuctteipTask *getTask(TaskHandle th);
  void criticalSection(int direction);
private :
  void putWorkForCheckAllTasks();
  void putWorkForReceivedListener(IListener *listener);
  void putWorkForSendingTask(IDuctteipTask *task);
  void putWorkForNewTask(IDuctteipTask *task);
  void putWorkForReceivedTask(IDuctteipTask *task);
  void putWorkForFinishedTask(IDuctteipTask * task);
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
  IDuctteipTask *getTaskByCommHandle(unsigned long handle);
  IListener *getListenerByCommHandle ( unsigned long  comm_handle ) ;
  void resetTime();
  void sendTerminateOK_old();
  void receivedTerminateOK_old();
    inline bool IsOdd (int a);
    inline bool IsEven(int a);
  /*---------------------------------------------------------------------------------*/
  int  getParentNodeInTree(int node);
  void getChildrenNodesInTree(int node,int *nodes,int *count);
  bool amILeafInTree();
  void sendTerminateOK();
  void receivedTerminateOK(int from);
  void receivedTerminateCancel(int from);
  void sendTerminateCancel();
  /*======================================================================*/
};

#define  DuctTeip_Submit(k,args...) AddTask((IContext*)this,#k,k,args)
void AddTask ( IContext *,string ,unsigned long,IData *,IData *d2,IData *d3);
void AddTask ( IContext *,string ,unsigned long,IData *                    );
void AddTask ( IContext *,string ,unsigned long,IData *,IData *d2          );

extern engine dtEngine;
#endif //__ENGINE_HPP__
