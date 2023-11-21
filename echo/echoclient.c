#include "csapp.h"

int main(int argc, char**argv)
{
  int clientfd; // 클라이언트 파일 설명자
  char *host, *port, buf[MAXLINE]; // 호스트, 포트 번호, 버퍼 배열
  rio_t rio; // rio 구조체

  if (argc != 3) { // 인자가 3개가 아니면
    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]); // 에러 메시지 출력
    exit(0); // 종료
  }

  host = argv[1]; // 호스트
  port = argv[2]; // 포트 번호

  clientfd = Open_clientfd(host, port); // 클라이언트 파일 설명자 생성

  Rio_readinitb(&rio, clientfd); // rio 구조체 초기화

  while (Fgets(buf, MAXLINE, stdin) != NULL) { // 표준 입력에서 한 줄 읽어오기
    Rio_writen(clientfd, buf, strlen(buf)); // 클라이언트 파일 설명자에 쓰기
    Rio_readlineb(&rio, buf, MAXLINE); // 클라이언트 파일 설명자에서 한 줄 읽어오기
    Fputs(buf, stdout); // 표준 출력에 쓰기
  }

  Close(clientfd); // 클라이언트 파일 설명자 닫기
  exit(0); // 종료
}