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
#include "Server.hpp"
#include "Config.hpp"

int	ret_error(std::string error)
{
	std::cout << error << std::endl;
	return (1);
}

int main(int ac, char **av)
{
	Config					config;
	std::vector<Server>		servers;

	fd_set					readSet;
	fd_set					writeSet;
	fd_set					rSet;
	fd_set					wSet;

	if (ac != 2)
		return (ret_error("Usage: ./webserv config-file"));
	else
		if (!config.parse(av[1], servers))
			return (ret_error("Error: wrong syntax in config file"));

	FD_ZERO(&rSet);
	FD_ZERO(&wSet);
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
		it->init(&readSet, &writeSet, &rSet, &wSet);

	while (1)
	{
		readSet = rSet;
		writeSet = wSet;
		select(config.getMaxFd(servers) + 1, &readSet, &writeSet, NULL, NULL);
		for (int fd = 0; fd <= config.getMaxFd(servers); ++fd)
		{
			for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
			{
				if (FD_ISSET(fd, &readSet))
				{
					if (fd == it->getFd())
						it->acceptConnection();
					else if (it->checkIfClient(fd))
						it->readRequest(fd);
				}
				if (FD_ISSET(fd, &writeSet))
				{
					if (it->checkIfClient(fd))
						it->writeResponse(fd);
				}
			}
		}
	}
	return(0);
}