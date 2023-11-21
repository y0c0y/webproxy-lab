/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
void doit(int fd); //클라이이언트 요청을 처리하는 함수의 선언, 파일디스크립터 fd를 인자로 받음
void read_requesthdrs(rio_t *rp); //클라이언트의 HTTP요청 헤더를 읽는 함수 선언
int parse_uri(char *uri, char *filename, char *cgiargs); //URI를 분석하여 파일 이름과 CGI인자로 파싱하는 함수 선언
// 11.11문제 반영 - HEAD 메소드 지원
void serve_static(int fd, char *filename, int filesize, char *method); //정적 콘텐츠를 제공하는 함수의 선언
void get_filetype(char *filename, char *filetype); //파일 이름 기반 파일 타입 결정하는 함수 선언
// 11.11문제 반영 - HEAD 메소드 지원
// void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method); //동적 콘텐츠 제공
//HEAD요청을 처리
//HEAD요청은 GET요청과 유사하지만 HEAD요청의 경우 서버는 요청된 자원의 헤더만을 반환하고 본문은 반환X
//목적 : 서버가 HEAD요청을 받았을 때, 해당 자원에 대한 메타데이터(길이, 수정날짜, MIME타입)만을 클라이언트에게 전송
//결과 : 클라이언트는 본문을 다운로드 하지 않아도 해당 자원에 대한 정보를 얻을 수 있게 됨
//장점 : 네트워크 트래픽 감소, 자원의 유효성 검사 및 메타데이터 검색 효율화, 서버 부하 감소, 검색 엔진 최적화
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg); //클라이언트에서 에러 메세지를 보냄
/*
* tiny는 반복실행 서버. open_listenfd함수 호출하여 listen socket 오픈 후,
   tiny는 전형적인 무한 서버 루프 실행, 반복적으로 연결 요청을 접수하고,
   트랜잭션을 수행하고, 자신 쪽의 연결 끝을 닫음
*/
int main(int argc, char **argv) {
  int listenfd, connfd; //listenfd는 연결된 서버의 소켓, connfd는 연결된 클라이언트의 소켓
  char hostname[MAXLINE], port[MAXLINE];  //hostname과 port는 연결된 클라이언트의 정보를 저장
  socklen_t clientlen;                    // clientlen은 클라이언트 주소의 길이를 저장, socklen_t는 unsigned int 타입
  struct sockaddr_storage clientaddr;     // sockaddr_storage 구조체는 모든 형태의 소켓 주소를 저장하기에 충분함
  //clientaddr은 클라이언트 주소를 저장하는데 사용, 모든 유형의 주소를 저장할 수 있음
  //서버를 시작할 때, 서버는 자신의 IP주소와 포트번호를 이미 알고 있기 때문에 서버 주소는 저장할 필요가 없음
//커맨드 라인 체크
  if (argc != 2) {  //프로그램 실행 시 포트번호가 제공되지 않으면
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1); //프로그램 종료
  }
  listenfd = Open_listenfd(argv[1]); // 리스닝 소켓을 열고, argv[1]에서 제공된 포트 번호에 바인딩
  // 무한 서버 루프 실행
  while (1) {
    clientlen = sizeof(clientaddr); //클라이언트 주소 구조체의 크기 설정
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 클라이언트 연결 수락
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); //클라이언트의 호스트 이름과 포트 번호를 추출
    printf("Accepted connection from (%s, %s)\n", hostname, port); //연결된 클랄이언트 정보 출력
    doit(connfd);   // 클라이언트 요청 처리
    Close(connfd);  // 클라이언트와의 연결 종료
  }
}
/*
* doit 함수: 트랜잭션 처리 함수
   요청라인을 읽고 분석(rio_readlineb함수 사용)
   tiny는 GET method만 지원
*/
void doit(int fd) {
  int is_static; //정적 콘텐츠인지 확인하는 플래그
  struct stat sbuf; //파일 상태 정보를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; //요청 처리에 필요한 버퍼와 문자열
  char filename[MAXLINE], cgiargs[MAXLINE]; //파일 이름과 CGI인자를 저장하는 문자열
  /*
   * Rio Package: 짧은 카운트에서 발생할 수 있는 네트워크 프로그램 같은 응용에서 편리하고,
     안정적이고 효율적인 I/O 제공
  */
  rio_t rio; //로버스트 I/O(Rio) 구조체
  // 클라이언트 요청 읽기
  Rio_readinitb(&rio, fd);// Rio 구조체 초기화, open한 식별자마다 한 번 호출. 함수는 식별자 clientfd를 주소 rio에 위치한 rio_t타입의 읽기 버퍼와 연결
  Rio_readlineb(&rio, buf, MAXLINE);  //클라이언트로부터 한 줄 읽기, 다음 텍스트 줄을 파일 rio(종료 새 줄 문자를 포함)에서 읽고, 이것을 메모리 buf로 복사하고, 텍스트 라인을 NULL(0)문자로 종료시킴
  printf("Request headers: \n");
  printf("%s", buf);  //요청 해더 출력, buf에는 요청헤더가 저장되고 method, uri, version의 정보가 들어가 있음
  sscanf(buf, "%s %s %s", method, uri, version);  // 요청 라인 파싱, sscanf(): buf에서 argumment-list가 제공하는 위치로 데이터 읽음
  //지원하지 않는 메소드 처리
  // GET이 아닌 다른 method 요청시 에러 메시지 보내고, main루틴으로 돌아오고, 연결을 닫고 다음 연결 요청 기다림
  // if (strcasecmp(method, "GET")) {  // strcasecmp함수: 대소문자를 구분하지 않고 두 인자 비교. 같으면 0 리턴
  if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {  // GET 또는 HEAD 메소드가 아닌 경우
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method"); //클라이언트애개 501에러 응답
    return;
  }
  read_requesthdrs(&rio);  // 요청헤더 읽기
  //URI분석
  //요청이 정적 또는 동적 콘텐츠를 위한 것인지 나타내는 플래그 설정
  is_static = parse_uri(uri, filename, cgiargs); // URI를 분석하여 정적/동적 콘텐츠 결정
  if (stat(filename, &sbuf) < 0) { //파일 상태 확인
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file"); // 파일을 찾을 수 없는 경우 404 에러 응답
    return; // 함수 종료
    return;
  }
  // 정적 콘텐츠 요구하는 경우
  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {  // 파일이 읽기 가능한지 확인
      /*
       S_ISREG(mode): mode가 regular file인지 확인
       S_IRUSR: read by owner
      */
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method);  // 읽기 권한 있다면 클라이언트에게 정적 콘텐츠 제공
  }
  // 동적 콘텐츠 요구하는 경우
  else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {  // 파일이 실행 가능한지 확인
      /*
       S_IXUSR: Execute by owner
      */
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);  // 실행 권한 있다면 클라이언트에게 동적 콘텐츠 제공
  }
}
/*
* HTTP 응답을 응답 라인에 적절한 상태 코드와 상태 메시지와 함께 클라이언트에 보내며,
   브라우저 사용자에게 에러를 설명하는 응답 본체에 HTML 파일도 함께 보냄
   (HTML 응답은 본체에서 콘텐츠의 크기와 타입을 나타내야 한다)
*/
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];  // MAXBUF 8192(Max I/O buffer size)
  // Build the HTTP response body
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff""\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>THe Tiny Web server</em>\r\n", body);
  // Print the HTTP response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, buf, strlen(body));
}
// tiny는 요청 헤더 내의 어떤 정보도 사용하지 않음. 이들을 읽고 무시
void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  Rio_readlineb(rp, buf, MAXLINE);    // MAXLINE까지 읽기
  while (strcmp(buf, "\r\n")) {       // EOF(한 줄 전체가 개행문자인 곳) 만날 때 까지 계속 읽기
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}
// URI를 파일 이름과 옵션으로 CGI 인자 스트링 분석
int parse_uri(char *uri, char *filename, char *cgiargs) {
  char * ptr;
  //숙제 11.10 반영 - /adder
  //CGI를 통해 웹서버와 외부 스크립트 또는 프로그램 간의 인터페이스를 제공
  //HTML 페이지를 준비하고 AJAX 요청을 사용하여 서버의 CGI 스크립트에 데이터 전송
  // /adder URI를 동적 콘텐츠 요청으로 처리
  if (strstr(uri, "/adder")) {
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    } else {
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0; // 동적 콘텐츠
  }
  // 요청이 정적 콘텐츠를 위한 것이라면
  else if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");                // CGI 인자 스트링 지우고
    strcpy(filename, ".");              // 상대 리눅스 경로이름으로 변환
    strcat(filename, uri);              // strcat(*str1, *str2): str2를 str1에 연결하고 NULL 문자로 결과 스트링 종료
    if (uri[strlen(uri)-1] == '/') {    // URI가 '/'로 끝난다면
      strcat(filename, "home.html");    // home.html로 파일이름 추가
    }
    return 1;
  }
  // 동적 콘텐츠를 위한 것이라면
  else {
    // 모든 CGI인자 추출
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);   // 물음표 뒤에 인자 붙이기
      *ptr = '\0';              // 포인터는 문자열 마지막으로 위치
    }
    else {
      strcpy(cgiargs, "");
    }
    // 나머지 URI 부분을 상대 리눅스 파일 이름으로 변환
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}
void serve_static(int fd, char *filename, int filesize, char *method) {
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  // Send response headers to client
  get_filetype(filename, filetype);   // 파일 타입 결정
  // 클라이언트에 응답 줄과 응답 헤더 보낸다.
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers: \n");
  printf("%s", buf);
  // 11.11문제 반영 - HEAD 메소드 지원
  if (strcasecmp(method, "HEAD") == 0) {
    return;
  }
  // Send response body to client
  srcfd = Open(filename, O_RDONLY, 0);                          // open함수: filename을 fd로 변환 후, fd번호 리턴, O_RDONLY: Reading only
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);   // mmap함수: 요청한 파일을 가상메모리 영역으로 매핑
  // 11.9 문제 반영
  srcp = malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);                                                 // 파일을 메모리로 매핑 후 더이상 이 식별자 필요X -> 파일 닫기
  Rio_writen(fd, srcp, filesize);                               // 실제로 파일을 클라이언트에게 전송
  // Munmap(srcp, filesize);                                       // 매핑된 가상메모리 주소를 반환(메모리 누수 방지에 중요)
  free(srcp);
}
/*
* get_filetype - derive file type from file name
*/
// 파일 형식 추출하기
void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4")) {
    strcpy(filetype, "video/mp4");
  }
  //11.7반영 ->.mpg파일 처리
   else if (strstr(filename, ".mpg")) {
    strcpy(filetype, "video/mpg");
  }
  else {
    strcpy(filetype, "text/plain");
  }
}
// serve_dynamic함수는 클라이언트에 성공을 알려주는 응답 라인을 보내는 것으로 시작
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method) {
  char buf[MAXLINE], *emptylist[] = {NULL};
  // Return first part of HTTP response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  // 응답의 첫 번째 부분을 보낸 후 새로운 자식 프로세스를 fork
  if (Fork() == 0) {  // Child
  // Real sercer would set all CGI vars here
    setenv("QUERY_STRING", cgiargs, 1);     // QUERY_STRING의 환경변수를 요청 URI의 CGI 인자들로 초기화
    // 11.11문제 반영 - HEAD 메소드 지원
    setenv("REQUEST_METHOD", method, 1);    // REQUEST_METHOD: GET or POST
    Dup2(fd, STDOUT_FILENO);                // 자식은 자식의 표준 출력은 연결 파일 식별자로 재지정
    Execve(filename, emptylist, environ);   // CGI프로그램 로드 후 실행
  }
  Wait(NULL);   // 부모는 자식이 종료되어 정리되는 것을 기다리기 위해 wait함수에 블록됨
}