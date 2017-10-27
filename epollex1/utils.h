#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int setnonblocking(int sockfd)
{
	CHK(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK));
	return 0;
}
