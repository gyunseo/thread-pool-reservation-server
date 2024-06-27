#include "user_queue.h"
#include <argon2.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MEMORY_USAGE 512
#define SALT_SIZE 16
#define HASH_SIZE 32
#define HASHED_PASSWORD_SIZE 128

extern bool fileInUse;
extern pthread_mutex_t lockFile;
extern pthread_cond_t condFile;
extern bool isSeatBooked[257];
extern pthread_mutex_t lockSeat;
extern pthread_cond_t condSeat;

void generate_salt(uint8_t *salt);
void hash_password(char *password, char *hashed_password);
int validate_password(char *password_to_validate, char *hashed_password);
void action0(int clientFD, uint32_t userID, int actionID, int seatNumber);
void action1(int clientFD, UserQueue *queue, uint32_t userID, char *password);
void action2(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber);
void action3(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber);
void action4(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber);
void action5(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber);