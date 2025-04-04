#include <stdio.h>
//#include <linux/time.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>




int load = 100;
long task1_period = 1000000000; // 1s

void task1();
void task2();

int main()
{
  pthread_t p1_id = 1;
  if ( pthread_create (&p1_id , NULL , &task1 , NULL ) !=0) {
    // If there is an error , print an error and exit
    printf("error\n");
    exit(1);
  }

  pthread_t p2_id = 2;
  if ( pthread_create (&p2_id , NULL , &task2 , NULL ) !=0) {
    // If there is an error , print an error and exit
    printf("error\n");
    exit(1);
  }
  printf ( " Thread created \n " ) ;
  while(1){}
  return 0 ;
}

void task1()
{
  printf("task1\n");
  struct timespec next_activation, task_period;
  set_task_period(&task_period);
  int err = clock_gettime(CLOCK_MONOTONIC, &next_activation);
  printf("task1 : err = %d\n", err);
  struct timespec now;

  // execute a sequence of jobs forever
  while (1)
  {
    // wait until release of next job
    sleep_until_next_activation(&next_activation);
    // call the actual application logic
    process_one_activation();

    err = clock_gettime(CLOCK_MONOTONIC, &now);
    printf("now: \t %ld\n",now.tv_nsec+now.tv_sec*10^9);
    printf("newt: \t %ld\n",next_activation.tv_nsec+next_activation.tv_sec*10^9);
    printf("1 \t %d\n", timespec_diff(&now, &next_activation));

    // compute the next activation time
    timespec_add(&next_activation, &task_period);
  }
}

void task2()
{
  printf("task2\n");
  struct timespec next_activation, task_period;
  set_task_period2(&task_period);
  int err = clock_gettime(CLOCK_MONOTONIC, &next_activation);
  printf("taskPeriod: s %ld\n ns: \t %ld\n",task_period.tv_sec, task_period.tv_nsec);
  printf("next activ: s %ld\n ns: \t %ld\n",next_activation.tv_sec, next_activation.tv_nsec);
  
  printf("task2 : err = %d\n", err);
  assert(err == 0);
  struct timespec now;
  
  // execute a sequence of jobs forever
  while (1)
  {
    // wait until release of next job
    //sleep_until_next_activation(&next_activation);

    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    // call the actual application logic
    process_one_activation();
    
    err = clock_gettime(CLOCK_MONOTONIC, &now);
    printf("now2: \t %ld\n",now.tv_nsec+now.tv_sec*10^9);
    printf("newt2: \t %ld\n",next_activation.tv_nsec+next_activation.tv_sec*10^9);
    printf("2 \t %d\n", timespec_diff(&now, &next_activation));
    // compute the next activation time
    timespec_add(&next_activation, &task_period);
  }
}

void process_one_activation()
{
  for (int i = 0; i < load * 1000; i++)
  {
    //printf("%d\n", i);
  }
}

void sleep_until_next_activation(struct timespec *next_activation)
{
  int err;
  do
  {
    // absolute sleep until next_activation
    err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    // if err is nonzero , we might have woken up
    // too early
  } while (err != 0 && errno == EINTR);
  // assert(err == 0);
}

void timespec_add(struct timespec *next_activation, struct timespec *task_period)
{
  next_activation->tv_sec += task_period->tv_sec;
  next_activation->tv_nsec += task_period->tv_nsec;
  if (next_activation->tv_nsec >= 1000000000)
  {
    next_activation->tv_sec++;
    next_activation->tv_nsec -= 1000000000;
  }
}

int timespec_diff(struct timespec *next_activation, struct timespec *now)
{
  return abs((int)(next_activation->tv_sec*10^9+next_activation->tv_nsec) - (now->tv_sec*10^9+now->tv_nsec));
}

void set_task_period(struct timespec *task_period)
{
  task_period->tv_sec = 1;
  task_period->tv_nsec = 0; // 1000ms
}

void set_task_period2(struct timespec *task_period)
{
  task_period->tv_sec = 2;
  task_period->tv_nsec = 0; // 2000ms
}

int clock_gettime(int clock_id, struct timespec *tp)
{
  tp->tv_sec += clock_id;
}



