#include "tlv.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdio.h>
#include <sys/socket.h>

int client_fd, port;
bool isLoggedIn = false;
int loggedInUserID = -1;
bool isExit = false;

void handleRecvTLV(int actionID, uint32_t userID, int seatNumber) {
  // 서버의 응답 대기 및 수신
  tlv_t resTLV;
  bool hasBooked = false;
  if (recv_tlv(client_fd, &resTLV) == 0) {
    // 응답 처리
    // printf("Received response - Type: %u, Length: %u\n", resTLV.type,
    //        resTLV.length);
    // 여기서 resTLV.value를 사용하여 응답 처리 로직 추가
    Res *res = (Res *)resTLV.value;
    // printf("Action ID: %d, Code: %d\n", res->actionID, res->code);
    for (int i = 1; i <= 256; i++) {
      if (res->seats[i]) {
        hasBooked = true;
        break;
      }
    }
    char *ERROR_MSG = NULL;
    switch (actionID) {
    case 0:
      if (res->code == 0)
        isExit = true;
      else if (res->code == 1) {
        ERROR_MSG = "arguments are invalid";
      }

      if (ERROR_MSG)
        fprintf(stderr, "Failed to disconnect as %s.\n", ERROR_MSG);
      else
        fprintf(stdout, "Connection terminated.\n");
      break;
    case 1:
      if (res->code == 0) {
        isLoggedIn = true;
        loggedInUserID = userID;
      } else if (res->code == 1) {
        ERROR_MSG = "user is active";
      } else if (res->code == 2) {
        ERROR_MSG = "client is active";
      } else if (res->code == 3) {
        ERROR_MSG = "password is incorrect";
      }

      if (ERROR_MSG)
        fprintf(stderr, "Failed to login as %s.\n", ERROR_MSG);
      else
        fprintf(stdout, "Logged in successfully.\n");
      break;
    case 2:
      if (res->code == 0) {
        ;
      } else if (res->code == 1) {
        ERROR_MSG = "user is not logged in";
      } else if (res->code == 2) {
        ERROR_MSG = "seat is unavailable";
      } else if (res->code == 3) {
        ERROR_MSG = "seat number is out of range";
      }
      if (ERROR_MSG)
        fprintf(stderr, "Failed to book as %s.\n", ERROR_MSG);
      else
        fprintf(stdout, "Booked seat %d.\n", seatNumber);
      break;
    case 3:
      if (res->code == 0) {
        if (hasBooked) {
          fprintf(stdout, "Booked seats ");
          bool isFirst = true;
          for (int i = 1; i <= 256; i++) {
            if (res->seats[i]) {
              if (isFirst) {
                fprintf(stdout, "%d", i);
                isFirst = false;
              } else
                fprintf(stdout, ", %d", i);
            }
          }
          fprintf(stdout, ".\n");
        } else
          fprintf(stdout, "Did not book any seats.\n");
      } else if (res->code == 1) {
        ERROR_MSG = "user is not logged in";
        fprintf(stderr, "Failed to confirm booking as %s.\n", ERROR_MSG);
      }

      break;
    case 4:
      if (res->code == 0) {
        ;
      } else if (res->code == 1) {
        ERROR_MSG = "user is not logged in";
      } else if (res->code == 2) {
        ERROR_MSG = "user did not book the specified seat";
      } else if (res->code == 3) {
        ERROR_MSG = "seat number is out of range";
      }
      if (ERROR_MSG)
        fprintf(stderr, "Failed to cancel booking as %s.\n", ERROR_MSG);
      else
        fprintf(stdout, "Canceled seat %d.\n", seatNumber);
      break;
    case 5:
      if (res->code == 0) {
        isLoggedIn = false;
        loggedInUserID = -1;
      } else if (res->code == 1) {
        ERROR_MSG = "user is not logged in";
      }
      if (ERROR_MSG)
        fprintf(stderr, "Failed to logout as %s.\n", ERROR_MSG);
      else
        fprintf(stdout, "Logged out successfully.\n");
      break;
    default:
      fprintf(stderr, "Action %d is unknown\n", actionID);
      break;
    }
    free(resTLV.value);

  } else {
    fprintf(stderr, "Failed to receive response from server.\n");
  }
}

