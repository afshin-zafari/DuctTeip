extern "C" {
#include "tl.h"
}

#include "accesstypes/readwriteadd.hpp"
#include "threads/threadmanager.hpp"
#include "threads/task.hpp"
#include "threads/workerthread.hpp"
#include "handle/handle.hpp"

#define CREATE_SCHEDULE
#include "schedule.h"

#include <string>

struct HandleParent {};

struct Options {
  enum { PassThreadId = 0 };    // Pass thread ID to task when run
  enum { CurrentTask = 1 };     // Remember currently executed task
  enum { ThreadWorkspace = 0 }; // Each thread keeps workspace for tasks
  enum { TaskId = 1 };
  enum { HandleId = 1 };
  enum { Lockable = 1 };
  enum { Renaming = Lockable && 1 };
  enum { Renaming_RunAllRenamed = Renaming && 0 };
  enum { DAG = CurrentTask && 0 };

  typedef ReadWriteAdd AccessInfoType;
  typedef Access<Options> AccessType;
  typedef Handle<Options> IHandleType;
  typedef Handle<Options> HandleType;
  typedef HandleParent HandleParentType;
  typedef ITask<Options> TaskType;
  typedef WorkerThread<Options> WorkerType;
  typedef ThreadManager<Options> ThreadManagerType;
};

typedef Options::TaskType ITask_;
typedef Options::IHandleType IHandle_;
typedef Options::ThreadManagerType ThreadManager_;
typedef Options::AccessInfoType AccessInfo_;

ThreadManager_ *tm;
int mynode_id,num_handles=0;

template size_t ITask_GlobalId<Options, true>::global_task_id;
template size_t Handle_GlobalId<Options, true>::global_handle_id;

class Task : public ITask_ {
private:
  tl_task_function function;
  char *args;
  std::string name;

public:
  Task(std::string name_,
       tl_task_function function_,
       void *args_, int argsize,
       IHandle_ **read_handles, int num_reads,
       IHandle_ **write_handles, int num_writes,
       IHandle_ **add_handles, int num_adds)
    : function(function_), name(name_)
  {
    args = new char[argsize];
    memcpy(args, args_, argsize);
    //printf("CPP SIZEOF CHOL_ARG%d\n",argsize);
    num_handles += num_reads + num_writes + num_adds;
    for (int i = 0; i < num_reads; ++i)
      registerAccess(ReadWriteAdd::read, read_handles[i]);
    for (int i = 0; i < num_writes; ++i)
      registerAccess(ReadWriteAdd::write, write_handles[i]);
    for (int i = 0; i < num_adds; ++i)
      registerAccess(ReadWriteAdd::add, add_handles[i]);
 }
  void run() {
    SCHEDULE_startTask();
    /*
    char s[256];
    sprintf(s,"SuperGlue %4.4s\nSuperGlue ",args);
    for(int i=0;i<getNumAccess();i++)
      sprintf(s+ strlen(s),"%ld %d ",getAccess(i).getHandle(),getAccess(i).getRequiredVersion());
    printf ("%s\n",s);
    */
    function(args);
    if (name.empty()) {
      SCHEDULE_register2("task%d",global_task_id);
    }
    else {
      SCHEDULE_register(name.c_str());
    }
  }
  ~Task() {
    delete[] args;
  }
  std::string get_name(){return name;}
};

void tl_init(int *num_threads,int *node_id) {
  ITask_GlobalId<Options, true>::global_task_id = 0;
  Handle_GlobalId<Options, true>::global_handle_id = 0;
  tm = new ThreadManager_(*num_threads);
  mynode_id = *node_id;
  SCHEDULE_init();
}

void tl_destroy(char *dir_name) {
  delete tm;
  char fname[200];
  sprintf(fname,"%s/schedule%d.dat",dir_name,mynode_id);
  SCHEDULE_dump(fname);
}

void tl_barrier() {
  tm->barrier();
}

namespace {
  static void tl_unique(tl_handle_t *start, int *num) {
    tl_handle_t *end = start + *num;
    std::sort(start, end);
    *num = std::unique(start, end) - start;
  }
}

