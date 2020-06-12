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
#include "Client.hpp"

int	ret_error(std::string error)
{
	std::cout << error << std::endl;
	return (1);
}

int main(int ac, char **av)
{
	Config					config;
	std::vector<Server>		servers;
	Client					*client;

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
		for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
		{
			if (FD_ISSET(it->getFd(), &readSet))
			{
				if (config.getOpenFd(servers) > MAX_FD)
					it->refuseConnection();
				else
					it->acceptConnection();
			}
			for (std::vector<Client*>::iterator it2(it->_clients.begin()); it2 != it->_clients.end(); ++it2)
			{
				client = *it2;
				if (FD_ISSET(client->getFd(), &readSet))
					if (!it->readRequest(it2))
						break ;
				if (FD_ISSET(client->getFd(), &writeSet))
					if (!it->writeResponse(it2))
						break ;
			}	
		}
	}
	return(0);
}