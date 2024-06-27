#include "service.h"

bool fileInUse = false;
pthread_mutex_t lockFile = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condFile = PTHREAD_COND_INITIALIZER;
bool isSeatBooked[257] = {false};
pthread_mutex_t lockSeat = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condSeat = PTHREAD_COND_INITIALIZER;

void generate_salt(uint8_t *salt) {
  int fd = open("/dev/urandom", O_RDONLY);
  read(fd, salt, SALT_SIZE);
  close(fd);
}

void hash_password(char *password, char *hashed_password) {
  uint8_t salt[SALT_SIZE];
  generate_salt(salt);
  char hash[HASHED_PASSWORD_SIZE];
  argon2id_hash_encoded(2, MEMORY_USAGE, 1, password, strlen(password), salt,
                        SALT_SIZE, HASH_SIZE, hash, HASHED_PASSWORD_SIZE);
  strcpy(hashed_password, hash);
}

int validate_password(char *password_to_validate, char *hashed_password) {
  if (argon2id_verify(hashed_password, password_to_validate,
                      strlen(password_to_validate)) == ARGON2_OK) {
    return 1;
  } else {
    return 0;
  }
}

void action0(int clientFD, uint32_t userID, int actionID, int seatNumber) {
  Res res = {0};
  // validate
  if (userID != 0 || actionID != 0 || seatNumber != 0) {
    res.actionID = 0;
    res.code = 1;
  } else {
    res.actionID = 0;
    res.code = 0;
  }
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
  // terminate the connection
}

