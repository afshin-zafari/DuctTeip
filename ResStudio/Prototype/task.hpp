#ifndef __DUCTTEIP_TASK_HPP__
#define __DUCTTEIP_TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "data.hpp"
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
    sg_handle = new Handle<Options>;
    pthread_mutexattr_init(&task_finish_ma);
    pthread_mutexattr_settype(&task_finish_ma,PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&task_finish_mx,&task_finish_ma);
    exported = imported = false;

  }
  /*--------------------------------------------------------------------------*/
  ~IDuctteipTask(){
    if ( message_buffer != NULL ) 
      delete message_buffer;
    if ( sg_handle != NULL ) 
      delete sg_handle;
    pthread_mutex_destroy(&task_finish_mx);
    pthread_mutexattr_destroy(&task_finish_ma);
  }  
  /*--------------------------------------------------------------------------*/
  IDuctteipTask():name(""),host(-1){
    state = WaitForData;
    type = NormalTask;
    message_buffer = NULL;
    sg_handle = NULL;
    exported = imported = false;
  }
  /*--------------------------------------------------------------------------*/
  IDuctteipTask(PropagateInfo *P);
  /*--------------------------------------------------------------------------*/
  pthread_mutex_t * getTaskFinishMutex(){
    return &task_finish_mx ;
  }
  /*--------------------------------------------------------------------------*/
  void createSyncHandle(){sg_handle = new Handle<Options>;}
  /*--------------------------------------------------------------------------*/
  Handle<Options> *getSyncHandle(){return sg_handle;}
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
  string  getName()          { return name + '-'+ to_string(handle);}
  unsigned long getKey(){ return key;}

  void       setHandle(TaskHandle h)     { handle = h;}
  TaskHandle getHandle()                 {return handle;}

  void          setCommHandle(unsigned long h) { comm_handle = h;   }
  unsigned long getCommHandle()                { return comm_handle;}

  /*--------------------------------------------------------------------------*/
  list<DataAccess *> *getDataAccessList() { return data_list;  }

  /*--------------------------------------------------------------------------*/
  void dumpDataAccess(list<DataAccess *> *dlist){
    if ( 0 && !DUMP_FLAG)
      return;
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("#daxs:%s @%d \n req-version:", (*it)->data->getName().c_str(),(*it)->data->getHost());
      (*it)->required_version.dump();
      printf("for data \n");
      (*it)->data->dump('N');
    }
    printf ("\n");
  }
  /*--------------------------------------------------------------------------*/
  void dump(char c=' ');
  /*--------------------------------------------------------------------------*/
  bool isFinished(){ return (state == Finished);}
  int getState(){ return state;}
  void setState( int s) { state = s;}
  /*--------------------------------------------------------------------------*/
  void    setName(string n ) {
    name = n ;
  }
  /*--------------------------------------------------------------------------*/
  int getPackSize(){
    return
      sizeof(TaskHandle) +
      sizeof(int)+
      //data_list->size() // todo
      3* (sizeof(DataAccess)+sizeof(int)+sizeof(bool)+sizeof(byte));

  }
  /*--------------------------------------------------------------------------*/
  bool canBeCleared() { return state == CanBeCleared;}
  bool isUpgrading()  { return state == UpgradingData;}
  void upgradeData(char);
  /*--------------------------------------------------------------------------*/
  bool canRun(char c=' '){
    list<DataAccess *>::iterator it;
    bool dbg=  (c !=' ')|| DLB_MODE;
    bool result=true;
    char stats[8]="WRFUC";
    char xi=isExported()?'X':(isImported()?'I':' ');
    if ( isExported() ) {
      return false;
    }
    if ( state == Finished ) {
      return false;
    }
    if ( state >= Running ){
      return false;
    }
    if (0 && dbg)printf("**task %s dep :  state:%d\n",	   getName().c_str(),	   state);
    string s1,s2;
    s1=getName();s2="            ";
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      if(DLB_MODE){
	s1+=" - "+(*it)->data->getName()+" , "+(*it)->required_version.dumpString();
	s2+="             " +(*it)->data->getRunTimeVersion((*it)->type).dumpString();
	/*
	printf("  **data:%s \n",  (*it)->data->getName().c_str());
	printf("  cur-version:%s\n",(*it)->data->getRunTimeVersion((*it)->type).dumpString().c_str());
	printf("  req-version:%s\n",(*it)->required_version.dumpString().c_str());
	*/
      }
      
      
      if ( (*it)->data->getRunTimeVersion((*it)->type) != (*it)->required_version ) {
	result=false;      
      }
    }
    if (DLB_MODE){
      if(dbg)printf("%c(%c)%s %c%c\n%s %ld\n",c,result?'+':'-',s1.c_str(),stats[state-2],xi,s2.c_str(),getTime());
      else   printf("%c[%c]%s %c%c\n%s %ld\n",c,result?'+':'-',s1.c_str(),stats[state-2],xi,s2.c_str(),getTime());
    }
    return result;
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
  void setExported(bool f) { exported=f;}
  /*--------------------------------------------------------------------------*/
  bool isExported(){return exported;}
  /*--------------------------------------------------------------------------*/
  void setImported(bool f) { imported=f;}
  /*--------------------------------------------------------------------------*/
  bool isImported(){return imported;}
  bool isRunning(){return state == Running ;}
  /*--------------------------------------------------------------------------*/
  void runPropagateTask();
  /*--------------------------------------------------------------------------*/
};
/*======================= ITask ==============================================*/

#endif //__DUCTTEIP_TASK_HPP__
