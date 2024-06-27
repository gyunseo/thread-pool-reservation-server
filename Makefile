CC = gcc

TARGET_SERVER := pa3_server
TARGET_CLIENT := pa3_client
TASK_QUEUE := task_queue
USER_QUEUE := user_queue
TLV := tlv
SERVICE := service

SERVER_OBJS = $(TARGET_SERVER).o $(TASK_QUEUE).o $(USER_QUEUE).o $(SERVICE).o 
CLIENT_OBJS = $(TARGET_CLIENT).o
COMMON_OBJS = $(TLV).o

SERVER_HEADERS = $(TASK_QUEUE).h $(USER_QUEUE).h $(TLV).h $(SERVICE).h 
CLIENT_HEADERS = $(TLV).h
CCFLAGS = -Wextra -Werror -D_GNU_SOURCE
LIBS = -pthread -largon2 -lreadline

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(SERVER_OBJS) $(COMMON_OBJS) $(SERVER_HEADERS)
	$(CC) $^ -o $@ $(LIBS) 

$(TARGET_CLIENT): $(CLIENT_OBJS) $(COMMON_OBJS) $(CLIENT_HEADERS)
	$(CC) $^ -o $@ $(LIBS) 

%.o: %.c
	$(CC) $(CCFLAGS) -g -c $< -o $@ $(LIBS) 

.PHONY: clean
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT) $(SERVER_OBJS) $(CLIENT_OBJS) $(COMMON_OBJS)