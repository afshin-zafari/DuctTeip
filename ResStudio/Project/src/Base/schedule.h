#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#ifdef CREATE_SCHEDULE

#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>

#define SCHEDULE_startTask() mytimer_t SCHEDULE_timer; start(&SCHEDULE_timer)
#define SCHEDULE_register(name) stop(&SCHEDULE_timer); SCHEDULE_add(name, pthread_self(), SCHEDULE_timer)
#define SCHEDULE_register2(a, b) stop(&SCHEDULE_timer); { char tmpname[400]; sprintf(tmpname, a, b); SCHEDULE_add(tmpname, pthread_self(), SCHEDULE_timer); }
#define SCHEDULE_register3(a, b, c) stop(&SCHEDULE_timer); { char tmpname[400]; sprintf(tmpname, a, b, c); SCHEDULE_add(tmpname, pthread_self(), SCHEDULE_timer); }

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  long long start;
  long long total;
} mytimer_t;

extern void start(mytimer_t *t);
extern void stop(mytimer_t *t);

extern void SCHEDULE_init();
extern void SCHEDULE_add(const char *name,
                         pthread_t threadid,
                         mytimer_t time);

extern void SCHEDULE_dump(const char *filename);

#ifdef __cplusplus
}
#endif

#else
#define SCHEDULE_startTask()
#define SCHEDULE_register(name)
#define SCHEDULE_register2(a, b)
#define SCHEDULE_register3(a, b, c)
#define SCHEDULE_init()
#define SCHEDULE_dump(a)
#endif

#endif // __SCHEDULE_H__
