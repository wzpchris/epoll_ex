//
//a simple echo server using epoll in linux
//

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <time.h>
#include <iostream>
using namespace std;

#define MAX_EVENTS 500

struct myevent_s
{
	int fd;
	void (*call_back)(int fd, int events, void *arg);
	int events;
	void *arg;
	int status; //1:in epoll wait list, 0 not in
	char buff[128];
	int len, s_offset;
	long last_active; //last active time
};

// set event
void EventSet(myevent_s *ev, int fd, void (*call_back)(int, int, void*), void *arg)
{
	ev->fd = fd;
	ev->call_back = call_back;
	ev->events = 0;
	ev->arg = arg;
	ev->status = 0;
	bzero(ev->buff, sizeof(ev->buff));
	ev->s_offset = 0;
	ev->len = 0;
	ev->last_active = time(NULL);
}

// add/mod an event to epoll
void EventAdd(int epollFd, int events, myevent_s *ev)
{
	struct epoll_event epv = {0, {0}};
	int op;
	epv.data.ptr = ev;
	epv.events = ev->events = events;
	if(ev->status == 1)
	{
		op = EPOLL_CTL_MOD;
	}else{
		op = EPOLL_CTL_ADD;
		ev->status = 1;
	}

	if(epoll_ctl(epollFd, op, ev->fd, &epv) < 0)
	{
		printf("Event Add failed[fd=%d], events[%d]\n", ev->fd, events);
	}else {
		printf("Event Add OK[fd=%d],op=%d,events[%0X]\n", ev->fd, op, events);
	}
}

// delete an event from epoll
void EventDel(int epollFd, myevent_s *ev)
{
	struct epoll_event epv = {0, {0}};
	if(ev->status != 1) return;
	epv.data.ptr = ev;
	ev->status = 0;
	epoll_ctl(epollFd, EPOLL_CTL_DEL, ev->fd, &epv);
}

int g_epollFd;
myevent_s g_Events[MAX_EVENTS+1]; //g_Events[MAX_EVENTS] is used by listen fd
void RecvData(int fd, int events, void *arg);
void SendData(int fd, int events, void *arg);

// accept new connections from clients
void AcceptConn(int fd, int events, void *arg)
{
	struct sockaddr_in sin;
	socklen_t len = sizeof(struct sockaddr_in);
	int nfd, i;
	//accept
	if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) == -1)
	{
		if(errno != EAGAIN && errno != EINTR)
		{
		}
		
		printf("%s: accept, %d", __func__, errno);
		return;
	}

	do
	{
		for(i = 0; i < MAX_EVENTS; ++i)
		{
			if(g_Events[i].status == 0)
			{
				break;
			}
		}

		if(i == MAX_EVENTS)
		{
			printf("%s:max connection limit[%d].", __func__, MAX_EVENTS);
			break;
		}

		// set nonblocking
		int iret = 0;
		if((iret = fcntl(nfd, F_SETFL, O_NONBLOCK)) < 0)
		{
			printf("%s: fcntl nonblocking failed:%d", __func__, iret);
			break;
		}

		// add a read event for receive data
		EventSet(&g_Events[i], nfd, RecvData, &g_Events[i]);
		EventAdd(g_epollFd, EPOLLIN, &g_Events[i]);
	}while(0);

	printf("new conn[%s:%d][tiem:%d],pos[%d]\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), g_Events[i].last_active, i);
}

// receive data
void RecvData(int fd, int events, void *arg)
{
	struct myevent_s *ev = (struct myevent_s*)arg;
	int len;
	//receive data
	len = recv(fd, ev->buff + ev->len, sizeof(ev->buff) - 1 - ev->len, 0);
	EventDel(g_epollFd, ev);
	if(len > 0)
	{
		ev->len += len;
		ev->buff[len] = '\0';
		printf("C[%d]:%s\n", fd, ev->buff);
		//change to send event
		EventSet(ev, fd, SendData, ev);
		EventAdd(g_epollFd, EPOLLOUT, ev);
	} else if(len == 0) {
		close(ev->fd);
		printf("[fd=%d] pos[%d], closed gracefully.\n", fd, ev-g_Events);
	} else {
		close(ev->fd);
		printf("rev[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));
	}
}

// send data
void SendData(int fd, int events, void *arg)
{
	struct myevent_s *ev = (struct myevent_s*)arg;
	int len;
	// send data
	len = send(fd, ev->buff + ev->s_offset, ev->len - ev->s_offset, 0);
	if(len > 0)
	{
		printf("send[fd=%d], [%d<-->%d]%s\n", fd, len, ev->len, ev->buff);
		ev->s_offset += len;
		if(ev->s_offset == ev->len)
		{
			//change to receive event
			EventDel(g_epollFd, ev);
			EventSet(ev, fd, RecvData, ev);
			EventAdd(g_epollFd, EPOLLIN, ev);
		}
	} else {
		close(ev->fd);
		EventDel(g_epollFd, ev);
		printf("send[fd=%d] error[%d]\n", fd, errno);
	}
}

void InitListenSocket(int epollFd, short port)
{
	int listenFd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(listenFd, F_SETFL, O_NONBLOCK); // set non-blocking
	printf("server listen fd=%d\n", listenFd);
	EventSet(&g_Events[MAX_EVENTS], listenFd, AcceptConn, &g_Events[MAX_EVENTS]);

	//add listen socket
	EventAdd(epollFd, FPOLLIN, &g_Events[MAX_EVENTS]);
	//bind & listen
	sockaddr_in sin;
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_add.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	bind(listenFd, (const sockaddr *)&sin, sizeof(sin));
	listen(listenFd, 5);
}

int main(int argc, char **argv)
{
	unsigned short port = 12345;
	if(argc == 2)
	{
		port = atoi(argv[1]);
	}

	// create epoll
	g_epollFd = epoll_create(MAX_EVENTS);
	
}
