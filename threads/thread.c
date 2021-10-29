#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <stdarg.h>
#include "stdio.h"
#include <stdbool.h>

#define DBG_LVL 108 //Debug level set to > 100 for debug purpose of lab 3 
// DBGLVL 101 lab3 preemetive
// DBGLVL 102 lab3 wakeup
// DBGLVL 104 lab3 wait
// DBGLVL 106 lab3 wait kill
//



// This is a function for debug print
void dprint(int lvl, char *fmt, ...) {
    if (lvl < DBG_LVL) return ;
    printf("\033[33m[DEBUG]: ");
    va_list args;
    va_start (args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\033[0m\n");
    return ;
}

volatile Tid currentThread;
enum {
    READY = 0,
    RUNNING = 1,
    BLOCKED = 2,
    KILLED = 3,
    EXITED = 4,
    SLEEPING = 5,
    NOT_EXIST = -1,

};

/* This is the wait queue structure */
struct wait_queue {
    //Reuse the same thing in the ready queue
    struct queueNode *wq;
};

/* This is the thread control block */
struct thread {
    Tid thr_id;
    ucontext_t ctx;
    int state;
    char *stack;
    //This is added in lab3, a wait queue connecting to this thread
    struct wait_queue *wq;
    Tid parent_id;
};

struct thread ThreadList[THREAD_MAX_THREADS];

struct queueNode {
    struct queueNode *next;
    struct thread *thread;
};

struct queueNode *rq; //The ready queue



//Here are some wrapper functions

bool islastThread() {
    for (int i = 0 ; i < THREAD_MAX_THREADS ; i++) {
        if (i != thread_id()) {
            if (ThreadList[i].state == READY) {
                return false;
            }
        }
    }
    return true;
}
void enqueue_ready(struct thread *th) {
    struct queueNode *cur = rq;
    if (cur) {
        while(cur -> next) {
            cur = cur -> next;
            if (cur->thread->thr_id == th->thr_id) {
                dprint(1, "Already in queue");
                return;
            }
        }
        struct queueNode *newNode = malloc(sizeof(struct queueNode));
        newNode -> thread = th;
        newNode -> next = NULL ;
        cur -> next = newNode;
    } else {
        rq = malloc(sizeof(struct queueNode));
        rq->next = NULL;
        rq->thread = th;
    }
    return ;

}

struct thread *dequeue_ready() {
    struct queueNode *head = rq;
    rq = rq -> next;
    struct thread *ret = head->thread;
    free(head);
    head = NULL;
    return ret;
}

struct thread *dequeue_ready_by_id(Tid tid) {
    struct queueNode *cur = rq;
    struct queueNode *prev = NULL;
    struct thread *ret = NULL;

    if (!rq) return NULL;
    if (rq -> thread -> thr_id == tid) {
        void *oldpos = rq;
        ret = rq-> thread;
        rq = rq->next ;
        free(oldpos);
        oldpos = NULL;
        return ret;
    }
    while(cur) {
        if(cur->thread->thr_id == tid) {
            ret = cur -> thread;
            prev->next = cur -> next;
        }
        prev = cur;
        cur = cur-> next;
    }

    return ret;

}

struct thread *peek_ready() {
    return rq -> thread;
}


void print_ready_queue() {
    int enable = interrupts_set(0);
    if (DBG_LVL > 20 )return ;
    struct queueNode *head = rq;
    printf("\033[34m[Queue]: ");

    while (head) {
        printf("%d ", head->thread->thr_id);
        head = head -> next;
    }
    printf("\033[0m\n");
    interrupts_set(enable);

}

/* Thread started by calling thread_stub(), *arg will be send to arg of the callee function
*/

void thread_stub (void *func (void *), void *arg) {
    dprint(1, "Stub is Running with func %p, arg %p", func, arg);
    interrupts_on();
    // if (ThreadList[thread_id()].state == KILLED) {
    //     thread_exit();
    // }
    func(arg);
    dprint(25, "Calling EXIT from %d", thread_id());
    thread_exit();
}

void
thread_init(void) {
    /* your optional code here */
    int enable = interrupts_set(0);
    currentThread = 0;
    ThreadList[currentThread].state = RUNNING;
    ThreadList[currentThread].wq = malloc(sizeof(struct wait_queue));
    rq = NULL;
    // rq = malloc(sizeof(struct queueNode));
    // rq -> prev = NULL;
    // rq -> next = NULL;
    // rq -> thread = NULL;

    //initialize all the tid
    for (int i = 0 ; i < THREAD_MAX_THREADS ; i++) {
        ThreadList[i].thr_id = i;
    }
    for (int i = 1 ; i < THREAD_MAX_THREADS ; i++) {
        ThreadList[i].state = NOT_EXIST;
    }
    interrupts_set(enable);
}

Tid
thread_id() {
    return currentThread;
}

Tid
thread_create(void (*fn) (void *), void *parg) {
    int enable = interrupts_set(0);
    enable = 1;
    dprint(101, "Get interrupt in create status now is %d", enable);

    //Now we need to find an available thread number
    Tid new = THREAD_FAILED;
    for(int i = 0; i < THREAD_MAX_THREADS ; i++) {
        if (ThreadList[i].state == NOT_EXIST ) {
            new = i;
            break;
        } else if(ThreadList[i].stack && (ThreadList[i].state == KILLED || ThreadList[i].state == EXITED)) {
            free(ThreadList[i].stack);
            ThreadList[i].stack = NULL;
            ThreadList[i].state = NOT_EXIST;
            i--;
        }
    }
    if (new == THREAD_FAILED) {
        interrupts_set(enable);
        return THREAD_NOMORE;
    }
    dprint(1, "The new TID is %d", new);
    // The new thread will use Tid new
    // Get a valid context
    getcontext(&ThreadList[new].ctx);

    //Allocate a new stack
    ThreadList[new].stack = malloc(THREAD_MIN_STACK);

    //Allocate a wq for lab3
    ThreadList[new].wq = malloc(sizeof(struct wait_queue));


    //Check if success
    if (ThreadList[new].stack == NULL) {
        interrupts_set(enable);
        return THREAD_NOMEMORY;
    }

    //Now fill in the registers
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RIP] = (long long int)&thread_stub;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP] = (long long int)ThreadList[new].stack - (long long int)ThreadList[new].stack % 16;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RSP] = (long long int)ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP] + THREAD_MIN_STACK - 8;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RSI] = (long long int)parg;


    ThreadList[new].state = READY;
    ThreadList[new].parent_id = THREAD_INVALID;
    dprint(1, "Stack: %p", ThreadList[new].stack);
    dprint(1, "%%rbp: %p", ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP]);
    dprint(1, "Get all the parameters Filled!");
    enqueue_ready(&ThreadList[new]);
    // print_ready_queue();

    interrupts_set(enable);
    return new;
}

