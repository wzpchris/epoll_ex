#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "local.h"
#include "utils.h"
#include <time.h>
#include <list>
#include <iostream>
using namespace std;

//存放客户端socket描述符的list
list<int> clients_list;

int main(int argc, char **argv)
{
	int listener;
	struct sockaddr_in addr, their_addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = inet_addr(SERVER_HOST);
	socklen_t socklen;
	socklen = sizeof(struct sockaddr_in);
	
	static struct epoll_event ev, events[EPOLL_SIZE];
	
	char message[BUF_SIZE];
	int epfd;
	clock_t tStart;
	
	int client, res, epoll_events_count;
	CHK2(listener, socket(PF_INET, SOCK_STREAM, 0));
	setnonblocking(listener);
	
	CHK(bind(listener, (struct sockaddr *)&addr, sizeof(addr)));
	CHK(listen(listener, 1));

	CHK2(epfd, epoll_create(EPOLL_SIZE));

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listener;
	//将监听socket加入epoll
	CHK(epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev));

	while(1)
	{
		CHK2(epoll_events_count, epoll_wait(epfd, events, EPOLL_SIZE, EPOLL_RUN_TIMEOUT));	
		tStart = clock();
		
		for(int i = 0; i < epoll_events_count; ++i)
		{
			//新的连接到来,将连接添加到epoll中,并发送欢迎消息
			if(events[i].data.fd == listener)
			{
				CHK2(client, accept(listener, (struct sockaddr *)&their_addr, &socklen));
				setnonblocking(client);
				ev.data.fd = client;
				CHK(epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev));
				
				clients_list.push_back(client);		
			
				bzero(message, BUF_SIZE);
				res = sprintf(message, STR_WELCOME, client);
				CHK2(res, send(client, message, BUF_SIZE, 0));
			}
			else 
			{
				CHK2(res, handle_message(events[i].data.fd));
			}
		}
		printf("Statistics: %d events handled at %.2f second(s)\n", epoll_events_count, (double)(clock() - tStart) / CLOCKS_PER_SEC);
	}
	
	close(listener);
	close(epfd);

	return 0;
}

int handle_message(int client)
{
	char buf[BUF_SIZE], message[BUF_SIZE];
	bzero(buf, BUF_SIZE);
	bzero(message, BUF_SIZE);

	int len;
	//接受客户端消息
	CHK2(len, recv(client, buf, BUF_SIZE, 0));
	if(len == 0)
	{
		CHK(close(client));
		clients_list.remove(client);
	} else {
		if(clients_list.size() == 1)
		{
			CHK(send(client, STR_NOONE_CONNECTED, strlen(STR_NOONE_CONNECTED), 0));
			return len;
		}

		sprintf(message, STR_MESSAGE, client, buf);
		list<int>::iterator it;
		for(it = clients_list.begin(); it != clients_list.end(); ++it)
		{
			if(*it != client)
			{
				CHK(send(*it, message, BUF_SIZE, 0));
			}
		}
	}

	return len;
}
