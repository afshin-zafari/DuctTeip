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
#include "eng_work.hpp"

extern int me;

struct PropagateInfo;
extern DuctteipLog dt_log;
extern Config config;

/*===================================================================*/
class engine{
private:
  friend class DLB;
  /*--------------------THREAD-------------------------------------------------------*/
  bool                  runMultiThread;
  pthread_t 		thread_id,mbsend_tid,mbrecv_tid;
  pthread_mutex_t 	thread_lock,work_ready_mx;
  pthread_mutexattr_t 	mutex_attr;
  pthread_cond_t 	work_ready_cv;
  pthread_attr_t 	attr;
  int 			thread_model;
  /*--------------------TASK,DATA, LISTENER -----------------------------------------*/
  list<IDuctteipTask*>  task_list,running_tasks,export_tasks,import_tasks;
  long                  last_task_handle,last_listener_handle;
  bool 			task_submission_finished;
  ulong                 dps;
  /*--------------------PROFILING----------------------------------------------------*/
  map<long,double>      avg_durations;
  map<long,long>        cnt_durations;
  TimeUnit 		start_time,wasted_time;
  ClockTimeUnit 	start_clktime;
  long time_out;
  /*--------------------PROPAGATION -------------------------------------------------*/
  byte                  *prop_buffer;
  int             	prop_buffer_length,num_threads,local_nb;
  /*--------------------OBJECTS     -------------------------------------------------*/

  MailBox        	*mailbox;
  INetwork         	*net_comm;
  list<DuctTeipWork *> 	work_queue;
  list<IListener *> 	listener_list;
  SuperGlue<Options>    *thread_manager;
  Config 		*cfg;
  MemoryManager 	*data_memory;
  int             	term_ok;
  enum {
    EVEN_INIT            ,
    WAIT_FOR_FIRST       ,
    FIRST_RECV           ,
    WAIT_FOR_SECOND      ,
    SECOND_RECV          ,
    TERMINATE_INIT  = 100,
    LEAF_TERM_OK    = 101,
    ONE_CHILD_OK    = 102,
    ALL_CHILDREN_OK = 103,
    SENT_TO_PARENT  = 104,
    TERMINATE_OK
  };
public:
  enum Fence{Enter,Leave};
  enum { MAIN_THREAD, ADMIN_THREAD,MBSEND_THREAD,MBRECV_THREAD,TASK_EXEC_THREAD};
  /*---------------------------------------------------------------------------------*/
  engine();
  void start ( int argc , char **argv,bool sg = false);
  void initComm();
  ~engine();
  /*---------------------------- THREAD STUFF---------------------------------------*/
  SuperGlue<Options> * getThrdManager() ;
  static void *runMBSendThread(void *);
  static void *runMBRecvThread(void *);
  void signalWorkReady(IDuctteipTask *p=NULL);
  void setThreadModel(int);
  int  getThreadModel();
  long getThreadId(int);
  void doSelfTerminate();
  void criticalSection(int direction);
  bool mb_canTerminate(int send_or_recv);
  /*---------------------------- TASK  STUFF---------------------------------------*/
  list<IDuctteipTask*>  &getRunningTasksList(){return running_tasks;}
  void register_task(IDuctteipTask*);
  TaskHandle  addTask(IContext * context,
		      string task_name,
		      ulong  key, 
		      int task_host, 
		      list<DataAccess *> *data_access);
  void dumpTasks();
  void sendTask(IDuctteipTask* task,int destination);
  void exportTask(IDuctteipTask* task,int destination);
  int  getTaskCount();
  IDuctteipTask *getTaskByHandle(TaskHandle  task_handle);
  IDuctteipTask *getTask(TaskHandle th);
  void receivedTask(MailBoxEvent *event);
  void importedTask(MailBoxEvent *event);
  long int checkRunningTasks(int v=0);
  void  checkMigratedTasks();
  void  checkImportedTasks();
  void  checkExportedTasks();
  void checkTaskDependencies(IDuctteipTask *task);
  long getActiveTasksCount();
  void removeTaskByHandle(TaskHandle task_handle);
  IDuctteipTask *getTaskByCommHandle(unsigned long handle);
  void waitForTaskFinish();
  long getUnfinishedTasks();
  void runFirstActiveTask();
  
  /*-----------------------------DATA & LISTENER --------------------------------------*/
  void receivedData(MailBoxEvent *event,MemoryItem*);
  IData *importedData(MailBoxEvent *event,MemoryItem*);
  ulong getDataPackSize();
  void receivedListener(MailBoxEvent *event);
  bool isDuplicateListener(IListener * listener);
  bool addListener(IListener *listener );
  IListener *getListenerForData(IData *data);
  void dumpListeners();
  void removeListenerByHandle(int handle ) ;
  IListener *getListenerByCommHandle ( unsigned long  comm_handle ) ;
  bool isAnyUnfinishedListener();
 
  /*----------------------------ADMINISTRATION-----------------------------------------*/
  template <typename T>
  void set_superglue(SuperGlue<T> * );
  int getLocalNumBlocks();
  void show_affinity();
  void setConfig(Config *cfg_,bool = false);
  MailBox *getMailBox(){return mailbox;}


  void finalize();
  void globalSync();

  TimeUnit elapsedTime(int scale);
  void dumpTime(char c=' ');
  void dumpAll();
  void resetTime();  
  long getAverageDur(long k);
  void updateDurations(IDuctteipTask *task);

  bool canTerminate();
  void doProcess();
  void processEvent(MailBoxEvent &event);
  void doProcessMailBox();
  void doDLB(int st=-1);
  static void *doProcessLoop  (void *);
  /*------------------MEMORY---------------------------------------------------------*/
  MemoryManager *getMemoryManager();
  MemoryItem *newDataMemory();
  void insertDataMemory(IData *,byte *);  
  /*------------------WORK  ---------------------------------------------------------*/
  void doProcessWorks();
  void waitForWorkReady();
  void putWorkForDataReady(list<DataAccess *> *data_list);
  void putWorkForSingleDataReady(IData* data);
  void putWorkForCheckAllTasks();
  void putWorkForReceivedListener(IListener *listener);
  void putWorkForSendingTask(IDuctteipTask *task);
  void putWorkForNewTask(IDuctteipTask *task);
  void putWorkForReceivedTask(IDuctteipTask *task);
  void putWorkForFinishedTask(IDuctteipTask * task);
  void executeTaskWork(DuctTeipWork * work);
  void executeDataWork(DuctTeipWork * work);
  void executeListenerWork(DuctTeipWork * work);
  void executeWork(DuctTeipWork *work);
  /*---------------TERMINATE      --------------------------------------------------*/
  inline bool IsOdd (int a);
  inline bool IsEven(int a);
  void sendTerminateOK_old();
  void receivedTerminateOK_old();
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
