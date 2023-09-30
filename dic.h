#ifndef __DIC_H__
#define __DIC_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <termios.h>
#include <pthread.h>
#include <sqlite3.h>

#define IP "192.168.21.84  "
#define PORT 6000

#define R 0 // 注册请求
#define L 1 // 登录请求
#define Y 2 // 请求确认包
#define E 3 // 错误包
#define D 4 // 单词查询包
#define S 5 // 历史记录查询包

#define Q 6 // 退出报
#define C 7 // 重复报


#define N 128

// 打印错误提示
#define ERR_MSG(msg) do{\
    fprintf(stderr, "__%d__:", __LINE__);\
    perror("msg");\
}while(0)

// 用户
struct  user
{
    char name[N];   // 用户名
    char pwd[N];    // 密码
};  

// 进程间传递消息结构体
struct msg 
{
	int newfd;
	struct sockaddr_in cin;
};


#endif
