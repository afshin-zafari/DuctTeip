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
  DT_ThreadManager(){}
  /*-----------------------------------------------------------------------------------*/
  ~DT_ThreadManager(){}
  /*----------------------------------------------------------------------------------- */
  void initThread(thread_name t, void* (func)(void *), void *arg){
    pthread_mutexattr_init   (&threads[t].mutex_attr);
    pthread_mutexattr_settype(&threads[t].mutex_attr,
			      PTHREAD_MUTEX_RECURSIVE);

    pthread_attr_init          (&threads[t].thread_attr);
    pthread_attr_setdetachstate(&threads[t].thread_attr,
				PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init (&threads[t].thread_sgnals[Starting]);
    pthread_cond_init  (&threads[t].thread_cond);
    pthread_mutex_lock (&threads[t].thread_sgnals[Starting]);

    pthread_create(&threads[t].thread_id,
		   &threads[t].thread_attr,
		   func,arg);

    pthread_cond_wait(&threads[t].thread_cond,&threads[t].thread_sgnals[Starting]);
  }
  /*-----------------------------------------------------------------------------------*/
  void signalThread(thread_name tn,signal_name sig){
    pthread_mutex_unlock(&threads[tn].thread_signals[sig]);
  }
  /*-----------------------------------------------------------------------------------*/
  void notifyThread(thread_name tn){
    pthread_cond_signal(&threads[tn].thread_cond);
  }
  /*-----------------------------------------------------------------------------------*/
  int syncTimed(thread_name tn,signal_name sig){
    timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);    
    ts.tv_nsec += 10000;    
    return pthread_mutex_timedlock(&threads[tn].thread_signals[sig],&ts);
  }
  /*-----------------------------------------------------------------------------------*/
  int syncWith(thread_name tn,signal_name sig){
    return pthread_mutex_lock(&threads[tn].thread_signals[sig]);
  }
  /*-----------------------------------------------------------------------------------*/
  void  enterCriticalSection(thread_name tn,section_name sec){
    pthread_mutex_lock(&threads[tn].thread_signals[sec]);
  }
  /*-----------------------------------------------------------------------------------*/
  void  exitCriticalSection(thread_name tn , section_name sec){
    pthread_mutex_unlock(&threads[tn].thread_signals[sec]);
  }
  /*-----------------------------------------------------------------------------------*/
};
DT_ThreadManager dtTMgr;
#endif // __DT_THRD_MANAGER_HPP__
