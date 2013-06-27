#ifndef __ENGINE_HPP__
#define __ENGINE_HPP__

typedef unsigned char byte;
#include "task.hpp"
class engine
{
private:
  vector<ITask*> task_list;
  byte *prop_buffer;
  int prop_buffer_length;
public:
  TaskHandle  addTask(string task_name, int task_host, list<DataAccess *> *data_access){
    ITask *task = new ITask (task_name,task_host,data_access);
    TaskHandle task_id = task_list.size();    
    task_list.push_back(task);
    //dumpTask(task_id ) ; 
    return task_id;
  }
  void dumpTask(TaskHandle th ) {
    ITask &t = *task_list[(int)th];
    printf ("Task:%s @ %d,",t.getName().c_str(),t.getHost());
    dumpDataAccess(t.getDataAccessList());
  }
  void dumpTasks(){
    for ( int i = 0 ; i < task_list.size() ; i ++ ) {
      ITask &t = *task_list[i];
      dumpTask(TaskHandle(i));
    }
  }
  void dumpDataAccess(list<DataAccess *> *dlist){
    list<DataAccess *>::iterator it;
    for (it = dlist->begin(); it != dlist->end(); it ++) {
      printf("%s,%d ", (*it)->data->getName().c_str(),(*it)->required_version);
    }
    printf ("\n");
  }
  void sendPropagateTask(unsigned char *msg_buffer, int msg_size, int dest_host) {
    prop_buffer = msg_buffer;
    prop_buffer_length = msg_size;
  }
  void addPropagateTask(void *P){
  }
  void receivePropagateTask(byte *buffer, int *length){
    buffer  = prop_buffer ;
    *length = prop_buffer_length ;

  }
};

engine dtEngine;
#endif //__ENGINE_HPP__
