#ifndef __DUCTTEIP_TASK_HPP__
#define __DUCTTEIP_TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "basic.hpp"
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
class DuctTeip_Data;
class IData;
class DataVersion;
class DataHandle;
//template <typename T > class Handle;
class IContext;



typedef unsigned long TaskHandle;
/*======================= IDuctteipTask ==============================================*/
class IDuctteipTask
{
private:
  string              name;
  list<DataAccess *> *data_list;
  int                 state,sync,type,host;
  TaskHandle          handle;
  unsigned long       key,comm_handle;
  IContext           *parent_context;
  PropagateInfo      *prop_info;
  MessageBuffer      *message_buffer;
  pthread_mutex_t    task_finish_mx;
  pthread_mutexattr_t  task_finish_ma;
  Handle<Options>    *sg_handle;
  bool                exported,imported;
public:
  enum TaskType{
    NormalTask,
    PropagateTask
  };
  enum TaskState{
    WaitForData=2,
    Running,
    Finished,
    UpgradingData,
    CanBeCleared
  };
  /*--------------------------------------------------------------------------*/
  IDuctteipTask (IContext *context,
		 string _name,
		 unsigned long _key,
		 int _host, 
		 list<DataAccess *> *dlist);
  ~IDuctteipTask();
  IDuctteipTask();
  IDuctteipTask(PropagateInfo *P);
  pthread_mutex_t * getTaskFinishMutex();
  void createSyncHandle();
  Handle<Options> *getSyncHandle();
  DuctTeip_Data *getArgument(int index);
  IData *getDataAccess(int index);
  void    setHost(int h )    ;
  int     getHost()          ;
  string  getName()          ;
  unsigned long getKey();
  void       setHandle(TaskHandle h)     ;
  TaskHandle getHandle()                 ;
  void          setCommHandle(unsigned long h) ;
  unsigned long getCommHandle()                ;
  list<DataAccess *> *getDataAccessList() ;
  void dumpDataAccess(list<DataAccess *> *dlist);
  void dump(char c=' ');
  bool isFinished();
  int getState();
  void setState( int s) ;
  void    setName(string n ) ;
  int getPackSize();
  bool canBeCleared() ;
  bool isUpgrading()  ;
  void upgradeData(char);
  bool canRun(char c=' ');
  void setFinished(bool f);
  void run();
  int serialize(byte *buffer,int &offset,int max_length);
  MessageBuffer *serialize();
  void deserialize(byte *buffer,int &offset,int max_length);
  void setExported(bool f) ;
  bool isExported();
  void setImported(bool f) ;
  bool isImported();
  bool isRunning();
  void runPropagateTask();
};
/*======================= IDuctteipTask ==============================================*/

#endif //__DUCTTEIP_TASK_HPP__
