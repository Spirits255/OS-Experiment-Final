// user/find.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--);
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  buf[DIRSIZ] = 0;
  return buf;
}

void
find(char *path, char *name, int type)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
  case T_DEVICE:
    if(name == 0 || strcmp(fmtname(path), name) == 0){
      if(type == 0 || st.type == type)
        printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      // Skip "." and ".."
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;

      // Check if current file matches criteria
      if(name == 0 || strcmp(de.name, name) == 0){
        if(type == 0 || st.type == type)
          printf("%s\n", buf);
      }

      // Recursively search directories
      if(st.type == T_DIR){
        find(buf, name, type);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  char *name = 0;
  int type = 0;
  int i;

  if(argc < 2){
    fprintf(2, "Usage: find [path] [options]\n");
    fprintf(2, "Options:\n");
    fprintf(2, "  -name filename  Search by name\n");
    fprintf(2, "  -type f|d       Search by type (file, directory)\n");
    exit(1);
  }

  // Parse arguments
  i = 1;
  if(argv[1][0] != '-'){
    path = argv[1];
    i = 2;
  }

  for(; i < argc; i++){
    if(strcmp(argv[i], "-name") == 0){
      if(++i >= argc){
        fprintf(2, "find: missing argument to -name\n");
        exit(1);
      }
      name = argv[i];
    } else if(strcmp(argv[i], "-type") == 0){
      if(++i >= argc){
        fprintf(2, "find: missing argument to -type\n");
        exit(1);
      }
      if(strcmp(argv[i], "f") == 0) type = T_FILE;
      else if(strcmp(argv[i], "d") == 0) type = T_DIR;
      else {
        fprintf(2, "find: unknown type %s\n", argv[i]);
        exit(1);
      }
    } else {
      fprintf(2, "find: unknown option %s\n", argv[i]);
      exit(1);
    }
  }

  find(path, name, type);
  exit(0);
}