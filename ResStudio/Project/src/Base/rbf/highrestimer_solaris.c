
#include <sys/times.h>

void highrestimer_(long long int *result){
  struct timeval tv;

  gettimeofday(&tv, NULL);
  *result =tv.tv_sec*1000000+tv.tv_usec;

  //*result=gethrtime();
}

