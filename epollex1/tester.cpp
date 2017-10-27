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

char message[BUF_SIZE];
list<int> list_of_clients;

int res;
clock_t tStart;

int main(int argc, char **arv)
{
	int sock;
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.s_addr = inet_addr(SERVER_HOST);
	
	tStart = clock();
	
	for(int i = 0; i < EPOLL_SIZE; ++i)
	{
		CHK2(sock, socket(PF_INET, SOCK_STREAM, 0));
		CHK(connect(sock, (struct sockaddr *)&addr, sizeof(addr)));

		list_of_clients.push_back(sock);

		bzero(&message, BUF_SIZE);
		CHK2(res, recv(sock, message, BUF_SIZE, 0));
		printf("%s\n", message);		
	}

	list<int>::iterator it;
	for(it = list_of_clients.begin(); it != list_of_clients.end(); ++it)
	{
		close(*it);
	}

	printf("Test passed at:%.2f second(s)\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);
	printf("Total server connections was: %d\n", EPOLL_SIZE);

	return 0;
}
