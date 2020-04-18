#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include "Listener.hpp"

int main()
{
	Listener	listener;

	while (1)
	{
		listener.select();
		for (int i = 0; i <= listener.getMaxFd(); ++i)
		{
			listener.getRequest(i);
			listener.sendResponse(i);
		}
	}
	return(0);
}