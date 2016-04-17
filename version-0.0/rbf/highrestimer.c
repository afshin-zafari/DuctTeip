
#if defined(sun) || defined(__sun)

#include <sys/times.h>

void highrestimer_(long long int *result){
  struct timeval tv;

  gettimeofday(&tv, NULL);
  *result =tv.tv_sec*1000000+tv.tv_usec;

  //*result=gethrtime();
}

#else

#include <sys/time.h>

void highrestimer_(long long int *result) {
  struct timeval tv;
  gettimeofday(&tv, 0);
  *result = tv.tv_sec*1000000+tv.tv_usec;
}

#endif
