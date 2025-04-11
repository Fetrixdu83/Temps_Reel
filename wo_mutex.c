#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <assert.h>

#define NUM_TASKS 3
#define HIGH_PRIO 99
#define MID_PRIO 50
#define LOW_PRIO 1

pthread_mutex_t mp =  PTHREAD_MUTEX_INITIALIZER;
int counter_mutex = 0;

struct timespec period[NUM_TASKS] = {
    {1, 0},      // Task 0: 1s period
    {2, 0},       // Task 1: 2s period
    {3, 0}       // Task 2: 3s period
};

int load[NUM_TASKS] = {1000000000, 1000000000, 1000000000};
pthread_barrier_t barrier;

void* periodic_task(void* arg) {
    int task_id = *(int*)arg;
    struct timespec next_activation;
    struct timespec start, end;
    double response_time;
    
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    pthread_barrier_wait(&barrier);

    while(1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Simulate workload
         
        
        if (task_id!=1){
            pthread_mutex_lock(&mp);
            counter_mutex = 0;
            for(int i = 0; i < load[task_id]; i++) {
                counter_mutex++; 
            }

            pthread_mutex_unlock(&mp);
        }
        else{
            int counter = 0;  
            for(int i = 0; i < load[task_id]; i++) {
                counter++;
            }
        }
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        response_time = (end.tv_sec - start.tv_sec) * 1e9 + 
                       (end.tv_nsec - start.tv_nsec);
        
        printf("Task %d response time: %.2f ms\n", 
               task_id, response_time / 1e6);

        // Calculate next activation time
        next_activation.tv_sec += period[task_id].tv_sec;
        next_activation.tv_nsec += period[task_id].tv_nsec;
        if(next_activation.tv_nsec >= 1e9) {
            next_activation.tv_sec++;
            next_activation.tv_nsec -= 1e9;
        }
        
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }   
    return NULL;
}

int main() {
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
    param[NUM_TASKS].sched_priority = HIGH_PRIO;
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
            case 2: param[i].sched_priority = HIGH_PRIO; break;
            case 1: param[i].sched_priority = MID_PRIO; break;
            case 0: param[i].sched_priority = LOW_PRIO; break;
        }
        pthread_attr_setschedpolicy(&attr[i], SCHED_FIFO);
        pthread_attr_setschedparam(&attr[i], &param[i]);
        if(pthread_create(&threads[i], &attr[i], periodic_task, &task_ids[i])) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        pthread_attr_destroy(&attr[i]);
    }

    // Wait for threads
    for(int i = 0; i < NUM_TASKS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    return 0;
}