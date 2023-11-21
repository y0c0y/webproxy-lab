/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd; // 리슨 파일 설명자, 커넥션 파일 설명자
  char hostname[MAXLINE], port[MAXLINE]; // 호스트 이름, 포트 번호
  socklen_t clientlen; // 클라이언트 길이
  struct sockaddr_storage clientaddr; // 클라이언트 주소

  /* 명령행 인수 확인 */
  if (argc != 2) { // 인수 개수가 2개가 아니라면
    fprintf(stderr, "사용법: %s <포트>\n", argv[0]); // 오류 메시지 출력
    exit(1); // 종료
  }

  listenfd = Open_listenfd(argv[1]); // 리슨 파일 설명자 열기
  while (1) // 무한 루프
  {
    clientlen = sizeof(clientaddr); // 클라이언트 길이 초기화
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 커넥션 파일 설명자 열기
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 호스트 이름, 포트 번호 가져오기
    printf("클라이언트로부터 연결 수락: (%s, %s)\n", hostname, port); // 호스트 이름, 포트 번호 출력
    doit(connfd); // doit 함수 호출
    Close(connfd); // 커넥션 파일 설명자 닫기
  }
}

void doit(int fd) // HTTP 트랜잭션 처리
{
  int is_static; // 정적 파일 여부
  struct stat sbuf; // 파일 정보
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 버퍼, 메서드, URI, 버전
  char filename[MAXLINE], cgiargs[MAXLINE]; // 파일 이름, CGI 인자
  rio_t rio; // rio 구조체

  /* 요청 라인 및 헤더 읽기 */
  Rio_readinitb(&rio, fd); // rio 구조체 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // 클라이언트 파일 기술자에서 한 줄 읽기
  printf("요청 헤더:\n"); // 요청 헤더 출력
  printf("%s", buf); // 버퍼 출력
  sscanf(buf, "%s %s %s", method, uri, version); // 버퍼에서 메서드, URI, 버전 파싱
  if (strcasecmp(method, "GET")) { // 메서드가 GET이 아니면
    clienterror(fd, method, "501", "구현되지 않음", "Tiny는 이 메서드를 구현하지 않습니다");
    return;
  }
  read_requesthdrs(&rio); // 요청 헤더 읽기

  /* GET 요청에서 URI 파싱 */
  is_static = parse_uri(uri, filename, cgiargs); // URI 파싱
  if (stat(filename, &sbuf) < 0) { // 파일 정보 가져오기
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  if (is_static) { /* 정적 콘텐츠 제공 */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 파일이 아니거나 읽기 권한이 없으면
      clienterror(fd, filename, "403", "Forbidden","Tiny couldn't run the CGI program"); // 오류 메시지 전송
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // 정적 파일 제공
  } 
  else
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // 파일이 아니거나 실행 권한이 없으면
      clienterror(fd, filename, "403", "Forbidden","Tiny couldn't run the CGI program"); // 오류 메시지 전송
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // 동적 파일 제공
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) // 오류 메시지를 클라이언트에게 전송
{
  char buf[MAXLINE], body[MAXBUF]; // 버퍼, 본문

  /* HTTP 응답 본문 생성 */
  sprintf(body, "<html><title>Tiny 오류</title>"); // 본문에 문자열 추가
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // 본문에 문자열 추가
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg); // 본문에 문자열 추가
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause); // 본문에 문자열 추가
  sprintf(body, "%s<hr><em>Tiny 웹 서버</em>\r\n", body); // 본문에 문자열 추가

  /* HTTP 응답 전송 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기
  sprintf(buf, "Content-type: text/html\r\n"); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기
  Rio_writen(fd, body, strlen(body)); // 클라이언트 파일 기술자에 쓰기
}

void read_requesthdrs(rio_t *rp) // 요청 헤더 읽고 무시하기
{
  char buf[MAXLINE]; // 버퍼

  Rio_readlineb(rp, buf, MAXLINE); // 클라이언트 파일 기술자에서 한 줄 읽기
  while(strcmp(buf, "\r\n")) // 버퍼가 "\r\n"이 아닌 동안
  {
    Rio_readlineb(rp, buf, MAXLINE); // 클라이언트 파일 기술자에서 한 줄 읽기
    printf("%s", buf); // 버퍼 출력
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) // HTTP URI 파싱
{
  char *ptr; // 포인터

  if (!strstr(uri, "cgi-bin")) // URI에 "cgi-bin"이 없으면
  { /* 정적 콘텐츠 */ 
    strcpy(cgiargs, ""); // CGI 인자를 빈 문자열로 설정
    strcpy(filename, "."); // 파일 이름에 "." 복사
    strcat(filename, uri); // 파일 이름에 URI 추가

   // URI의 마지막 문자가 '/'이면 파일 이름에 "home.html" 추가
   if (uri[strlen(uri)-1] == '/') strcat(filename, "home.html"); 
    return 1; // 정적 파일
  }
  else
  { /* 동적 콘텐츠 */ 
    ptr = index(uri, '?'); // URI에서 '?' 위치 찾기
    if (ptr) // ptr이 NULL이 아니면
    { 
      strcpy(cgiargs, ptr+1); // ptr 이후 문자열을 CGI 인자로 복사
      *ptr = '\0'; // ptr 위치에 NULL 문자 삽입
    }
    else strcpy(cgiargs, ""); // CGI 인자를 빈 문자열로 설정
    
    strcpy(filename, "."); // 파일 이름에 "." 복사
    strcat(filename, uri); // 파일 이름에 URI 추가
    return 0; // 동적 파일
  }
}

