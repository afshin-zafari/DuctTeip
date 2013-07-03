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
} DataAccess;

typedef unsigned long TaskHandle;

class ITask 
{
private:
  string name;
  list<DataAccess *> *data_list;
  int state,sync,type,host;
  TaskHandle handle;
public:
  ITask (string _name,int _host, list<DataAccess *> *dlist):name(_name),host(_host),data_list(dlist){
  }
  void    setHost(int h )    { host = h ;  }
  int     getHost()          { return host;}
  string  getName()          { return name;} 
  void    setName(string n ) { name = n ;  }
  list<DataAccess *> *getDataAccessList() { return data_list;  }
  int getSerializeRequiredSpace(){
    return  
      sizeof(TaskHandle) + 
      sizeof(int)+
      data_list->size()* (sizeof(DataAccess)+sizeof(int));
     
  }
  int serialize(byte *buffer,int &offset,int max_length){
    int count =data_list->size();
    list<DataAccess *>::iterator it;
    copy<TaskHandle>(buffer,offset,handle);
    copy<int>(buffer,offset,count);
    for ( it = data_list->begin(); it != data_list->end(); ++it ) {
      (*it)->data->getDataHandle()->serialize(buffer,offset,max_length);
      int ver = (*it)->required_version;
      copy<int>(buffer,offset,ver);
    }
    
  }
  void deserialize(byte *buffer,int &offset,int max_length);
};

#endif //__TASK_HPP__
