#ifndef __TASK_HPP__
#define __TASK_HPP__


#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include "data.hpp"

typedef struct {
  IData *data;
  int required_version;
  bool ready;
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
public:
  ITask (string _name,int _host, list<DataAccess *> *dlist):host(_host),data_list(dlist){
    setName(_name);
  }
  ITask():name(""),host(-1){}
  void    setHost(int h )    { host = h ;  }
  int     getHost()          { return host;}
  string  getName()          { return name;} 

  void       setHandle(TaskHandle h)     { handle = h;}
  TaskHandle getHandle()                 {return handle;}

  void          setCommHandle(unsigned long h) { comm_handle = h;   }
  unsigned long getCommHandle()                { return comm_handle;}

  list<DataAccess *> *getDataAccessList() { return data_list;  }

  void dumpDataAccess(list<DataAccess *> *dlist){
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("%s,%d rdy:%d ", (*it)->data->getName().c_str(),(*it)->required_version,(*it)->ready);
    }
    printf ("\n");
  }
  void dump(){
    printf("---------------------------\n");
    printf("task:%s key:%lx ,#of data:%ld ",name.c_str(),key,data_list->size());
    printf("handle:%ld , comm_handle:%ld\n",handle,comm_handle);
    dumpDataAccess(data_list);
    printf("---------------------------\n");
  }
  void    setName(string n ) { 
    name = n ;  
    memcpy(&key,name.c_str(),sizeof(unsigned long));
  }
  int getSerializeRequiredSpace(){
    return  
      sizeof(TaskHandle) + 
      sizeof(int)+
      data_list->size()* (sizeof(DataAccess)+sizeof(int)+sizeof(bool));
     
  }
  int serialize(byte *buffer,int &offset,int max_length){
    int count =data_list->size();
    list<DataAccess *>::iterator it;
    copy<unsigned long>(buffer,offset,key);
    copy<int>(buffer,offset,count);
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      (*it)->data->getDataHandle()->serialize(buffer,offset,max_length);
      int ver = (*it)->required_version;
      copy<int>(buffer,offset,ver);
      bool ready=(*it)->ready;
      copy<bool>(buffer,offset,ready);
    }
    
  }
  void deserialize(byte *buffer,int &offset,int max_length);
};

#endif //__TASK_HPP__
