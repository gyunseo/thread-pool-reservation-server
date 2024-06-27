#include "tlv.h"
#include <pthread.h>
#include <stdio.h>

#define QSIZE 300

typedef int Task;

typedef struct {
  int putIndex; // rear
  int getIndex; // front
  int length;   // size
  int capacity;
  pthread_mutex_t lockQueue;
  pthread_cond_t condNotFull;
  pthread_cond_t condNotEmpty;
  Task TaskArr[];
} TaskQueue;

void putTask(TaskQueue *queue, Task task);
Task getTask(TaskQueue *queue);
TaskQueue *initTaskQueue(int capacity);
void deleteTaskQueue(TaskQueue *queue);