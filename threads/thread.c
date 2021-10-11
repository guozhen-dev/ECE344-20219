#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <stdarg.h>
#include "stdio.h"

#define DBG_LVL 30
/* This is the wait queue structure */
struct wait_queue {
    /* ... Fill this in Lab 3 ... */
};

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
    NOT_EXIST = -1,

};


/* This is the thread control block */
struct thread {
    Tid thr_id;
    ucontext_t ctx;
    int state;
    char *stack;
};

struct thread ThreadList[THREAD_MAX_THREADS];

struct queueNode {
    struct queueNode *next;
    struct thread *thread;
};

struct queueNode *rq; //The ready queue

//Here are some wrapper functions

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
    return ret;
}

struct thread *dequeue_ready_by_id(Tid tid) {
    struct queueNode *cur = rq;
    struct queueNode *prev = NULL;
    struct thread *ret = NULL;

    if (!rq) return NULL;
    if (rq -> thread -> thr_id == tid) {
        void * oldpos = rq; 
        ret = rq-> thread;
        rq = rq->next ;
        free(oldpos);
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
    if (DBG_LVL > 20 )return ;
    struct queueNode *head = rq;
    printf("\033[34m[Queue]: ");

    while (head) {
        printf("%d ", head->thread->thr_id);
        head = head -> next;
    }
    printf("\033[0m\n");

}

/* Thread started by calling thread_stub(), *arg will be send to arg of the callee function
*/

void thread_stub (void *func (void *), void *arg) {
    dprint(1, "Stub is Running with func %p, arg %p", func, arg);
    if (ThreadList[thread_id()].state == KILLED) {
        thread_exit();
    }
    func(arg);
    thread_exit();
}

void
thread_init(void) {
    /* your optional code here */
    currentThread = 0;
    ThreadList[currentThread].state = RUNNING;
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
}

Tid
thread_id() {
    return currentThread;
}

Tid
thread_create(void (*fn) (void *), void *parg) {
    //Now we need to find an available thread number
    Tid new = THREAD_FAILED;
    for(int i = 0; i < THREAD_MAX_THREADS ; i++) {
        if (ThreadList[i].state == NOT_EXIST ) {
            new = i;
            break;
        }else if(ThreadList[i].state == KILLED || ThreadList[i].state == EXITED){
            free(ThreadList[i].stack);
            ThreadList[i].state = NOT_EXIST; 
            i--;
        }
    }
    if (new == THREAD_FAILED) {
        return THREAD_NOMORE;
    }
    dprint(1, "The new TID is %d", new);
    // The new thread will use Tid new
    // Get a valid context
    getcontext(&ThreadList[new].ctx);

    //Allocate a new stack
    ThreadList[new].stack = malloc(THREAD_MIN_STACK);
    //Check if success
    if (ThreadList[new].stack == NULL) return THREAD_NOMEMORY;

    //Now fill in the registers
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RIP] = (long long int)&thread_stub;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP] = (long long int)ThreadList[new].stack - (long long int)ThreadList[new].stack % 16;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RSP] = (long long int)ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP] + THREAD_MIN_STACK - 8;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
    ThreadList[new].ctx.uc_mcontext.gregs[REG_RSI] = (long long int)parg;


    ThreadList[new].state = READY;
    dprint(1, "Stack: %p", ThreadList[new].stack);
    dprint(1, "%%rbp: %p", ThreadList[new].ctx.uc_mcontext.gregs[REG_RBP]);
    dprint(1, "Get all the parameters Filled!");
    enqueue_ready(&ThreadList[new]);
    // print_ready_queue();

    return new;
}