Tid
thread_yield(Tid want_tid) {
    dprint(102, "Get interrupt in yield status now is %d, from Thread: %d", interrupts_enabled(), thread_id());
    int enable = interrupts_set(0);
    for (int i = 0 ; i < THREAD_MAX_THREADS  ; i++) {
        if (ThreadList[i].stack && ThreadList[i].state == EXITED && i != thread_id()) {
            dprint(41, "Thread is exited, earsing %d", i);
            free(ThreadList[i].stack);
            ThreadList[i].stack = NULL;
            ThreadList[i].state = NOT_EXIST;
        }
    }

    if(want_tid == THREAD_SELF) {
        want_tid = thread_id();
    } else if (want_tid == THREAD_ANY) {
        if (!rq) {
            interrupts_set(enable);
            return THREAD_NONE;
        } else {
            want_tid = peek_ready()->thr_id;
            while(ThreadList[want_tid].state != READY) {
                dprint(107, "Throw away thread %d", want_tid);
                dequeue_ready();
                want_tid = peek_ready() -> thr_id;
            }
        }
    } else if (want_tid < 0 || want_tid > THREAD_MAX_THREADS || ThreadList[want_tid].state == NOT_EXIST || ThreadList[want_tid].state == EXITED) {
        interrupts_set(enable);
        return THREAD_INVALID;
    }

    dprint(107, "Current id: %d, state: %d, want_tid: %d, state %d", thread_id(), ThreadList[thread_id()].state,  want_tid, ThreadList[want_tid].state);


    if (ThreadList[thread_id()].state == RUNNING) {
        ThreadList[thread_id()].state = READY;
    }
    ThreadList[want_tid].state = RUNNING;
    print_ready_queue();
    if( ThreadList[thread_id()].state == READY) {
        dprint(103, "ENQ %d, state is %d", thread_id(), ThreadList[thread_id()].state);
        enqueue_ready(&ThreadList[thread_id()]);
    }
    // print_ready_queue();
    dprint(1, "DEQ %d", want_tid);
    dequeue_ready_by_id(want_tid);
    dprint(1, "Queue Maintance success");
    // print_ready_queue();
    volatile int restore_cnt = 0;
    // volatile int prev_tid = thread_id();
    getcontext(&ThreadList[thread_id()].ctx);
    dprint(101, "Now the enable is %d", enable);
    restore_cnt ++;
    if (restore_cnt == 1) {
        currentThread = want_tid;
        dprint(101, "The enable now is %d", enable);
        setcontext(&ThreadList[want_tid].ctx);
    }
    interrupts_set(enable);
    return want_tid;
}

