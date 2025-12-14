// user/stat.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void
print_stat(char *path, int follow_links)
{
  struct stat st;
  int fd;
  char target[128];
  int is_symlink = 0;
  int link_result = 0;



  if(follow_links) {
    // 跟随符号链接 - 获取目标文件的信息
    if((fd = open(path, O_RDONLY)) < 0) {
      fprintf(2, "stat: cannot open %s\n", path);
      return;
    }
    if(fstat(fd, &st) < 0) {
      fprintf(2, "stat: cannot stat %s\n", path);
      close(fd);
      return;
    }
    close(fd);
    
    // 跟随模式：显示目标文件的信息
    const char* type_str;
    switch(st.type) {
      case T_DIR:    type_str = "directory"; break;
      case T_FILE:   type_str = "regular file"; break;
      case T_DEVICE: type_str = "device"; break;
      case T_SYMLINK:type_str = "symbolic link"; break;
      default:       type_str = "unknown"; break;
    }
    
    printf("  File: %s\n", path);
    printf("  Type: %s\n", type_str);
    link_result = readlink(path, target, sizeof(target) - 1);
    if(link_result > 0) {
      is_symlink = 1;
      target[link_result] = '\0';
    }
    if(is_symlink) {
      printf("  Note: accessed via symlink -> %s\n", target);
    }
    printf("  Inode: %d\n", st.ino);
    printf("  Size: %d bytes\n", (int)st.size);
    printf("  Links: %d\n", st.nlink);
    printf("  Device: %d/%d\n", st.dev >> 8, st.dev & 0xFF);
    
  } else {
    if(lstat(path, &st) < 0) {
      fprintf(2, "stat: cannot stat %s\n", path);
      return;
    }
    link_result = readlink(path, target, sizeof(target) - 1);
    if(link_result > 0) {
      is_symlink = 1;
      target[link_result] = '\0';
    }
    
    if(is_symlink) {
      // 不跟随模式：显示符号链接本身的信息
      printf("  File: %s\n", path);
      printf("  Type: symbolic link\n");
      printf("  Link: %s -> %s\n", path, target);
      printf("  Inode: %d\n", st.ino);  // 符号链接本身的 inode
      printf("  Size: %d bytes\n", (int)st.size);  // 目标路径字符串的长度
      printf("  Links: %d\n", st.nlink);
      printf("  Device: %d/%d\n", st.dev >> 8, st.dev & 0xFF);
      
      // 额外显示目标文件信息（可选）
      struct stat target_st;
      if(stat(target, &target_st) >= 0) {
        const char* target_type_str;
        switch(target_st.type) {
          case T_DIR:    target_type_str = "directory"; break;
          case T_FILE:   target_type_str = "regular file"; break;
          case T_DEVICE: target_type_str = "device"; break;
          case T_SYMLINK:target_type_str = "symbolic link"; break;
          default:       target_type_str = "unknown"; break;
        }
        printf("  Target Type: %s\n", target_type_str);
        printf("  Target Inode: %d\n", target_st.ino);  // 目标文件的 inode
        printf("  Target Size: %d bytes\n", (int)target_st.size);
        printf("  Target Links: %d\n", target_st.nlink);
      } else {
        printf("  Target: [invalid or inaccessible]\n");
      }
    } else {
      // 不是符号链接，正常显示文件信息
      const char* type_str;
      switch(st.type) {
        case T_DIR:    type_str = "directory"; break;
        case T_FILE:   type_str = "regular file"; break;
        case T_DEVICE: type_str = "device"; break;
        default:       type_str = "unknown"; break;
      }
      printf("  File: %s\n", path);
      printf("  Type: %s\n", type_str);
      printf("  Inode: %d\n", st.ino);
      printf("  Size: %d bytes\n", (int)st.size);
      printf("  Links: %d\n", st.nlink);
      printf("  Device: %d/%d\n", st.dev >> 8, st.dev & 0xFF);
    }
  }
}

int
main(int argc, char *argv[])
{
  int follow_links = 1;
  int i = 1;

  if(argc < 2) {
    fprintf(2, "Usage: stat file...\n");
    fprintf(2, "       stat -L file...  (do not follow symlinks)\n");
    exit(1);
  }

  if(strcmp(argv[1], "-L") == 0) {
    follow_links = 0;
    i = 2;
  }

  if(i >= argc) {
    fprintf(2, "stat: missing file operand\n");
    exit(1);
  }

  for(; i < argc; i++) {
    print_stat(argv[i], follow_links);
    if(i < argc - 1) printf("\n");
  }

  exit(0);
}