void tl_add_task_unsafe(tl_task_function function, 
                        void *args, int *argsize,
                        tl_handle_t **read_handles, int *num_reads,
                        tl_handle_t **write_handles, int *num_writes,
                        tl_handle_t **add_handles, int *num_adds) {
  Task *t = new Task("", 
                     *function, args, *argsize,
                     (IHandle_ **) read_handles, *num_reads,
                     (IHandle_ **) write_handles, *num_writes,
                     (IHandle_ **) add_handles, *num_adds);
  tm->addTask(t);
}

void tl_add_task_safe(tl_task_function function, 
                      void *args, int *argsize,
                      tl_handle_t **read_handles, int *num_reads,
                      tl_handle_t **write_handles, int *num_writes,
                      tl_handle_t **add_handles, int *num_adds) {
  if (*num_reads > 0)
    tl_unique(*read_handles, num_reads);
  
  if (*num_writes > 0)
    tl_unique(*write_handles, num_writes);
  
  if (*num_adds > 0)
    tl_unique(*add_handles, num_adds);
  
  Task *t = new Task("",
                     *function, args, *argsize,
                     (IHandle_ **) read_handles, *num_reads,
                     (IHandle_ **) write_handles, *num_writes,
                     (IHandle_ **) add_handles, *num_adds);
  tm->addTask(t);
}

void tl_add_task_named(const char *name,
                       tl_task_function function, 
                       void *args, int *argsize,
                       tl_handle_t **read_handles, int *num_reads,
                       tl_handle_t **write_handles, int *num_writes,
                       tl_handle_t **add_handles, int *num_adds,
                       int len) {
  
  std::string  ss(name, len);
  SCHEDULE_startTask();
  Task *t = new Task(ss, 
                     *function, args, *argsize,
                     (IHandle_ **) read_handles, *num_reads,
                     (IHandle_ **) write_handles, *num_writes,
                     (IHandle_ **) add_handles, *num_adds);
  tm->addTask(t);
  SCHEDULE_register(name);
}


void tl_create_handles(int *num, tl_handle_t **handles) {
  for (int i = 0; i < *num; ++i)
    (*handles)[i] = new Handle<Options>();
}

void tl_create_handle(tl_handle_t *handle) {
  // printf("create before\n");
  // printf("\thandles=%p\n", handle);
  // printf("\t*handles=%p\n", *handle);
  (*handle) = new Handle<Options>();
  // printf("create after\n");
  // printf("\thandles=%p\n", handle);
  // printf("\t*handles=%p\n", *handle);
}

// void tl_destroy_handles_(tl_handle_t **handles, int *num) {
// printf("tl_destroy_handles(%p, %d)\n", *handles, *num);
// for (int i = 0; i < *num; ++i)
// delete (Handle<Options> *) (*handles)[i];
// }

void tl_destroy_handles(tl_handle_t **handles, int *num) {
  // printf("tl_destroy_handles(%p, %d)\n", *handles, *num);
  for (int i = 0; i < *num; ++i)
    delete (Handle<Options> *) (*handles)[i];
}

void tl_destroy_handle(tl_handle_t *handle) {
  // printf("destroy before\n");
  // printf("\thandles=%p\n", handle);
  // printf("\t*handles=%p\n", *handle);
  delete (Handle<Options> *) *handle;
  // printf("destroy after\n");
  // printf("\thandles=%p\n", handle);
  // printf("\t*handles=%p\n", *handle);
}

void echo0_(int pointer_to_fortran_arg) {
  printf("Pointer\t=\t%p\n", pointer_to_fortran_arg);
  printf("Integer\t=\t%d\n", pointer_to_fortran_arg);
}

void echo1_(int *pointer_to_fortran_arg) {
  printf("Pointer\t=\t%p\n", *pointer_to_fortran_arg);
  printf("Integer\t=\t%d\n", *pointer_to_fortran_arg);
}

void echo2_(int **pointer_to_fortran_arg) {
  printf("Pointer\t=\t%p\n", **pointer_to_fortran_arg);
  printf("Integer\t=\t%d\n", **pointer_to_fortran_arg);
}