void serve_static(int fd, char *filename, int filesize) // 정적 콘텐츠 클라이언트에게 제공
{
  int srcfd; // 파일 설명자
  char *srcp, filetype[MAXLINE], buf[MAXBUF]; // 파일 포인터, 파일 유형, 버퍼

  /* 클라이언트에게 응답 헤더 전송 */
  get_filetype(filename, filetype); // 파일 유형 가져오기
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 버퍼에 문자열 추가
  sprintf(buf, "%sServer: Tiny 웹 서버\r\n", buf); // 버퍼에 문자열 추가
  sprintf(buf, "%sConnection: close\r\n", buf); // 버퍼에 문자열 추가
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize); // 버퍼에 문자열 추가
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기
  printf("응답 헤더:\n"); // 응답 헤더 출력
  printf("%s", buf); // 버퍼 출력

  /* 클라이언트에게 응답 본문 전송 */
  srcfd = Open(filename, O_RDONLY, 0); // 파일 설명자 열기
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일 매핑
  Close(srcfd); // 파일 설명자 닫기
  Rio_writen(fd, srcp, filesize); // 클라이언트 파일 기술자에 쓰기
  Munmap(srcp, filesize); // 파일 매핑 해제
}

void get_filetype(char *filename, char *filetype) // 파일 유형 가져오기
{
  if (strstr(filename, ".html")) // 파일 이름에 ".html" 포함되면
    strcpy(filetype, "text/html"); // 파일 유형을 "text/html"로 설정
  else if (strstr(filename, ".gif")) // 파일 이름에 ".gif" 포함되면
    strcpy(filetype, "image/gif"); // 파일 유형을 "image/gif"로 설정
  else if (strstr(filename, ".jpg")) // 파일 이름에 ".jpg" 포함되면
    strcpy(filetype, "image/jpeg"); // 파일 유형을 "image/jpeg"로 설정
  else
    strcpy(filetype, "text/plain"); // 파일 유형을 "text/plain"으로 설정
}

void serve_dynamic(int fd, char *filename, char *cgiargs) // 동적 콘텐츠 클라이언트에게 제공
{
  char buf[MAXLINE], *emptylist[] = {NULL}; // 버퍼, 빈 문자열 포인터 배열

  /* HTTP 응답 첫 부분 전송 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기
  sprintf(buf, "Server: Tiny 웹 서버\r\n"); // 버퍼에 문자열 추가
  Rio_writen(fd, buf, strlen(buf)); // 클라이언트 파일 기술자에 쓰기

  if (Fork() == 0) { /* 자식 프로세스 */
    /* CGI 프로그램 실행 */
    setenv("QUERY_STRING", cgiargs, 1); // QUERY_STRING 환경 변수 설정
    Dup2(fd, STDOUT_FILENO); // 표준 출력을 클라이언트 파일 기술자로 복제
    Execve(filename, emptylist, environ); // filename 실행
  }
  Wait(NULL); // 자식 프로세스가 종료될 때까지 기다리기
}

