#ifndef __DUCTTEIP_TASK_HPP__
#define __DUCTTEIP_TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include "data.hpp"

struct PropagateInfo;

/*======================= Dataaccess =========================================*/
struct DataAccess{
  IData *data;
  DataVersion required_version;
  byte type;
  int getPackSize(){
    DataHandle dh;
    return dh.getPackSize() + required_version.getPackSize() ;
  }   
} ;

typedef unsigned long TaskHandle;
/*======================= ITask ==============================================*/
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
public:
  enum TaskType{
    NormalTask,
    PropagateTask
  };
  enum TaskState{
    WaitForData=2,
    Running,
    Finished,
    CanBeCleared
  };
  /*--------------------------------------------------------------------------*/
  IDuctteipTask (IContext *context,
		 string _name,
		 unsigned long _key,
		 int _host, 
		 list<DataAccess *> *dlist):
    host(_host),data_list(dlist){
    
    parent_context = context;    
    key = _key;
    if (_name.size() ==0  )
      _name.assign("task");
    setName(_name);
    comm_handle = 0 ;    
    state = WaitForData; 
    type = NormalTask;
    message_buffer = new MessageBuffer ( getPackSize(),0);
  }
  /*--------------------------------------------------------------------------*/
  ~IDuctteipTask(){
    TRACE_LOCATION;
    delete message_buffer;
    TRACE_LOCATION;
    printf("zzz\n");
  }  
  /*--------------------------------------------------------------------------*/
  IDuctteipTask():name(""),host(-1){
    state = WaitForData;
    type = NormalTask;
  }
  /*--------------------------------------------------------------------------*/
  IDuctteipTask(PropagateInfo *P);
  /*--------------------------------------------------------------------------*/
  IData *getDataAccess(int index){
    if (index <0 || index >= data_list->size()){
      return NULL;
    }
    list<DataAccess *> :: iterator it;
    for ( it = data_list->begin(); it != data_list->end() && index >0 ; it ++,index--){
    }
    return (*it)->data;
  }
  /*--------------------------------------------------------------------------*/
  void    setHost(int h )    { host = h ;  }
  int     getHost()          { return host;}
  string  getName()          { return name;}
  unsigned long getKey(){ return key;}

  void       setHandle(TaskHandle h)     { handle = h;}
  TaskHandle getHandle()                 {return handle;}

  void          setCommHandle(unsigned long h) { comm_handle = h;   }
  unsigned long getCommHandle()                { return comm_handle;}

  /*--------------------------------------------------------------------------*/
  list<DataAccess *> *getDataAccessList() { return data_list;  }

  /*--------------------------------------------------------------------------*/
  void dumpDataAccess(list<DataAccess *> *dlist){
    if (!DUMP_FLAG)
      return;
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("#daxs:%s @%d \n ", (*it)->data->getName().c_str(),(*it)->data->getHost());
      (*it)->required_version.dump();
      (*it)->data->dump();
    }
    printf ("\n");
  }
  /*--------------------------------------------------------------------------*/
  void dump();
  /*--------------------------------------------------------------------------*/
  bool isFinished(){ return (state == Finished);}
  /*--------------------------------------------------------------------------*/
  void    setName(string n ) {
    name = n ;
  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){
    return
      sizeof(TaskHandle) +
      sizeof(int)+
      data_list->size()* (sizeof(DataAccess)+sizeof(int)+sizeof(bool)+sizeof(byte));

  }
  /*--------------------------------------------------------------------------*/
  bool canBeCleared() { return state == CanBeCleared;}
  void upgradeData();
  /*--------------------------------------------------------------------------*/
  bool canRun(){
    list<DataAccess *>::iterator it;
    TRACE_LOCATION;
    if ( state == Finished ) {
      upgradeData();
      state = CanBeCleared;
      return false;
    }
    if ( state == Running ) 
      return false;
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      /*
        printf("**task %s dep :  state:%d\n",	   getName().c_str(),	   state);
	printf("**data:%s \n",  (*it)->data->getName().c_str());
	printf("cur-version: ");(*it)->data->getRunTimeVersion((*it)->type).dump();
	printf("req-version: ");(*it)->required_version.dump();
      */
      
    TRACE_LOCATION;
      if ( (*it)->data->getRunTimeVersion((*it)->type) != (*it)->required_version ) {
	return false;      
      }
      (*it)->data->dump();
    }
    TRACE_LOCATION;
    return true;
  }
  /*--------------------------------------------------------------------------*/
  void setFinished(bool f);
  /*--------------------------------------------------------------------------*/
  void run();
  /*--------------------------------------------------------------------------*/
  int serialize(byte *buffer,int &offset,int max_length);
  /*--------------------------------------------------------------------------*/
  MessageBuffer *serialize();
  /*--------------------------------------------------------------------------*/

  void deserialize(byte *buffer,int &offset,int max_length);
  /*--------------------------------------------------------------------------*/
  void runPropagateTask();
  /*--------------------------------------------------------------------------*/
};
/*======================= ITask ==============================================*/

#endif //__DUCTTEIP_TASK_HPP__