void action1(int clientFD, UserQueue *queue, uint32_t userID, char *password) {
  // create /tmp/passwords.tsv if doesn't exist, but if exists, open in append
  // mode
  Res res = {0};
  pthread_mutex_lock(&lockFile);
  while (fileInUse) {
    pthread_cond_wait(&condFile, &lockFile);
  }
  fileInUse = true;
  FILE *fp = fopen("/tmp/passwords.tsv", "a");
  if (fp == NULL) {
    perror("fopen");
    exit(1);
  }
  FILE *readFile = fopen("/tmp/passwords.tsv", "r");

  // read userID from file, and check if userID already exists
  uint32_t userIDInFile;
  bool userExistsInFile = false;
  char hashedPasswordInFile[HASHED_PASSWORD_SIZE + 10];

  while (fscanf(readFile, "%u\t%s\n", &userIDInFile, hashedPasswordInFile) !=
         EOF) {
    // DEBUG
    // printf("userIDInFile: %u, hashedPasswordInFile: %s\n", userIDInFile,
    //        hashedPasswordInFile);
    if (userIDInFile == userID) {
      userExistsInFile = true;
      break;
    }
  }
  fclose(readFile);

  if (userLoggedIn(queue, userID)) {
    res.actionID = 1;
    res.code = 1;
    // DEBUG
    // printf("User already logged in a client\n");
    fclose(fp);
    fileInUse = false;
    pthread_cond_signal(&condFile);
    pthread_mutex_unlock(&lockFile);
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }

  if (isClientActive(queue, clientFD)) {
    res.actionID = 1;
    res.code = 2;
    // DEBUG
    // printf("Client already active\n");
    fclose(fp);
    fileInUse = false;
    pthread_cond_signal(&condFile);
    pthread_mutex_unlock(&lockFile);
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }

  if (!userExistsInFile) {
    // treat this as registration
    char hashedPassword[HASHED_PASSWORD_SIZE + 10];
    hash_password(password, hashedPassword);
    // write userID and hashed password to file
    // "userID\tpassword"
    fprintf(fp, "%u\t%s\n", userID, hashedPassword);
    User newUser = {
        .userID = userID,
        .clientID = clientFD,
        .isLoggedIn = true,
        .numBookedSeats = 0,
    };
    putUser(queue, newUser);
    // DEBUG
    // printf("registration success\n");
    fclose(fp);
    fileInUse = false;
    pthread_cond_signal(&condFile);
    pthread_mutex_unlock(&lockFile);
    res.actionID = 1;
    res.code = 0;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  if (!validate_password(password, hashedPasswordInFile)) {
    res.actionID = 1;
    res.code = 3;
    // DEBUG
    // printf("wrong password\n");
    fclose(fp);
    fileInUse = false;
    pthread_cond_signal(&condFile);
    pthread_mutex_unlock(&lockFile);
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  // treat this as login
  int idx = getIndexOfUser(queue, userID);
  // DEBUG
  // printf("idx: %d, userID: %d\n", idx, userID);
  if (idx < 0) {
    // 서버를 껐다 켜면 queue가 비어있을 수 있음
    User newUser = {
        .userID = userID,
        .clientID = clientFD,
        .isLoggedIn = true,
        .numBookedSeats = 0,
    };
    putUser(queue, newUser);
  } else {
    pthread_mutex_lock(&queue->lockQueue);
    queue->UserArr[idx].isLoggedIn = true;
    queue->UserArr[idx].clientID = clientFD;
    pthread_mutex_unlock(&queue->lockQueue);
  }
  // DEBUG
  // printf("right password, login success\n");

  fclose(fp);
  fileInUse = false;
  pthread_cond_signal(&condFile);
  pthread_mutex_unlock(&lockFile);
  res.actionID = 1;
  res.code = 0;
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
}

void action2(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber) {
  Res res = {0};
  // guard
  if (!userLoggedIn(queue, userID)) {
    // DEBUG
    // printf("User not logged in, you cannot booka a seat\n");
    res.actionID = 2;
    res.code = 1;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  // validate seatNumber
  if (seatNumber < 1 || seatNumber > 256) {
    // DEBUG
    // printf("Invalid seat number\n");
    res.actionID = 2;
    res.code = 3;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  pthread_mutex_lock(&lockSeat);
  if (isSeatBooked[seatNumber]) {
    // DEBUG
    // printf("Seat already booked\n");
    pthread_mutex_unlock(&lockSeat);
    res.actionID = 2;
    res.code = 2;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  isSeatBooked[seatNumber] = true;
  int idx = getIndexOfUser(queue, userID);
  pthread_mutex_lock(&queue->lockQueue);
  queue->UserArr[idx].bookedSeats[queue->UserArr[idx].numBookedSeats++] =
      seatNumber;
  res.seats[seatNumber] = true;
  pthread_mutex_unlock(&queue->lockQueue);
  // DEBUG
  // printf("%d Seat booked\n", seatNumber);
  pthread_mutex_unlock(&lockSeat);
  res.actionID = 2;
  res.code = 0;
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
}

void action3(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber) {
  Res res = {0};
  // guard
  if (!userLoggedIn(queue, userID)) {
    // DEBUG
    // printf("User not logged in, you cannot see booked seats\n");
    res.actionID = 3;
    res.code = 1;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  int idx = getIndexOfUser(queue, userID);
  // DEBUG
  // printf("Booked seats: ");
  if (seatNumber == -1) {
    res.actionID = 3;
    res.code = 0;
    pthread_mutex_lock(&queue->lockQueue);

    for (int i = 0; i < queue->UserArr[idx].numBookedSeats; i++) {
      res.seats[queue->UserArr[idx].bookedSeats[i]] = true;
    }
    pthread_mutex_unlock(&queue->lockQueue);
  } else if (seatNumber == 0) {
    // return the available seats
    res.actionID = 3;
    res.code = 0;
    pthread_mutex_lock(&lockSeat);
    for (int i = 1; i <= 256; i++) {
      if (!isSeatBooked[i]) {
        res.seats[i] = true;
      }
    }
    pthread_mutex_unlock(&lockSeat);
  }
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
}

void action4(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber) {
  // guard
  Res res = {0};
  if (!userLoggedIn(queue, userID)) {
    // DEBUG
    // printf("User not logged in, you cannot cancel a seat\n");
    res.actionID = 4;
    res.code = 1;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  // validate seatNumber
  if (seatNumber < 1 || seatNumber > 256) {
    // DEBUG
    // printf("Invalid seat number\n");
    res.actionID = 4;
    res.code = 3;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  pthread_mutex_lock(&lockSeat);
  if (!isSeatBooked[seatNumber]) {
    // DEBUG
    // printf("Seat not booked\n");

    pthread_mutex_unlock(&lockSeat);
    res.actionID = 4;
    res.code = 2;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  // booked by another user
  int idx = getIndexOfUser(queue, userID);
  bool isBooked = false;
  for (int i = 0; i < queue->UserArr[idx].numBookedSeats; i++) {
    if (queue->UserArr[idx].bookedSeats[i] == seatNumber) {
      isBooked = true;
      break;
    }
  }
  if (!isBooked) {
    // DEBUG
    // printf("Seat not booked by you\n");
    pthread_mutex_unlock(&lockSeat);
    res.actionID = 4;
    res.code = 2;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  isSeatBooked[seatNumber] = false;
  pthread_mutex_unlock(&lockSeat);

  pthread_mutex_lock(&queue->lockQueue);
  for (int i = 0; i < queue->UserArr[idx].numBookedSeats; i++) {
    if (queue->UserArr[idx].bookedSeats[i] == seatNumber) {
      for (int j = i; j < queue->UserArr[idx].numBookedSeats - 1; j++) {
        queue->UserArr[idx].bookedSeats[j] =
            queue->UserArr[idx].bookedSeats[j + 1];
      }
      queue->UserArr[idx].numBookedSeats--;
      break;
    }
  }
  // printf("%d Seat cancelled\n", seatNumber);
  pthread_mutex_unlock(&queue->lockQueue);
  res.actionID = 4;
  res.code = 0;
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
}

void action5(int clientFD, UserQueue *queue, uint32_t userID, int seatNumber) {
  Res res = {0};
  // guard
  if (!userLoggedIn(queue, userID)) {
    // DEBUG
    // printf("User not logged in, you cannot logout\n");
    res.actionID = 5;
    res.code = 1;
    tlv_t resTLV = {
        .type = 0,
        .length = sizeof(Res),
        .value = (uint8_t *)&res,
    };
    send_tlv(clientFD, &resTLV);
    return;
  }
  int idx = getIndexOfUser(queue, userID);
  pthread_mutex_lock(&queue->lockQueue);
  queue->UserArr[idx].isLoggedIn = false;
  // 오류날 수도 있음
  queue->UserArr[idx].clientID = 0;
  pthread_mutex_unlock(&queue->lockQueue);
  res.actionID = 5;
  res.code = 0;
  tlv_t resTLV = {
      .type = 0,
      .length = sizeof(Res),
      .value = (uint8_t *)&res,
  };
  send_tlv(clientFD, &resTLV);
  // printf("Logout success\n");
}
