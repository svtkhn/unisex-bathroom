#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif
struct Agent {
    uthread_mutex_t mutex;
    uthread_cond_t  match;
    uthread_cond_t  paper;
    uthread_cond_t  tobacco;
    uthread_cond_t  smoke;
};

struct Agent* createAgent() {
    struct Agent* agent = malloc (sizeof (struct Agent));
    agent->mutex   = uthread_mutex_create();
    agent->paper   = uthread_cond_create (agent->mutex);
    agent->match   = uthread_cond_create (agent->mutex);
    agent->tobacco = uthread_cond_create (agent->mutex);
    agent->smoke   = uthread_cond_create (agent->mutex);
    return agent;
}

int ingredients = 0;
uthread_cond_t tobaccer_ready;
uthread_cond_t paperer_ready;
uthread_cond_t matcher_ready;

//
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked

/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */
void* agent (void* av) {

    struct Agent* a = av;
    static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
    static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};

    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {

        ingredients = 0;

        int r = random() % 3;
        signal_count [matching_smoker [r]] ++;
        int c = choices [r];
        if (c & MATCH) {
            VERBOSE_PRINT ("match available\n");
            uthread_cond_signal (a->match);
        }
        if (c & PAPER) {
            VERBOSE_PRINT ("paper available\n");
            uthread_cond_signal (a->paper);
        }
        if (c & TOBACCO) {
            VERBOSE_PRINT ("tobacco available\n");
            uthread_cond_signal (a->tobacco);
        }
        VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
        uthread_cond_wait (a->smoke);
    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}




void* have_match (void* av) {

    struct Agent* a = av;
    static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {

        uthread_cond_wait (a->match);

        ingredients = ingredients|MATCH;
        if (ingredients == choices[0]){
            uthread_cond_signal(tobaccer_ready);
        }else if (ingredients == choices[1]){
            uthread_cond_signal(paperer_ready);
        }else if (ingredients == choices[2]){
            uthread_cond_signal(matcher_ready);
        }

    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}



void* have_paper (void* av) {
    struct Agent* a = av;
    static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uthread_cond_wait (a->paper);
        ingredients = ingredients|PAPER;
        if (ingredients == choices[0]){
            uthread_cond_signal(tobaccer_ready);
        }else if (ingredients == choices[1]){
            uthread_cond_signal(paperer_ready);
        }else if (ingredients == choices[2]){
            uthread_cond_signal(matcher_ready);
        }
    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}



void* have_tobacco (void* av) {
    struct Agent* a = av;
    static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uthread_cond_wait (a->tobacco);
        ingredients = ingredients|TOBACCO;
        if (ingredients == choices[0]){
            uthread_cond_signal(tobaccer_ready);
        }else if (ingredients == choices[1]){
            uthread_cond_signal(paperer_ready);
        }else if (ingredients == choices[2]){
            uthread_cond_signal(matcher_ready);
        }

    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}




void* tobaccer (void* av) {
    struct Agent* a = av;


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uthread_cond_wait(tobaccer_ready);
        smoke_count[TOBACCO]++;
        uthread_cond_signal (a->smoke);
    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}


void* matcher (void* av) {
    struct Agent* a = av;


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uthread_cond_wait(matcher_ready);
        smoke_count[MATCH]++;
        uthread_cond_signal (a->smoke);

    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}


void* paperer (void* av) {
    struct Agent* a = av;


    uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uthread_cond_wait(paperer_ready);
        smoke_count[PAPER]++;
        uthread_cond_signal (a->smoke);

    }
    uthread_mutex_unlock (a->mutex);
    return NULL;
}






int main (int argc, char** argv) {
    uthread_init (7);
    struct Agent*  a = createAgent();

    tobaccer_ready = uthread_cond_create (a->mutex);
    paperer_ready = uthread_cond_create (a->mutex);
    matcher_ready = uthread_cond_create (a->mutex);


    uthread_create (tobaccer, a);
    uthread_create (matcher, a);
    uthread_create (paperer, a);
    uthread_create (have_match, a);
    uthread_create (have_paper, a);
    uthread_create (have_tobacco, a);


    uthread_join (uthread_create (agent, a), 0);



    assert (signal_count [MATCH]   == smoke_count [MATCH]);
    assert (signal_count [PAPER]   == smoke_count [PAPER]);
    assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
    assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
    printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
            smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}