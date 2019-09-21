#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "uthread.h"
#include "uthread_mutex_cond.h"

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

#define MAX_OCCUPANCY      3
#define NUM_ITERATIONS     100
#define NUM_PEOPLE         20
#define FAIR_WAITING_COUNT 4

/**
 * You might find these declarations useful.
 */
enum GenderIdentity {MALE = 0, FEMALE = 1};
const static enum GenderIdentity otherGender [] = {FEMALE, MALE};

struct Washroom {
    unsigned int    man_num;
    unsigned int    wom_num;
    uthread_mutex_t mutex;
    uthread_cond_t  not_full;
} ;


struct Washroom* createWashroom() {
    struct Washroom* washroom = malloc (sizeof (struct Washroom));
    washroom->man_num = 0;
    washroom->wom_num = 0;
    washroom->mutex = uthread_mutex_create();
    washroom->not_full  = uthread_cond_create (washroom->mutex);
    return washroom;
};

struct Washroom* washroom;

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_PEOPLE)
int             entryTicker;                                          // incremented with each entry
int             waitingHistogram         [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogrammutex;
int             occupancyHistogram       [2] [MAX_OCCUPANCY + 1];




void enterWashroom (enum GenderIdentity g) {

    entryTicker++;
    if (g == MALE) {
        occupancyHistogram[MALE][washroom->man_num]++;
    } else {
        occupancyHistogram[FEMALE][washroom->wom_num]++;

    }
    assert(((washroom->man_num == 0 && washroom->wom_num < 4) || (washroom->wom_num == 0 && washroom->man_num < 4)) && (washroom->wom_num > 0 || washroom->man_num > 0));
    assert(washroom->man_num*washroom->wom_num == 0);


    for (int i = 0; i < NUM_PEOPLE*(rand()%4+1); i++) { uthread_yield(); }
}

void leaveWashroom() {
    for (int i = 0; i < NUM_PEOPLE; i++) { uthread_cond_signal(washroom->not_full); }
    uthread_mutex_unlock(washroom->mutex);
    for (int i = 0; i < NUM_PEOPLE; i++) { uthread_yield(); }
    uthread_mutex_lock(washroom->mutex);
}

void recordWaitingTime (int waitingTime) {
    uthread_mutex_lock (waitingHistogrammutex);
    if (waitingTime < WAITING_HISTOGRAM_SIZE)
        waitingHistogram [waitingTime] ++;
    else
        waitingHistogramOverflow ++;
    uthread_mutex_unlock (waitingHistogrammutex);
}




void* male(int number) {

    for (int i = 0; i < NUM_ITERATIONS; i++) {

        //do something
        for (int i = 0; i < NUM_PEOPLE; i++) { uthread_yield(); }



        //go to washroom
        uthread_mutex_lock(washroom->mutex);
        int waiting = 0;

        while (washroom->wom_num > 0 || washroom->man_num == MAX_OCCUPANCY) {
            waiting++;
            uthread_cond_wait(washroom->not_full);
            printf("male %d trying\n", number);

        }
        recordWaitingTime(waiting);


        washroom->man_num++;

        printf("male %d entering\n", number);

        uthread_mutex_lock(washroom->mutex);
        enterWashroom(MALE);
        uthread_mutex_unlock(washroom->mutex);



        washroom->man_num--;
        printf("male %d leaving\n", number);

        leaveWashroom();

        uthread_mutex_unlock(washroom->mutex);

    }
    return NULL;
}


void* female(int number) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {

        //do something
        for (int i = 0; i < NUM_PEOPLE; i++) { uthread_yield(); }



        //go to washroom
        uthread_mutex_lock(washroom->mutex);
        int waiting = 0;

        while (washroom->man_num > 0 || washroom->wom_num == MAX_OCCUPANCY) {
            waiting++;
            uthread_cond_wait(washroom->not_full);
            printf("female %d trying\n", number);

        }
        recordWaitingTime(waiting);


        washroom->wom_num++;
        printf("female %d entering\n", number);




        uthread_mutex_unlock(washroom->mutex);
        enterWashroom(FEMALE);
        uthread_mutex_lock(washroom->mutex);




        washroom->wom_num--;
        printf("female %d leaving\n", number);

        leaveWashroom();

        uthread_mutex_unlock(washroom->mutex);
    }
    return NULL;
}



int main (int argc, char** argv) {
    srand (time(NULL));
    uthread_init (1);
    washroom = createWashroom();
    uthread_t pt [NUM_PEOPLE];
    waitingHistogrammutex = uthread_mutex_create ();
    washroom = createWashroom();
    int no_male = 0, no_female = 0;

    for (int i = 0; i < NUM_PEOPLE; i++) {
        if (rand()%2 == 0) {
            pt[i] = uthread_create(male, ++no_male);
        } else {
            pt[i] = uthread_create(female, ++no_female);

        }
    }
    for (int i = 0; i < NUM_PEOPLE; i++) {
        if (pt[i]) (void) uthread_join(pt[i], NULL);
    }

    printf ("Times with 1 person who identifies as male   %d\n", occupancyHistogram [MALE]   [1]);
    printf ("Times with 2 people who identifies as male   %d\n", occupancyHistogram [MALE]   [2]);
    printf ("Times with 3 people who identifies as male   %d\n", occupancyHistogram [MALE]   [3]);
    printf ("Times with 1 person who identifies as female %d\n", occupancyHistogram [FEMALE] [1]);
    printf ("Times with 2 people who identifies as female %d\n", occupancyHistogram [FEMALE] [2]);
    printf ("Times with 3 people who identifies as female %d\n", occupancyHistogram [FEMALE] [3]);
    printf ("Waiting Histogram\n");
    for (int i=0; i<WAITING_HISTOGRAM_SIZE; i++)
        if (waitingHistogram [i])
            printf ("  Number of times people waited for %d %s to enter: %d\n", i, i==1?"person":"people", waitingHistogram [i]);
    if (waitingHistogramOverflow)
        printf ("  Number of times people waited more than %d entries: %d\n", WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}
