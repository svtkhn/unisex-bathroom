#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;


int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items



int items = 0;
uthread_mutex_t mx;
uthread_cond_t condc,condp;

void* producer (void* v) {

    for (int i=0; i<NUM_ITERATIONS; i++) {
        uthread_mutex_lock(mx);
        while (items == MAX_ITEMS){
            producer_wait_count++;

            uthread_cond_wait(condc);

        }

        items++;

        histogram[items]++;
        uthread_cond_signal(condp);


        uthread_mutex_unlock(mx);
    }

    return NULL;
}

void* consumer (void* v) {

    for (int i=0; i<NUM_ITERATIONS; i++) {
        uthread_mutex_lock(mx);
        while (items == 0) {
            consumer_wait_count++;

            uthread_cond_wait(condp);

        }

        items--;
        histogram[items]++;

        uthread_cond_signal(condc);

        uthread_mutex_unlock(mx);
    }

    return NULL;
}


int main (int argc, char** argv) {
    uthread_t t[4];
    uthread_init (4);


    mx = uthread_mutex_create();
    condc = uthread_cond_create(mx);
    condp = uthread_cond_create(mx);


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