void processInput(char *line) {
  if (strlen(line) > 0) {
    add_history(line);

    uint32_t userID = 0;
    int idx = 0, actionID = -1, seatNumber = -1;
    char *password = NULL;
    char *ptr = strtok(line, " ");

    while (ptr != NULL) {
      if (idx == 0) {
        userID = atoi(ptr);
      } else if (idx == 1) {
        actionID = atoi(ptr);
      } else if (idx == 2 && (actionID == 0 || actionID == 2 || actionID == 3 ||
                              actionID == 4 || actionID == 5)) {
        seatNumber = atoi(ptr);
      } else if (idx == 2 && actionID == 1) {
        // make new string and copy
        password = strdup(ptr); // 문자열 복사
        // printf("password: %s\n", password);
      } else {
        ;
        // fprintf(stderr, "Invalid input\n");
      }
      ptr = strtok(NULL, " "); // 다음 문자열을 잘라서 포인터를 반환
      idx++;
    }
    size_t password_len =
        password ? strlen(password) + 1 : 0; // 널 문자 포함 길이
    size_t query_len = sizeof(Query) - sizeof(char *) + password_len;
    uint8_t *query_buffer = malloc(query_len);
    Query query = {
        .userID = userID,
        .actionID = actionID,
        .seatNumber = seatNumber,
        .password = NULL,
    };
    if (query.actionID == 0 && isLoggedIn) {
      Query prefixQuery = {
          .userID = loggedInUserID,
          .actionID = 5,
          .seatNumber = 0,
          .password = NULL,
      };
      size_t prefix_password_len = 0;
      size_t prefix_query_len = sizeof(Query) - sizeof(char *) + password_len;
      uint8_t *prefix_query_buffer = malloc(prefix_query_len);
      memcpy(prefix_query_buffer, &prefixQuery, sizeof(Query) - sizeof(char *));
      tlv_t tlv = {
          .type = 0,
          .length = prefix_query_len,
          .value = prefix_query_buffer,
      };
      send_tlv(client_fd, &tlv);
      free(prefix_query_buffer);
      handleRecvTLV(5, loggedInUserID, 0);
    }
    memcpy(query_buffer, &query, sizeof(Query) - sizeof(char *));
    if (password) {
      memcpy(query_buffer + sizeof(Query) - sizeof(char *), password,
             password_len);
    }
    tlv_t tlv = {
        .type = 0,
        .length = query_len,
        .value = query_buffer,
    };
    send_tlv(client_fd, &tlv);
    free(query_buffer);
    if (password)
      free(password);

    handleRecvTLV(actionID, userID, seatNumber);
    free(line);
  }
}

void processFile(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("fopen");
    exit(1);
  }
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  while ((read = getline(&line, &len, file)) != -1) {
    if (line[read - 1] == '\n') {
      line[read - 1] = '\0'; // Remove newline character
    }
    processInput(strdup(line)); // Duplicate line for processing
  }
  free(line);
  fclose(file);
  // Send termination request to server
  processInput(strdup("0 0 0"));
}

int main(int argc, char *argv[]) {
  if (argc != 3 && argc != 4) {
    fprintf(stderr, "Usage: %s ip_address port [file]\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[2]);
  struct sockaddr_in server_addr;
  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(argv[1]); // Use provided IP address
  if (connect(client_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    perror("connect");
    exit(1);
  }

  if (argc == 4) {
    processFile(argv[3]); // Process the provided file
  } else {
    char *line;
    while ((line = readline("")) != NULL) {
      processInput(line);
      if (isExit)
        break;
    }
  }

  close(client_fd);
  return 0;
}