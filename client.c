/*
 * Description:  A simple client program to connect to the TCP/IP server thanks to Darren Smith
 */

/*
 * Linux:   compile with gcc -Wall client.c -o client
 */ 
 
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

#include <time.h>
#include <assert.h>
#include <errno.h>

#define SOCKET_PORT 10020
#define SOCKET_SERVER "127.0.0.1" /* local host */
#define NUM_TASKS 2
#define HIGH_PRIO 99
#define MID_PRIO 50
#define LOW_PRIO 1

int fd; 
double left_sensor , right_sensor ;
char buffer[256];
pthread_mutex_t mp =  PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

struct timespec period[NUM_TASKS] = {
  {0, 250000000},      // Task 0: 1s period
  {0, 100000000},       // Task 1: 2s period
};

float direction = 0;

void read_sensors(){
    printf("read\n");

    struct timespec next_activation;
    struct timespec start, end;
    double response_time;
    
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    pthread_barrier_wait(&barrier);

    while(1) {
      clock_gettime(CLOCK_MONOTONIC, &start);
        
         
        

      pthread_mutex_lock(&mp);
      send ( fd , "S\n " , strlen ( "S\n" ) ,0) ;
      int n = recv ( fd , buffer , 256 , 0) ;
      buffer [ n ] = '\0';
      printf ( "read :  %s\n " , buffer ) ;
      sscanf ( buffer , "S,%lf,%lf " , & left_sensor , & right_sensor ) ;
      pthread_mutex_unlock(&mp);

        
      clock_gettime(CLOCK_MONOTONIC, &end);
      response_time = (end.tv_sec - start.tv_sec) * 1e9 + 
                     (end.tv_nsec - start.tv_nsec);
      
      printf("read response time: %.2f ms\n", 
             response_time / 1e6);

      next_activation.tv_sec += period[0].tv_sec;
      next_activation.tv_nsec += period[0].tv_nsec;
      if(next_activation.tv_nsec >= 1e9) {
          next_activation.tv_sec++;
          next_activation.tv_nsec -= 1e9;
      }
      
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }   

}

void move_robot(){



  printf("move\n");

    struct timespec next_activation;
    struct timespec start, end;
    double response_time;
    
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    pthread_barrier_wait(&barrier);

    while(1) {
      clock_gettime(CLOCK_MONOTONIC, &start);
        
         
        

      pthread_mutex_lock(&mp);
      if((left_sensor!=0 && right_sensor !=0)){
        if (direction){
          send ( fd , "T,0\n" , strlen ( "T,0\n" ) ,0) ;
          direction = 0;
        }else{
          send ( fd , "T,3.14\n" , strlen ( "T,3.14\n" ) ,0) ;
          direction = 3.14;
        }
        sleep(2);
      }
      else{
        send(fd,"M,99,99\n",strlen("M,99,99\n"),0);
      }
      printf("sent\n");
      pthread_mutex_unlock(&mp);

        
      clock_gettime(CLOCK_MONOTONIC, &end);
      response_time = (end.tv_sec - start.tv_sec) * 1e9 + 
                     (end.tv_nsec - start.tv_nsec);
      
      printf("move response time: %.2f ms\n", 
             response_time / 1e6);

      next_activation.tv_sec += period[1].tv_sec;
      next_activation.tv_nsec += period[1].tv_nsec;
      if(next_activation.tv_nsec >= 1e9) {
          next_activation.tv_sec++;
          next_activation.tv_nsec -= 1e9;
      }
      
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }   



  

}



int main(int argc, char *argv[]) {
  struct sockaddr_in address;
  const struct hostent *server;
  int rc;
  

  /* create the socket */
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    printf("cannot create socket\n");
    return -1;
  }

  /* fill in the socket address */
  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_port = htons(SOCKET_PORT);
  server = gethostbyname(SOCKET_SERVER);

  if (server)
    memcpy((char *)&address.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
  else {
    printf("cannot resolve server name: %s\n", SOCKET_SERVER);
    close(fd);
    return -1;
  }

  /* connect to the server */
  rc = connect(fd, (struct sockaddr *)&address, sizeof(struct sockaddr));
  if (rc == -1) {
    printf("cannot connect to the server\n");
    close(fd);
    return -1;
  }

  fflush(stdout);

  //send(fd,"M,0,0\n",strlen("M,99,99\n"),0);
  //send ( fd , "T,3.14\n" , strlen ( "T,3.14\n" ) ,0) ;


  
  pthread_t threads[NUM_TASKS];
  int task_ids[NUM_TASKS];
  pthread_attr_t attr[NUM_TASKS];
  struct sched_param param[NUM_TASKS+1];


  int mutex = pthread_mutex_init(&mp, NULL);
    
  pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

  // Initialize mutex
  pthread_mutex_init(&mp, &attr[NUM_TASKS]);
  
  // Initialize barrier
  pthread_barrier_init(&barrier, NULL, NUM_TASKS);

  // Set main thread to real-time priority
  param[NUM_TASKS].sched_priority = LOW_PRIO;
  if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param[NUM_TASKS])) {
      perror("Failed to set main thread priority");
      exit(EXIT_FAILURE);
  }

  for(int i = 0; i < NUM_TASKS; i++) {
      task_ids[i] = i;
      pthread_attr_init(&attr[i]);
      pthread_attr_setinheritsched(&attr[i], PTHREAD_EXPLICIT_SCHED);
      
      // Set CPU affinity to processor 0
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(0, &cpuset);  // Set to CPU 0
      pthread_attr_setaffinity_np(&attr[i], sizeof(cpu_set_t), &cpuset);
      
      // Set priorities
      switch(i) {
          case 1: param[i].sched_priority = HIGH_PRIO; break;
          case 0: param[i].sched_priority = MID_PRIO; break;
      }
      pthread_attr_setschedpolicy(&attr[i], SCHED_RR);
      pthread_attr_setschedparam(&attr[i], &param[i]);
      switch(i) {
        case 1: 
          if(pthread_create(&threads[i], &attr[i], read_sensors, &task_ids[i])) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
          } 
          printf("creation read\n");
          break;
        case 0: 
          if(pthread_create(&threads[i], &attr[i], move_robot, &task_ids[i])) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
          } 
          printf("creation move\n");
          break;
    }
      
      pthread_attr_destroy(&attr[i]);
  }

  // Wait for threads
  for(int i = 0; i < NUM_TASKS; i++) {
      pthread_join(threads[i], NULL);
  }





  for (;;) {
    int n = recv(fd, buffer, 256, 0);
    buffer[n] = '\0';
    printf("Received: %s", buffer);

    
  }

  close(fd);

  return 0;
}
