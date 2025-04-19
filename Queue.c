#include "Queue.h"


void queue_push(WorkerQueue *worker_queue, const char* src, const char* tgt) {
    WorkerQueue* new_worker = malloc(sizeof(WorkerQueue));
    strcpy(new_worker->source_dir, src);
    strcpy(new_worker->target_dir, tgt);
    new_worker->next = worker_queue;
    worker_queue = new_worker;
}

void queue_pop(WorkerQueue *worker_queue) {
    if (worker_queue == NULL) {
        return;
    }
    WorkerQueue * temp = worker_queue->next;
    free (worker_queue);
    worker_queue = temp;
}

