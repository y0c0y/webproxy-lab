/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p; // buf : query string, p : &의 위치
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // arg1, arg2 : query string에서 받은 두 수, content : response body
  int n1=0, n2=0; // n1, n2 : arg1, arg2를 정수로 변환한 값

  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) { // query string을 buf에 저장
    p = strchr(buf, '&'); // buf에서 &의 위치를 찾아 p에 저장
    *p = '\0'; // p가 가리키는 위치에 null 문자 삽입
    strcpy(arg1, buf); // buf에 있는 문자열을 arg1에 복사
    strcpy(arg2, p+1); // p가 가리키는 위치의 다음 문자부터 arg2에 복사
    n1 = atoi(arg1); // arg1을 정수로 변환하여 n1에 저장
    n2 = atoi(arg2); // arg2를 정수로 변환하여 n2에 저장
  }

  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf); // content에 query string 저장
  sprintf(content, "Welcome to add.com: "); // content에 문자열 저장
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content); // content에 문자열 저장
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2); // content에 문자열 저장
  sprintf(content, "%sThanks for visiting!\r\n", content); // content에 문자열 저장

  /* Generate the HTTP response */

  printf("Connection: close\r\n"); // response header에 Connection: close 추가
  printf("Content-length: %d\r\n", (int)strlen(content)); // response header에 Content-length: content의 길이 추가
  printf("Content-type: text/html\r\n\r\n"); // response header에 Content-type: text/html 추가
  printf("%s", content); // response body 출력
  fflush(stdout); // 버퍼 비우기

  exit(0);
}
/* $end adder */
