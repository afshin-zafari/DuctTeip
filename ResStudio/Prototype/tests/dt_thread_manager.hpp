#ifndef __DT_THRD_MANAGER_HPP__
#define __DT_THRD_MANAGER_HPP__
typedef struct {
  pthread_t           thread_id;
  pthread_mutex_t     thread_signals[2];
  pthread_mutexattr_t mutex_attr;
  pthread_attr_t      thread_attr;
  pthread_cond_t      thread_cond;
}ThreadInfo;

typedef int thread_name;
typedef int signal_name;
typedef int section_name;
typedef int subject_name;
class DT_ThreadManager{
private:
  ThreadInfo threads[5];
  enum {
    Admin      = 0,  // Admin and MailBox are cooperating, i.e wait/sync. with each other
    MailBox    = 0,
    MPI        = 1,
    TaskGen    = 2,
    SuperGlue  = 3  
  }ThreadNames;
  enum {
    Starting,
    MailBoxEvent,
    NewEvent    ,
    RequestList
  }ThreadSignals;
  enum {
    TimedLockSuccess =0
  }ReturnCodes;
public:
  /*-----------------------------------------------------------------------------------*/
  DT_ThreadManager();
  ~DT_ThreadManager();
  void initThread(thread_name t, void* (func)(void *), void *arg);
  void signalThread(thread_name tn,signal_name sig);
  void notifyThread(thread_name tn);
  int syncTimed(thread_name tn,signal_name sig);
  int syncWith(thread_name tn,signal_name sig);
  void  enterCriticalSection(thread_name tn,section_name sec);
  void  exitCriticalSection(thread_name tn , section_name sec);
};
DT_ThreadManager dtTMgr;
#endif // __DT_THRD_MANAGER_HPP__