Tid
thread_yield(Tid want_tid) {

    dprint(13, "Current id: %d, want_tid: %d", thread_id(),  want_tid);

    //    print_ready_queue();

    // if (want_tid == THREAD_SELF){
    //  want_tid = thread_id();
    // }
    // if (want_tid == THREAD_ANY){
    //  if (!rq){
    //      return THREAD_NONE;
    //  }else{
    //      want_tid = dequeue_ready()->thr_id;
    //  }
    // }

    //    if (want_tid < 0 || want_tid > THREAD_MAX_THREADS -1){
    //        return THREAD_INVALID;
    //    }
    // if (ThreadList[want_tid].state == NOT_EXIST || ThreadList[want_tid].state == EXITED || ThreadList[want_tid].state == KILLED){
    //  return THREAD_INVALID;
    // }
    // //Save the context to the TCB
    //    volatile int restore_time = 0;
    //    dprint(1, "Yield to: %d", want_tid);
    //    assert(ThreadList[thread_id()].state==RUNNING);
    //    ThreadList[thread_id()].state = READY;
    //    ThreadList[want_tid].state = RUNNING;

    //    if(thread_id() != want_tid)
    //        enqueue_ready(&ThreadList[thread_id()]);
    //    getcontext(&ThreadList[thread_id()].ctx);
    //    restore_time ++;

    //    //Write the context back to TCB
    //    if (restore_time < 2)
    //        setcontext(&ThreadList[want_tid].ctx);
    //    currentThread = want_tid;

    //    dprint(1,"After Yeild: %d", thread_id());
    //    return want_tid;
    if(want_tid == THREAD_SELF) {
        want_tid = thread_id();
    } else if (want_tid == THREAD_ANY) {
        if (!rq) {
            return THREAD_NONE;
        } else {
            want_tid = peek_ready()->thr_id;
            while(ThreadList[want_tid].state != READY){
                dequeue_ready_by_id(want_tid);
                want_tid = peek_ready() -> thr_id; 
            }
        }
    } else if (want_tid < 0 || want_tid > THREAD_MAX_THREADS || ThreadList[want_tid].state == NOT_EXIST || ThreadList[want_tid].state == EXITED) {
        return THREAD_INVALID;
    }
    if (ThreadList[thread_id()].state == RUNNING){
        ThreadList[thread_id()].state = READY;
    }
    ThreadList[want_tid].state = RUNNING;
    // print_ready_queue();
    if( ThreadList[thread_id()].state == READY){
        dprint(1, "ENQ %d", thread_id());
        enqueue_ready(&ThreadList[thread_id()]);
    }
    // print_ready_queue();
    dprint(1, "DEQ %d", want_tid);
    dequeue_ready_by_id(want_tid);
    dprint(1, "Queue Maintance success");
    // print_ready_queue();
    volatile int restore_cnt = 0;
    Tid prev = currentThread;
    getcontext(&ThreadList[thread_id()].ctx);
    restore_cnt ++;
    if (restore_cnt == 1) {
        currentThread = want_tid;
        setcontext(&ThreadList[want_tid].ctx);
    }else{
        if (ThreadList[prev].state == EXITED){
            free(ThreadList[prev].stack);
        }
    }
    return want_tid;
}

void
thread_exit() {
    print_ready_queue(); 
    dprint(12,"Exiting %d", thread_id());
    ThreadList[thread_id()].state = EXITED;
    thread_yield(THREAD_ANY);
}

Tid
thread_kill(Tid tid) {
    dprint(22, "killing %d from %d", tid, thread_id());
    if(tid == thread_id()) {
        dprint(1, "Cannot Kill my self ^_^!");
        return THREAD_INVALID;
    } else if (tid < 0 || tid > THREAD_MAX_THREADS || ThreadList[tid].state != READY) {
        dprint(1, "Not a valid thread!");
        return THREAD_INVALID;
    } else {
        dequeue_ready_by_id(tid);
        ThreadList[tid].state = NOT_EXIST;
        free(ThreadList[tid].stack);
        return tid;
    }
    return THREAD_FAILED;
}



/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create() {
    struct wait_queue *wq;

    wq = malloc(sizeof(struct wait_queue));
    assert(wq);

    TBD();

    return wq;
}

void
wait_queue_destroy(struct wait_queue *wq) {
    TBD();
    free(wq);
}

Tid
thread_sleep(struct wait_queue *queue) {
    TBD();
    return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all) {
    TBD();
    return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid) {
    TBD();
    return 0;
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
