#!/bin/bash

# 서버 주소와 포트 설정
SERVER="localhost"
PORT="11398"

# 256번의 요청을 보내는 루프
for i in {1..256}
do
    echo "Sending request #$i to $SERVER:$PORT"
    ./pa3_client $SERVER $PORT test.txt &  # 백그라운드에서 클라이언트 실행
    sleep 0.1  # 요청 간의 간격을 0.1초로 설정
done

# 백그라운드에서 실행된 프로세스들이 모두 종료될 때까지 기다림
wait
echo "Load test completed. All requests sent."