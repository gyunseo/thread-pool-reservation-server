#ifndef USER_QUEUE_H
#define USER_QUEUE_H
#include "tlv.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#define QSIZE 300

typedef struct {
  uint32_t userID;
  int clientID;
  bool isLoggedIn;
  int numBookedSeats;
  int bookedSeats[300];
} User;

typedef struct {
  int putIndex; // rear
  int getIndex; // front
  int length;   // size
  int capacity;
  pthread_mutex_t lockQueue;
  pthread_cond_t condNotFull;
  pthread_cond_t condNotEmpty;
  User UserArr[];
} UserQueue;

void putUser(UserQueue *queue, User user);
User getUser(UserQueue *queue);
bool userLoggedIn(UserQueue *queue, uint32_t userID);
bool isClientActive(UserQueue *queue, int clientID);
bool userExistsInQueue(UserQueue *queue, uint32_t userID);
int getIndexOfUser(UserQueue *queue, uint32_t userID);
UserQueue *initUserQueue(int capacity);
void deleteUserQueue(UserQueue *queue);
#endif