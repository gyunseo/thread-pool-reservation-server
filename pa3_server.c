#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "service.h"
#include "task_queue.h"
#include "user_queue.h"
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
  TaskQueue *taskQueue;
  UserQueue *userQueue;
} ThreadArgs;

#define MAX_CLIENTS 256

int NUM_THREADS;
int server_fd, port;
void handle_exit();
int getNumCores();
void *consume(void *args);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);
  NUM_THREADS = getNumCores();
  pthread_t *workerThreads = malloc(NUM_THREADS * sizeof(pthread_t));
  TaskQueue *taskQueue = initTaskQueue(QSIZE);
  UserQueue *userQueue = initUserQueue(QSIZE);

  printf("Number of cores: %d\n", NUM_THREADS);
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    exit(1);
  }

  listen(server_fd, MAX_CLIENTS);
  printf("Server is listening on port %d...\n", port);

  // Register handle_exit to be called at program exit
  atexit(handle_exit);

  ThreadArgs args = {taskQueue, userQueue};
  // Create worker threads and assign them to consume function
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&workerThreads[i], NULL, consume, (void *)&args);
  }

  // Main loop to accept incoming connections (producer)
  while (true) {
    int connFD;
    if ((connFD = accept(server_fd, (struct sockaddr *)&client_addr,
                         &addr_len)) < 0) {
      perror("accept");
      continue;
    }
    putTask(taskQueue, connFD);
  }
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(workerThreads[i], NULL);
  }

  free(workerThreads);
  deleteTaskQueue(taskQueue);
  deleteUserQueue(userQueue);
  return 0;
}

void handle_exit() { close(server_fd); }
int getNumCores() {
  cpu_set_t cpu_set;
  sched_getaffinity(0, sizeof(cpu_set), &cpu_set);
  return CPU_COUNT_S(sizeof(cpu_set), &cpu_set);
}
void *consume(void *args) {
  int i;
  ThreadArgs *threadArgs = (ThreadArgs *)args;
  TaskQueue *taskQueue = threadArgs->taskQueue;
  UserQueue *userQueue = threadArgs->userQueue;
  while (1) {
    Task connFD;
    connFD = getTask(taskQueue);
    while (1) {
      tlv_t tlv;
      if (recv_tlv(connFD, &tlv) == -1) {
        printf("Connection was terminated from %d\n", connFD);
        break;
      } else {
        Query query;
        memcpy(&query, tlv.value, sizeof(Query) - sizeof(char *));
        if (tlv.length > sizeof(Query) - sizeof(char *)) {
          query.password =
              (char *)malloc(tlv.length - (sizeof(Query) - sizeof(char *)));
          memcpy(query.password, tlv.value + sizeof(Query) - sizeof(char *),
                 tlv.length - (sizeof(Query) - sizeof(char *)));
        } else {
          query.password = NULL;
        }
        // printf("Received - Type: %u, Length: %u, Value: UserID: %d, ActionID:
        // "
        //        "%d, SeatNumber: %d, password: %s\n",
        //        tlv.type, tlv.length, query.userID, query.actionID,
        //        query.seatNumber, query.password ? query.password : "(null)");
        switch (query.actionID) {
        case 0:
          // printf("Action: Termination\n");
          action0(connFD, query.userID, query.actionID, query.seatNumber);
          break;
        case 1:
          // printf("Action: Login\n");
          action1(connFD, userQueue, query.userID, query.password);
          break;
        case 2:
          // printf("Action: Book a Seat\n");
          action2(connFD, userQueue, query.userID, query.seatNumber);
          break;
        case 3:
          // printf("Action: Confirm Booking\n");
          action3(connFD, userQueue, query.userID, query.seatNumber);
          break;
        case 4:
          // printf("Action: Cancel\n");
          action4(connFD, userQueue, query.userID, query.seatNumber);
          break;
        case 5:
          // printf("Action: Logout\n");
          action5(connFD, userQueue, query.userID, query.seatNumber);
          break;
        default:
          printf("Action %d is unknown\n", query.actionID);
          Res res = {0};
          res.actionID = query.actionID;
          res.code = 1;
          tlv_t resTLV = {
              .type = 0, .length = sizeof(Res), .value = (uint8_t *)&res};
          send_tlv(connFD, &resTLV);
          break;
        }

        if (query.password) {
          free(query.password);
        }

        free(tlv.value);
      }
    }
    close(connFD);
  }
  pthread_exit(NULL);
  return NULL;
}