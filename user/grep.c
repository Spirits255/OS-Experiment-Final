// user/grep.c
// Simple grep.  Only supports ^ . * $ operators.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

char buf[1024];

// 大小写不敏感比较
int strcmp_i(const char *s1, const char *s2) {
  while(*s1 && *s2) {
    char c1 = *s1, c2 = *s2;
    if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
    if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
    if(c1 != c2) return c1 - c2;
    s1++; s2++;
  }
  return *s1 - *s2;
}

// 大小写不敏感的字符串查找
char* strstr_i(char *haystack, char *needle) {
  int nlen = strlen(needle);
  int hlen = strlen(haystack);
  
  for(int i = 0; i <= hlen - nlen; i++) {
    int j;
    for(j = 0; j < nlen; j++) {
      char c1 = haystack[i+j], c2 = needle[j];
      if(c1 >= 'A' && c1 <= 'Z') c1 += 32;
      if(c2 >= 'A' && c2 <= 'Z') c2 += 32;
      if(c1 != c2) break;
    }
    if(j == nlen) return haystack + i;
  }
  return 0;
}

int match(char*, char*);

void
grep(char *pattern, int fd, int ignore_case)
{
  int n, m;
  char *p, *q;

  m = 0;
  while((n = read(fd, buf+m, sizeof(buf)-m-1)) > 0){
    m += n;
    buf[m] = '\0';
    p = buf;
    while((q = strchr(p, '\n')) != 0){
      *q = 0;
      
      int found = 0;
      if(ignore_case) {
        // 大小写不敏感匹配
        if(strstr_i(p, pattern) != 0) {
          found = 1;
        }
      } else {
        // 原有正则匹配
        if(match(pattern, p)) {
          found = 1;
        }
      }
      
      if(found){
        *q = '\n';
        write(1, p, q+1 - p);
      }
      p = q+1;
    }
    if(m > 0){
      m -= p - buf;
      memmove(buf, p, m);
    }
  }
}

int
main(int argc, char *argv[])
{
  int fd, i;
  char *pattern;
  int ignore_case = 0;
  int pattern_index = 1;

  if(argc <= 1){
    fprintf(2, "usage: grep pattern [file ...]\n");
    fprintf(2, "       grep -i pattern [file ...]\n");
    exit(1);
  }

  // 检查-i选项
  if(strcmp(argv[1], "-i") == 0) {
    ignore_case = 1;
    if(argc <= 2) {
      fprintf(2, "usage: grep -i pattern [file ...]\n");
      exit(1);
    }
    pattern = argv[2];
    pattern_index = 3;
  } else {
    pattern = argv[1];
    pattern_index = 2;
  }

  if(argc <= pattern_index){
    grep(pattern, 0, ignore_case);
    exit(0);
  }

  for(i = pattern_index; i < argc; i++){
    if((fd = open(argv[i], O_RDONLY)) < 0){
      printf("grep: cannot open %s\n", argv[i]);
      exit(1);
    }
    grep(pattern, fd, ignore_case);
    close(fd);
  }
  exit(0);
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9, or
// https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html

int matchhere(char*, char*);
int matchstar(int, char*, char*);

int
match(char *re, char *text)
{
  if(re[0] == '^')
    return matchhere(re+1, text);
  do{  // must look at empty string
    if(matchhere(re, text))
      return 1;
  }while(*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text)
{
  if(re[0] == '\0')
    return 1;
  if(re[1] == '*')
    return matchstar(re[0], re+2, text);
  if(re[0] == '$' && re[1] == '\0')
    return *text == '\0';
  if(*text!='\0' && (re[0]=='.' || re[0]==*text))
    return matchhere(re+1, text+1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text)
{
  do{  // a * matches zero or more instances
    if(matchhere(re, text))
      return 1;
  }while(*text!='\0' && (*text++==c || c=='.'));
  return 0;
}