void
thread_exit() {
    int enable = interrupts_set(0);
    // print_ready_queue();
    dprint(104, "Exiting %d", thread_id());
    ThreadList[thread_id()].state = EXITED;
    if (ThreadList[thread_id()].wq ->wq)
        dprint(104, "Wake up %d", ThreadList[thread_id()].wq ->wq -> thread -> thr_id);
    thread_wakeup(ThreadList[thread_id()].wq, 1);
    if(thread_yield(THREAD_ANY) == THREAD_NONE) {
        exit(0);
    }
    interrupts_set(enable);

}
struct thread *dequeue_wait_by_id(struct wait_queue *wq, Tid tid) ;

Tid
thread_kill(Tid tid) {
    int enable = interrupts_set(0);
    dprint(107, "killing %d from %d, parent is : %d", tid, thread_id(), ThreadList[thread_id()].parent_id);
    if(tid == thread_id()) {
        dprint(1, "Cannot Kill my self ^_^!");
        interrupts_set(enable);
        return THREAD_INVALID;
    } else if (tid == ThreadList[thread_id()].parent_id) {
        dprint(106, "kill parent");
        thread_wakeup(ThreadList[tid].wq, 1);
        dequeue_wait_by_id(ThreadList[thread_id()].wq, tid);
        ThreadList[tid].state = NOT_EXIST;
        ThreadList[tid].parent_id = THREAD_INVALID;
        free(ThreadList[tid].stack);
        ThreadList[tid].stack = NULL;
        return tid;

    } else if (tid < 0 || tid > THREAD_MAX_THREADS || (ThreadList[tid].state != READY && ThreadList[tid].state != SLEEPING)) {
        dprint(1, "Not a valid thread!");
        interrupts_set(enable);
        return THREAD_INVALID;
    }

    dequeue_ready_by_id(tid);
    thread_wakeup(ThreadList[tid].wq, 1);
    ThreadList[tid].state = NOT_EXIST;
    ThreadList[tid].parent_id = THREAD_INVALID;
    free(ThreadList[tid].stack);
    ThreadList[tid].stack = NULL;
    interrupts_set(enable);
    return tid;

}



/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */



void enqueue_waiting(struct wait_queue *wq,  struct thread *th) {
    struct queueNode *cur = wq -> wq;
    if (cur) {
        while(cur -> next) {
            cur = cur -> next;
            if (cur->thread->thr_id == th->thr_id) {
                dprint(1, "Already in queue");
                return;
            }
        }
        struct queueNode *newNode = malloc(sizeof(struct queueNode));
        newNode -> thread = th;
        newNode -> next = NULL ;
        cur -> next = newNode;
    } else {
        wq -> wq = malloc(sizeof(struct queueNode));
        wq -> wq ->next = NULL;
        wq -> wq ->thread = th;
    }
    return ;

}

struct thread *dequeue_wait(struct wait_queue *wq) {
    struct queueNode *head = wq->wq;
    wq -> wq = wq -> wq -> next;
    struct thread *ret = head->thread;
    free(head);
    head = NULL;
    return ret;
}

struct thread *dequeue_wait_by_id(struct wait_queue *wq, Tid tid) {
    struct queueNode *cur = wq->wq;
    struct queueNode *prev = NULL;
    struct thread *ret = NULL;

