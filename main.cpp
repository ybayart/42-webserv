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
#include "Config.hpp"

int	ret_error(std::string error)
{
	std::cout << error << std::endl;
	return (1);
}

int main(int ac, char **av)
{
	Listener	listener;

	if (ac != 2)
		return (ret_error("Usage: ./webserv config-file"));
	else
		if (listener.config(av[1]) == -1)
			return (ret_error("Error: wrong syntax in config file"));
	listener.init();
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