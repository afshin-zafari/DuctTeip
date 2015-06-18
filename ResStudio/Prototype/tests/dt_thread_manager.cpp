#include "dt_thread_manager.hpp"

DT_ThreadManager::DT_ThreadManager(){}
/*-----------------------------------------------------------------------------------*/
DT_ThreadManager::~DT_ThreadManager(){}
/*----------------------------------------------------------------------------------- */
void DT_ThreadManager::initThread(thread_name t, void* (func)(void *), void *arg){
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
void DT_ThreadManager::signalThread(thread_name tn,signal_name sig){
  pthread_mutex_unlock(&threads[tn].thread_signals[sig]);
}
/*-----------------------------------------------------------------------------------*/
void DT_ThreadManager::notifyThread(thread_name tn){
  pthread_cond_signal(&threads[tn].thread_cond);
}
/*-----------------------------------------------------------------------------------*/
int DT_ThreadManager::syncTimed(thread_name tn,signal_name sig){
  timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);    
  ts.tv_nsec += 10000;    
  return pthread_mutex_timedlock(&threads[tn].thread_signals[sig],&ts);
}
/*-----------------------------------------------------------------------------------*/
int DT_ThreadManager::syncWith(thread_name tn,signal_name sig){
  return pthread_mutex_lock(&threads[tn].thread_signals[sig]);
}
/*-----------------------------------------------------------------------------------*/
void  DT_ThreadManager::enterCriticalSection(thread_name tn,section_name sec){
  pthread_mutex_lock(&threads[tn].thread_signals[sec]);
}
/*-----------------------------------------------------------------------------------*/
void  DT_ThreadManager::exitCriticalSection(thread_name tn , section_name sec){
  pthread_mutex_unlock(&threads[tn].thread_signals[sec]);
}
