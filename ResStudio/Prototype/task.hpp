#ifndef __TASK_HPP__
#define __TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include "data.hpp"

struct PropagateInfo;
typedef struct {
  IData *data;
  DataVersion required_version;
  byte type;
} DataAccess;

typedef unsigned long TaskHandle;

class ITask
{
private:
  string name;
  list<DataAccess *> *data_list;
  int state,sync,type,host;
  TaskHandle handle;
  unsigned long key,comm_handle;
  IContext *parent_context;
  PropagateInfo *prop_info;
public:
  enum TaskType{
    NormalTask,
    PropagateTask
  };
  enum TaskState{
    WaitForData,
    Running,
    Finished
  };
  ITask (IContext *context,string _name,unsigned long _key,int _host, list<DataAccess *> *dlist):host(_host),data_list(dlist){
    parent_context = context;
    key = _key;
    if (_name.size() ==0  )
      _name.assign("task");
    setName(_name);
    comm_handle = 0 ;    
    state = WaitForData; 
    type = NormalTask;
  }
  ITask():name(""),host(-1){
    state = WaitForData;
    type = NormalTask;
  }
  ITask(PropagateInfo *P);
  void    setHost(int h )    { host = h ;  }
  int     getHost()          { return host;}
  string  getName()          { return name;}
  unsigned long getKey(){ return key;}

  void       setHandle(TaskHandle h)     { handle = h;}
  TaskHandle getHandle()                 {return handle;}

  void          setCommHandle(unsigned long h) { comm_handle = h;   }
  unsigned long getCommHandle()                { return comm_handle;}

  list<DataAccess *> *getDataAccessList() { return data_list;  }

  void dumpDataAccess(list<DataAccess *> *dlist){
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("#daxs:%s @%d \n ", (*it)->data->getName().c_str(),(*it)->data->getHost());
      (*it)->required_version.dump();
      (*it)->data->dump();
    }
    printf ("\n");
  }
  void dump();
  bool isFinished(){ return (state == Finished);}
  void    setName(string n ) {
    name = n ;
    //memcpy(&key,name.c_str(),sizeof(unsigned long));
  }
  int getSerializeRequiredSpace(){
    return
      sizeof(TaskHandle) +
      sizeof(int)+
      data_list->size()* (sizeof(DataAccess)+sizeof(int)+sizeof(bool)+sizeof(byte));

  }
  bool canRun(){
    list<DataAccess *>::iterator it;
    if ( state == Finished ) 
      return false;
    //printf("**task %s dep :  state:%d\n",	   getName().c_str(),	   state);
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      /*
	printf("**data:%s \n",  (*it)->data->getName().c_str());
	printf("cur-version: ");(*it)->data->getRunTimeVersion((*it)->type).dump();
	printf("req-version: ");(*it)->required_version.dump();
      */
      if ( (*it)->data->getRunTimeVersion((*it)->type) != (*it)->required_version ) {
	return false;      
      }
    }
    return true;
  }
  void setFinished(bool f);
  void run();
  int serialize(byte *buffer,int &offset,int max_length);
  void deserialize(byte *buffer,int &offset,int max_length);
  void runPropagateTask();
};

#endif //__TASK_HPP__
