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
		std::cout << "helo\n";
		listener.select();
		std::cout << "helofdsfq\n";
		for (std::vector<Client>::iterator it(listener._clients.begin()); it != listener._clients.end(); ++it)
			listener.handleRequest(it);
	}
	return(0);
}