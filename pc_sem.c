#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_sem.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"
#include <fcntl.h>
#include <time.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;



int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items



int items = 0;
uthread_sem_t condc,condp;

void* producer (void* v) {

    for (int i=0; i<NUM_ITERATIONS; i++) {
        while (items == MAX_ITEMS){
            producer_wait_count++;
            printf("want to produce\n");
            uthread_sem_wait(condc);

        }

        items++;

        histogram[items]++;
        uthread_sem_signal(condp);
        printf("produced\n");



    }

    return NULL;
}

void* consumer (void* v) {

    for (int i=0; i<NUM_ITERATIONS; i++) {
        while (items == 0) {
            consumer_wait_count++;
            printf("want to consume\n");

            uthread_sem_wait(condp);

        }

        items--;
        histogram[items]++;

        uthread_sem_signal(condc);
        printf("consumed\n");

    }

    return NULL;
}


int main (int argc, char** argv) {
    uthread_t t[4];
    uthread_init (4);


    condc = uthread_sem_create(0);
    condp = uthread_sem_create(0);

    for (int i = 0; i<(NUM_PRODUCERS+NUM_CONSUMERS); i++){
        if (i<NUM_PRODUCERS) {
            t[i] = uthread_create(producer, 0);
        } else {
            t[i] = uthread_create(consumer, 0);
        }
    }

    for (int i = 0; i<(NUM_PRODUCERS+NUM_CONSUMERS); i++){
        uthread_join(t[i], 0);
    }




    printf ("producer_wait_count=%d, consumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
    printf ("items value histogram:\n");
    int sum=0;
    for (int i = 0; i <= MAX_ITEMS; i++) {
        printf ("  items=%d, %d times\n", i, histogram [i]);
        sum += histogram [i];
    }
    assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);

}