    if (!wq -> wq) return NULL;
    if (wq -> wq -> thread -> thr_id == tid) {
        void *oldpos = wq -> wq;
        ret = wq -> wq-> thread;
        wq -> wq = wq -> wq->next ;
        free(oldpos);
        oldpos = NULL;
        return ret;
    }
    while(cur) {
        if(cur->thread->thr_id == tid) {
            ret = cur -> thread;
            prev->next = cur -> next;
        }
        prev = cur;
        cur = cur-> next;
    }

    return ret;

}

struct thread *peek_wait(struct wait_queue *wq ) {
    return wq -> wq -> thread;
}


void print_wait_queue(struct wait_queue *wq) {
    int enable = interrupts_set(0);
    struct queueNode *head = wq -> wq;
    printf("\033[34m[Waiting Queue]: ");

    while (head) {
        printf("%d ", head->thread->thr_id);
        head = head -> next;
    }
    printf("\033[0m\n");
    interrupts_set(enable);

}

struct wait_queue *
wait_queue_create() {
    int enable = interrupts_set(0);
    struct wait_queue *wq;

    wq = malloc(sizeof(struct wait_queue));
    assert(wq);

    wq -> wq = NULL; //initially there is no thread in the waiting queue
    interrupts_set(enable);
    return wq;
}

void
wait_queue_destroy(struct wait_queue *wq) { // When call this we have to ensure that there is no thread in the queue
    int enable = interrupts_set(0);
    if(wq -> wq != NULL) {
        assert(0);
    }
    free(wq);
    wq = NULL;
    interrupts_set(enable);
    return;
}

Tid
thread_sleep(struct wait_queue *queue) {
    int enable = interrupts_set(0);
    dprint(104, "Thread id %d want to sleep in to queue %p", thread_id(), queue);
    if (!queue) return THREAD_INVALID;
    if (!rq) return THREAD_NONE;
    ThreadList[thread_id()].state = SLEEPING;
    enqueue_waiting(queue, &ThreadList[thread_id()]);
    while(dequeue_ready_by_id(thread_id()));
    // dprint(104, "ready to yeild, id is %d, state is %d", thread_id(), ThreadList[thread_id()].state);
    interrupts_set(enable);
    return thread_yield(THREAD_ANY) ;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all) {
    int enable = interrupts_set(0);
    int ret = 0;
    if (!queue || !queue -> wq ) {
        dprint(107, "Where is my queue?");
        ret = 0;
    } else if(all) {
        while(queue->wq != NULL) {
            struct thread *item = dequeue_wait(queue);
            if (ThreadList[item->thr_id].state == SLEEPING) {
                ThreadList[item->thr_id].state = READY;
                enqueue_ready(item);
                ret ++;
            }
        }
    } else {
        if (queue->wq == NULL) return 0;
        struct thread *item = dequeue_wait(queue);
        ThreadList[item->thr_id].state = READY;
        enqueue_ready(item);
        ret = 1;
    }
    interrupts_set(enable);
    return ret;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid) {
    // int enable = interrupts_set(0);
    if (tid < 0 || tid == thread_id() || (ThreadList[tid].state != READY && ThreadList[tid].state != SLEEPING)) {
        dprint(105, "FAIL!!, target state %d", ThreadList[tid].state);
        return THREAD_INVALID;
    }
    dprint(106, "Thread %d wait thread %d, thread: %dshould be stopped", thread_id(), tid, thread_id());
    // interrupts_set(enable);

    ThreadList[tid].parent_id = thread_id();
    thread_sleep(ThreadList[tid].wq);
    return tid;
}

struct lock {
    /* ... Fill this in ... */
};

struct lock *
lock_create() {
    struct lock *lock;

    lock = malloc(sizeof(struct lock));
    assert(lock);

    TBD();

    return lock;
}

void
lock_destroy(struct lock *lock) {
    assert(lock != NULL);

    TBD();

    free(lock);
}

void
lock_acquire(struct lock *lock) {
    assert(lock != NULL);

    TBD();
}

void
lock_release(struct lock *lock) {
    assert(lock != NULL);

    TBD();
}

struct cv {
    /* ... Fill this in ... */
};

struct cv *
cv_create() {
    struct cv *cv;

    cv = malloc(sizeof(struct cv));
    assert(cv);

    TBD();

    return cv;
}

void
cv_destroy(struct cv *cv) {
    assert(cv != NULL);

    TBD();

    free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);

    TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);

    TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock) {
    assert(cv != NULL);
    assert(lock != NULL);

    TBD();
}
