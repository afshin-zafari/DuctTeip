#ifndef __DUCTTEIP_TASK_HPP__
#define __DUCTTEIP_TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <string>
//#include <atomic>
#include "basic.hpp"
#include "config.hpp"
#include "data_basic.hpp"
#include "partition.hpp"
#include <sstream>

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

using namespace std;
struct PropagateInfo;
class IData;
class DataVersion;
class DataHandle;
//template <typename T > class Handle;
class IContext;
class IDuctteipTask;
class LastLevel_Data;

/*======================= IDuctteipTask ==============================================*/
struct KernelTask : Task<Options>
{
    IDuctteipTask* dt_task;
    KernelTask(IDuctteipTask*);
    ~KernelTask();
  void run(TaskExecutor<Options> &);
    string get_name();
};
  /*--------------------------------------------------------------------------*/
class SuperGlueTaskBase:public Task<Options> {
private:
  IDuctteipTask* dt_task;
  TaskExecutor<Options> *task_executor;
protected:
  string name;
public:
  SuperGlueTaskBase(IDuctteipTask* d);
  void run(TaskExecutor<Options> &te) {
    task_executor= &te;
    //LOG_INFO(LOG_TASKS,"sgTask.run().\n");//,dt_task->getName().c_str());
    if (config.simulation) return;
    runKernel();
    }
  virtual void runKernel() =0;
  string get_name(){return name;}//string("basic_task_for_kernels");}
  LastLevel_Data  &get_argument(int);
};
  /*--------------------------------------------------------------------------*/
class SyncTask :public Task<Options> {
private:
  IDuctteipTask* dt_task;
  string name;
public:
  SyncTask(IDuctteipTask *);
  ~SyncTask();
  void run(TaskExecutor<Options> &){}
  string get_name(){return name;}
};

typedef unsigned long TaskHandle;
/*======================= IDuctteipTask ==============================================*/
class IDuctteipTask
{
protected:
    string                 name;
    list<DataAccess *>    *data_list;
    int                    sync,type,host;
    int                    state;
    TaskHandle             handle;
    unsigned long          key,comm_handle;
    IContext              *parent_context;
    PropagateInfo         *prop_info;
    MessageBuffer         *message_buffer;
    pthread_mutex_t        task_finish_mx;
    pthread_mutexattr_t    task_finish_ma;
    bool                   exported,imported;
    TaskExecutor<Options> *te;
    TimeUnit               start,end,exp_fin;
    void                  *guest;
    SyncTask              *syncTask;
public:
    Handle<Options>       *sg_handle;
    TaskBase<Options>     *sg_task;
  IDuctteipTask           *task_parent;
  int child_count;
    enum TaskType
    {
        NormalTask,
        PropagateTask
    };
    enum TaskState
    {
        WaitForData=2,
        Running,
        Finished,
        UpgradingData,
        CanBeCleared,
	Cleared
    };
    /*--------------------------------------------------------------------------*/
    IDuctteipTask();
    IDuctteipTask(PropagateInfo *P);
    IDuctteipTask(IContext *,string,ulong,int,list<DataAccess *> *);
    virtual ~IDuctteipTask();
    void createSyncHandle();
    Handle<Options> *getSyncHandle();
    IData *getArgument(int index);
    IData  *getDataAccess(int index);
    void    setHost(int h )    ;
    int     getHost()          ;
    string  getName()          ;
    string get_name();
    ulong   getKey();
    void    setHandle(TaskHandle h)     ;
    TaskHandle getHandle()                 ;
    void       setCommHandle(unsigned long h) ;
    ulong getCommHandle()                ;
    list<DataAccess *> *getDataAccessList() ;
    void setDataAccessList(list<DataAccess *>*) ;
    void dumpDataAccess(list<DataAccess *> *dlist);
    void dump(char c=' ');
    bool isFinished();
    int  getState();
    void setState( int s) ;
    void setName(string n ) ;
    int  getPackSize();
    bool canBeCleared() ;
    bool isCleared() ;
    bool isUpgrading()  ;
    void upgradeData(char);
    bool canRun(char c=' ');
    void setFinished(bool f);
    void run();
  virtual void runKernel(){}
    int  serialize(byte *buffer,int &offset,int max_length);
    MessageBuffer *serialize();
    void deserialize(byte *buffer,int &offset,int max_length);
    void setExported(bool f) ;
    bool isExported();
    void setImported(bool f) ;
    bool isImported();
    bool isRunning();
    IContext *getParentContext();
    TaskExecutor<Options> *getTaskExecutor();
    void setTaskExecutor(TaskExecutor<Options> *);
    TaskBase<Options>*getKernelTask();
    TimeUnit getExpFinish();
    ulong  getMigrateSize();
    TimeUnit getDuration();
  void *get_guest();
  void  set_guest(void *);
  void subtask(SuperGlueTaskBase *);
  void submission_finished();
  virtual void finished(){};
};
/*======================= IDuctteipTask ==============================================*/
typedef IDuctteipTask DuctTeip_Task;
#endif //__DUCTTEIP_TASK_HPP__
