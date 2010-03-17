#ifndef TIME_H_FILE
#define TIME_H_FILE

#include <time.h>
#include <sys/time.h>

/*      
  struct timeval {
    long tv_sec;         seconds 
    long tv_usec;   microseconds 
  };
*/

double timediff(time_t starttime, time_t finishtime);

#endif