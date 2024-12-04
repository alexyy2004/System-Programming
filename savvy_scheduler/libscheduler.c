/**
 * savvy_scheduler
 * CS 341 - Fall 2024
 * [Group Working]
 * Group Member Netids: pjame2, boyangl3, yueyan2,
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

static double totalTurnAroundTime = 0;
static double totalResponseTime = 0;
static double totalWaitTime = 0;
static int totalJobs = 0;

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;
    double arrivalTime;
    double ResponseStartTime;
    double runningTimeOld;
    double runningTime;
    int pri;
    bool started;
    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
} job_info;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    job_info * mdata1 = ((job *) a)->metadata;
    job_info * mdata2 = ((job *) b)->metadata;
    
    if (mdata1->arrivalTime == mdata2->arrivalTime) {
        return -1; // not sure about this 
    }
    return mdata1->arrivalTime < mdata2->arrivalTime ? -1 : 1;  
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    job_info * mdata1 = ((job *) a)->metadata;
    job_info * mdata2 = ((job *) b)->metadata;
    
    if (mdata1->pri == mdata2->pri) {
        return -1; // not sure about this 
    }
    return mdata1->pri > mdata2->pri ? 1 : -1;  
    return 0;
}

int comparer_psrtf(const void *a, const void *b) {
    // TODO: Implement me!
    return comparer_sjf(a, b); // pretty sure the pre part is in scheduler_quantum_expired()
}

int comparer_rr(const void *a, const void *b) {
    // pretty sure we do nothing.
    return 1;
}

int comparer_sjf(const void *a, const void *b) {
    job_info * mdata1 = ((job *) a)->metadata;
    job_info * mdata2 = ((job *) b)->metadata;
    
    if (mdata1->runningTime == mdata2->runningTime) {
        return -1; //mdata1->arrivalTime < mdata2->arrivalTime ? -1 : 1;  
    }
    return mdata1->runningTime > mdata2->runningTime ? 1 : -1;
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // move over the relevent data
    job_info * mdata = malloc(sizeof(job_info));
    mdata->arrivalTime = time;
    mdata->id = job_number;
    mdata->runningTime = sched_data->running_time;
    mdata->runningTimeOld = sched_data->running_time;
    
    mdata->pri = sched_data->priority;
    mdata->started = false;
    newjob->metadata = mdata;
    priqueue_offer(&pqueue, newjob);
    totalJobs++;
}

job * SetResTime(job * job, double time) {
    if (job == NULL) {
        return NULL;
    }
    job_info * mdata = job->metadata;
    if (mdata->started == false) {
        mdata->ResponseStartTime = time;
        mdata->started = true;
    }
    return job;
}

// Used chatGPT to understand how some of the behaviour should be for some of the schemes 
job *scheduler_quantum_expired(job *job_evicted, double time) {
    if (job_evicted != NULL) {
        job_info * mdata = job_evicted->metadata;
        mdata->runningTime -= time - mdata->arrivalTime;
    }
    if (pqueue_scheme == FCFS || pqueue_scheme == SJF || pqueue_scheme == PRI) {
        // keep going until we finish the current job
        if (job_evicted != NULL) {
            return SetResTime(job_evicted, time);
        }
        // current job is finished
        job *next_job = priqueue_poll(&pqueue); // NULL if q is empty 
        return SetResTime(next_job, time);
    }
    
    if (pqueue_scheme == RR || pqueue_scheme == PSRTF || pqueue_scheme == PPRI) {
        // throw it back in the q and let others have a turn 
        if (job_evicted != NULL) {
            priqueue_offer(&pqueue, job_evicted);
        }
        return SetResTime(priqueue_poll(&pqueue), time);  
    }


    return NULL;
}

void scheduler_job_finished(job *job_done, double time) {
    //collect stats
    job_info * mdata = job_done->metadata;
    totalTurnAroundTime += time - mdata->arrivalTime;
    totalWaitTime += (time - mdata->arrivalTime) - mdata->runningTimeOld;
    totalResponseTime = mdata->ResponseStartTime - mdata->arrivalTime;
    // free data
    if (job_done != NULL && job_done->metadata != NULL) {
        free(job_done->metadata);
    }
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return totalWaitTime / totalJobs;
}

double scheduler_average_turnaround_time() {
    return totalTurnAroundTime / totalJobs;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return totalResponseTime / totalJobs;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
