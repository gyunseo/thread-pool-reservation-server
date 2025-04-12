## PA3 Thread Pool Reservation Server That Handles Many Clients

#### 2019311801 Gyunseo Lee (이균서)

## Prerequities

```zsh
sudo apt-get -y install libargon2-dev libreadline-dev
```

## 0. Project Hierarchy

```zsh
├── Makefile
├── pa3_client
├── pa3_client.c
├── pa3_client.o
├── pa3_server
├── pa3_server.c
├── pa3_server.o
├── service.c
├── service.h
├── service.o
├── task_queue.c
├── task_queue.h
├── task_queue.o
├── tlv.c
├── tlv.h
├── tlv.o
├── user_queue.c
├── user_queue.h
└── user_queue.o
```

## 1. How to Build

```zsh
make
```

## 2. How to Run

```zsh
./pa3_server 11398
```

On another terminal,

```zsh
./pa3_client localhost 11398
```

or

```zsh
./pa3_client localhost 11398 [filename]
```

## 3. Implementation

### 3.1. Server

- Producer는 Client로부터의 연결 요청, 이를 Task Queue에 넣는다.
- Consumer는 Task Queue에서 Task를 가져와서 처리한다. Consumer는 최대 Thread 개수만큼 동시에 동작한다.
- Task는 Client로부터 받은 데이터를 처리하는 것을 의미한다. (여기서는 client로부터 연결 및 예약 시스템 쿼리를 받아 처리한다.)



### 3.2. Client

- Client는 Server에 연결하여 데이터를 보낸다.
- Client REPL형태로 동작한다.
- 동시에 Client는 파일을 읽어서 Server에 보낼 수 있다. (파일명을 인자로 받는 경우)
- Client는 Server로부터 받은 데이터를 출력한다. (Response Code에 따라 Success 메시지와 Error 메시지를 출력한다.)

## 4. Test

| Action ID | Name              | On success                               | On failure                                |
|-----------|-------------------|------------------------------------------|-------------------------------------------|
| 0         | Termination        | Connection terminated.                   | Failed to disconnect as ERROR_MSG.        |
| 1         | Log in             | Logged in successfully.                  | Failed to log in as ERROR_MSG.            |
| 2         | Book               | Booked seat SEAT_NUMBER.                 | Failed to book as ERROR_MSG.              |
| 3         | Confirm booking    | "Booked the seats seats1, seats2, etc.." | Failed to confirm booking as ERROR_MSG.   |
| 4         | Cancel booking     | Canceled seat SEAT_NUMBER.              | Failed to cancel booking as ERROR_MSG.    |
| 5         | Log out            | Logged out successfully.                 | Failed to log out as ERROR_MSG.           |
| <Other>   | Unknown action     | -                                        | Action <Other> is unknown.                |

`test.txt`:

```plaintxt
5 2 12
3 5
1 3
7 1 password
1 3
7 2 12
7 2 1025
7 10 5
7 5 0
3 1 hunter2
3 2 12
3 2 21
3 2 22
3 3
3 5
7 1 abc
```

On sserver terminal,

```zsh
./pa3_server 11398
```

On another terminal(client terminal),

```zsh
./pa3_client 127.0.0.1 11398 test.txt
```

```zsh
Failed to book as user is not logged in.
Failed to logout as user is not logged in.
Failed to confirm booking as user is not logged in.
Logged in successfully.
Failed to confirm booking as user is not logged in.
Booked seat 12.
Failed to book as seat number is out of range.
Action 10 is unknown
Logged out successfully.
Logged in successfully.
Failed to book as seat is unavailable.
Booked seat 21.
Booked seat 22.
Booked seats 21, 22.
Logged out successfully.
Failed to login as password is incorrect.
Connection terminated.
```

`./load_test.sh`를 실행하여 최대 256명의 유저를 핸들링하는 시나리오를 재현할 수 있다.  