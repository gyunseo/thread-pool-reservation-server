#include "task_queue.h"

// Put data unless queue is full
void putTask(TaskQueue *queue, Task task) {
  // 1. Lock
  pthread_mutex_lock(&queue->lockQueue);
  // 2. Wait until queue is not full
  while (queue->length == QSIZE) {
    pthread_cond_wait(&queue->condNotFull, &queue->lockQueue);
  }
  // 3. Put Algorithm for Circular Array

  // 3.a. Assign the value `data` to the position pointed by `put_index`
  queue->TaskArr[queue->putIndex] = task;
  // 3.b. Modularly increase `put_index` by 1 so that it is within `capacity`
  queue->putIndex = (queue->putIndex + 1) % QSIZE;
  // at all times 3.c. Increase `length` by 1
  queue->length++;

  // printf("put task: %d to queue\n", task);

  // 4. Send a signal that queue is not empty
  pthread_cond_signal(&queue->condNotEmpty);
  // 5. Unlock
  pthread_mutex_unlock(&queue->lockQueue);
}

// Get data unless queue is empty
Task getTask(TaskQueue *queue) {
  Task task;
  // 1. Lock
  pthread_mutex_lock(&queue->lockQueue);
  // 2: Wait until queue is not empty
  while (queue->length == 0) {
    pthread_cond_wait(&queue->condNotEmpty, &queue->lockQueue);
  }
  // 3. Get Algorithm for Circular Array
  // 3.a. Get the data at index `get_index` and then modularly add `get_index`
  // by
  //      so that it is within capacity at all times
  task = queue->TaskArr[queue->getIndex];
  // 3.b. Decrease `length` by 1
  queue->length--;
  // 3.c. When `length` is zero, set `get_index` and `put_index` to 0
  if (queue->length == 0) {
    queue->getIndex = queue->putIndex = 0;
  } else {
    queue->getIndex = (queue->getIndex + 1) % QSIZE;
  }

  // printf("get task: %d from queue\n", task);

  // 4. Send a signal that queue is not full
  pthread_cond_signal(&queue->condNotFull);
  // 5. Unlock
  // Note: Signal before releasing the lock because the signal thread might
  // acquire the lock again.
  pthread_mutex_unlock(&queue->lockQueue);
  // 6. Return data
  return task;
}

TaskQueue *initTaskQueue(int capacity) {
  TaskQueue *queue =
      (TaskQueue *)malloc(sizeof(TaskQueue) + sizeof(int[capacity]));
  queue->capacity = capacity;
  queue->putIndex = queue->getIndex = queue->length = 0;
  pthread_mutex_init(&queue->lockQueue, NULL);
  pthread_cond_init(&queue->condNotFull, NULL);
  pthread_cond_init(&queue->condNotEmpty, NULL);
  return queue;
}

void deleteTaskQueue(TaskQueue *queue) {
  pthread_mutex_destroy(&queue->lockQueue);
  pthread_cond_destroy(&queue->condNotFull);
  pthread_cond_destroy(&queue->condNotEmpty);
  free(queue);
}
