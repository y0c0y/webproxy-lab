#include "csapp.h"

void echo(int connfd); // echo 함수 선언

int main(int argc, char **argv)
{
  int listenfd, connfd; // 리슨 파일 설명자, 커넥션 파일 설명자
  socklen_t clientlen; // 클라이언트 길이
  struct sockaddr_storage clientaddr; // 클라이언트 주소
  char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트 이름, 클라이언트 포트 번호

  if(argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // 에러 메시지 출력
    exit(0); // 종료
  }

  listenfd = Open_listenfd(argv[1]); // 리슨 파일 설명자 생성
  while(1)
  {
    clientlen = sizeof(struct sockaddr_storage); // 클라이언트 길이 초기화
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 커넥션 파일 설명자 생성
    Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트 호스트 이름, 클라이언트 포트 번호 가져오기
    printf("Connected to (%s, %s)\n", client_hostname, client_port); // 클라이언트 호스트 이름, 클라이언트 포트 번호 출력
    echo(connfd); // echo 함수 호출
    Close(connfd); // 커넥션 파일 설명자 닫기
  }
  exit(0); // 종료

}

void echo(int connfd)
{
  size_t n;
  char buf[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, connfd); // rio 구조체 초기화
  while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) // 클라이언트 파일 설명자에서 한 줄 읽어오기
  {
    printf("server received %d bytes\n", (int)n); // 받은 바이트 수 출력
    Rio_writen(connfd, buf, n); // 클라이언트 파일 설명자에 쓰기
  }
}