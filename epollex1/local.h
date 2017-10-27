#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#define BUF_SIZE 1024						//默认缓冲区
#define SERVER_PORT 8888					//监听端口
#define SERVER_HOST "127.0.0.1"				//服务器ip地址
#define EPOLL_RUN_TIMEOUT -1				//epoll的超时时间
#define EPOLL_SIZE 1000					//epoll监听的客户端的最大数目,这里的数字不能超过可打开的最大文件数，可通过ulimit -a查看

#define STR_WELCOME "Welcome to seChat! You ID is:Client #%d"
#define STR_MESSAGE "Client #%d >> %s"
#define STR_NOONE_CONNECTED "Noone connected to server except you!"
#define CMD_EXIT "EXIT"

//两个有用的宏定义:检查和赋值并且检测
#define CHK(eval) if(eval < 0) { perror("eval"); exit(-1); }
#define CHK2(res, eval) if((res = eval) < 0) { perror("eval"); exit(-1); }
#define bzero(src, size) memset(src, 0, size) 

//---------------------
//函数名:	setnonblocking
//
//
//
//----------------------
int setnonblocking(int sockfd);

int handle_message(int new_fd);
