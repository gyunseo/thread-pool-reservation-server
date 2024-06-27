#include "user_queue.h"

// Put data unless queue is full
void putUser(UserQueue *queue, User user) {
  // 1. Lock
  pthread_mutex_lock(&queue->lockQueue);
  // 2. Wait until queue is not full
  while (queue->length == QSIZE) {
    pthread_cond_wait(&queue->condNotFull, &queue->lockQueue);
  }
  // 3. Put Algorithm for Circular Array

  // 3.a. Assign the value `data` to the position pointed by `put_index`
  queue->UserArr[queue->putIndex] = user;
  // 3.b. Modularly increase `put_index` by 1 so that it is within `capacity`
  queue->putIndex = (queue->putIndex + 1) % QSIZE;
  // at all times 3.c. Increase `length` by 1
  queue->length++;

  // 4. Send a signal that queue is not empty
  pthread_cond_signal(&queue->condNotEmpty);
  // 5. Unlock
  pthread_mutex_unlock(&queue->lockQueue);
}

// Get data unless queue is empty
User getUser(UserQueue *queue) {
  User user;
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
  user = queue->UserArr[queue->getIndex];
  // 3.b. Decrease `length` by 1
  queue->length--;
  // 3.c. When `length` is zero, set `get_index` and `put_index` to 0
  if (queue->length == 0) {
    queue->getIndex = queue->putIndex = 0;
  } else {
    queue->getIndex = (queue->getIndex + 1) % QSIZE;
  }

  // 4. Send a signal that queue is not full
  pthread_cond_signal(&queue->condNotFull);
  // 5. Unlock
  // Note: Signal before releasing the lock because the signal thread might
  // acquire the lock again.
  pthread_mutex_unlock(&queue->lockQueue);
  // 6. Return data
  return user;
}

bool userLoggedIn(UserQueue *queue, uint32_t userID) {
  pthread_mutex_lock(&queue->lockQueue);
  // condition variable 쓸 여지가 있음
  // from getIndex to putIndex because the queue is circular
  for (int i = queue->getIndex;; i = (i + 1) % QSIZE) {
    if (i == queue->putIndex) {
      break;
    }
    if (queue->UserArr[i].userID == userID && queue->UserArr[i].isLoggedIn) {
      pthread_mutex_unlock(&queue->lockQueue);
      return true;
    }
  }
  pthread_mutex_unlock(&queue->lockQueue);
  return false;
}

bool isClientActive(UserQueue *queue, int clientID) {
  pthread_mutex_lock(&queue->lockQueue);
  // Condition variable 쓸 여지가 있음
  // from getIndex to putIndex because the queue is circular
  for (int i = queue->getIndex;; i = (i + 1) % QSIZE) {
    if (i == queue->putIndex) {
      break;
    }
    if (queue->UserArr[i].clientID == clientID) {
      pthread_mutex_unlock(&queue->lockQueue);
      return true;
    }
  }
  pthread_mutex_unlock(&queue->lockQueue);
  return false;
}

bool userExistsInQueue(UserQueue *queue, uint32_t userID) {
  pthread_mutex_lock(&queue->lockQueue);
  // Condition variable 쓸 여지가 있음
  // from getIndex to putIndex because the queue is circular
  for (int i = queue->getIndex;; i = (i + 1) % QSIZE) {
    if (i == queue->putIndex) {
      break;
    }
    if (queue->UserArr[i].userID == userID) {
      pthread_mutex_unlock(&queue->lockQueue);
      return true;
    }
  }
  pthread_mutex_unlock(&queue->lockQueue);
  return false;
}

int getIndexOfUser(UserQueue *queue, uint32_t userID) {
  pthread_mutex_lock(&queue->lockQueue);
  // Condition variable 쓸 여지가 있음
  // from getIndex to putIndex because the queue is circular
  for (int i = queue->getIndex;; i = (i + 1) % QSIZE) {
    if (i == queue->putIndex) {
      break;
    }
    if (queue->UserArr[i].userID == userID) {
      pthread_mutex_unlock(&queue->lockQueue);
      return i;
    }
  }
  pthread_mutex_unlock(&queue->lockQueue);
  return -1;
}

UserQueue *initUserQueue(int capacity) {
  UserQueue *queue =
      (UserQueue *)malloc(sizeof(UserQueue) + sizeof(User[capacity]));
  queue->capacity = capacity;
  queue->putIndex = queue->getIndex = queue->length = 0;
  for (size_t i = 0; i < QSIZE; i++) {
    queue->UserArr[i].userID = 0;
    queue->UserArr[i].clientID = 0;
    queue->UserArr[i].isLoggedIn = false;
    queue->UserArr[i].numBookedSeats = 0;
    memset(queue->UserArr[i].bookedSeats, 0x00,
           sizeof(queue->UserArr[i].bookedSeats));
  }
  pthread_mutex_init(&queue->lockQueue, NULL);
  pthread_cond_init(&queue->condNotFull, NULL);
  pthread_cond_init(&queue->condNotEmpty, NULL);
  return queue;
}

void deleteUserQueue(UserQueue *queue) {
  pthread_mutex_destroy(&queue->lockQueue);
  pthread_cond_destroy(&queue->condNotFull);
  pthread_cond_destroy(&queue->condNotEmpty);
  free(queue);
}
