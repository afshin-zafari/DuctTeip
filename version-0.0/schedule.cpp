#include "schedule.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <pthread.h>

using namespace std;

struct Event {
  string name;
  pthread_t threadid;
  mytimer_t time;
  Event(const char *name_, pthread_t threadid_, mytimer_t time_) 
    : name(name_), threadid(threadid_), time(time_) {}
};

vector<Event> events;
pthread_mutex_t mutex;

extern "C" void SCHEDULE_init() {
  pthread_mutex_init(&mutex, NULL);
}


extern "C" void SCHEDULE_add(const char *name, 
                             pthread_t threadid,
                             mytimer_t time) {

  pthread_mutex_lock(&mutex);
  events.push_back(Event(name, threadid, time));
  pthread_mutex_unlock(&mutex);
}

extern "C" void SCHEDULE_dump(const char *filename) {

  ofstream out(filename);
  pthread_mutex_lock(&mutex);

  for (size_t i = 0; i < events.size(); ++i) {
    out << events[i].name << std::endl;
    out << events[i].threadid << std::endl;
    out << events[i].time.start << std::endl;
    out << events[i].time.total << std::endl;
  }

  out.close();

  pthread_mutex_unlock(&mutex);
  pthread_mutex_destroy(&mutex);
} 

extern "C" void start(mytimer_t *t) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  t->start=tv.tv_sec*1000000+tv.tv_usec;
}

extern "C" void stop(mytimer_t *t) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  t->total = (tv.tv_sec*1000000+tv.tv_usec) - t->start;
}

extern "C" long long getTime(mytimer_t *t) {
  return t->total;
}
