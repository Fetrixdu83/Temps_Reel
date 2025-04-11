#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <errno.h>
#include <sched.h>
#include <assert.h>

#define NUM_TASKS 3
#define HIGH_PRIO 99
#define MID_PRIO 50
#define LOW_PRIO 1

struct timespec period[NUM_TASKS] = {
    {1, 0},    // Task 1: 1s period
    //{2, 0},    // Task 2: 2s period
    //{1, 900000000} // Task 3: 1.5s period
};

int load[NUM_TASKS] = {50000, 25000, 30000};
pthread_barrier_t barrier;
pthread_mutex_t shared_mutex;

void* periodic_task(void* arg) {
    int task_id = *(int*)arg;
    struct timespec next_activation;
    struct timespec start, end;
    double response_time;
    
    clock_gettime(CLOCK_MONOTONIC, &next_activation);
    pthread_barrier_wait(&barrier);

    while(1) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        // Critical section for tasks 0 and 2
        if(task_id == 0 || task_id == 2) {
            pthread_mutex_lock(&shared_mutex);
            printf("Task %d entered critical section\n", task_id);
        }

        // Simulate workload
        volatile int counter = 0;
        for(int i = 0; i < load[task_id]; i++) counter++;

        if(task_id == 0 || task_id == 2) {
            pthread_mutex_unlock(&shared_mutex);
            printf("Task %d left critical section\n", task_id);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        response_time = (end.tv_sec - start.tv_sec) * 1e9 + 
                       (end.tv_nsec - start.tv_nsec);
        
        printf("Task %d response time: %.2f ms\n", 
               task_id, response_time / 1e6);

        // Calculate next activation
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
    pthread_attr_t attr;
    struct sched_param param;
    pthread_mutexattr_t mutex_attr;

    // Initialize barrier
    pthread_barrier_init(&barrier, NULL, NUM_TASKS);

    // Initialize mutex with priority inheritance
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT);
    pthread_mutex_init(&shared_mutex, &mutex_attr);

    // Set main thread to real-time priority
    param.sched_priority = HIGH_PRIO;
    if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &param)) {
        perror("Failed to set main thread priority");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < NUM_TASKS; i++) {
        task_ids[i] = i;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

        // Set priorities
        switch(i) {
            case 0: param.sched_priority = HIGH_PRIO; break;
            case 1: param.sched_priority = MID_PRIO; break;
            case 2: param.sched_priority = LOW_PRIO; break;
        }
        pthread_attr_setschedparam(&attr, &param);

        if(pthread_create(&threads[i], &attr, periodic_task, &task_ids[i])) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
        pthread_attr_destroy(&attr);
    }

    // Wait for threads (never reached in this example)
    for(int i = 0; i < NUM_TASKS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&shared_mutex);
    return 0;
}