#include "Server.hpp"
#include "Config.hpp"
#include "Logger.hpp"

std::vector<Server>		g_servers;
Logger					g_logger(1, "console", MED);

int		ret_error(std::string error)
{
	std::cout << error << std::endl;
	return (1);
}

int 	main(int ac, char **av)
{
	Config					config;
	Client					*client;

	fd_set					readSet;
	fd_set					writeSet;
	fd_set					rSet;
	fd_set					wSet;
	struct timeval			timeout;

	if (ac != 2)
		return (ret_error("Usage: ./webserv config-file"));
	else
		if (!config.parse(av[1], g_servers))
			return (ret_error("Error: wrong syntax in config file"));

	signal(SIGINT, Config::exit);

	FD_ZERO(&rSet);
	FD_ZERO(&wSet);
	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	for (std::vector<Server>::iterator it(g_servers.begin()); it != g_servers.end(); ++it)
		it->init(&readSet, &writeSet, &rSet, &wSet);

	while (1)
	{
		readSet = rSet;
		writeSet = wSet;
		select(config.getMaxFd(g_servers) + 1, &readSet, &writeSet, NULL, &timeout);
		for (std::vector<Server>::iterator s(g_servers.begin()); s != g_servers.end(); ++s)
		{
			if (FD_ISSET(s->getFd(), &readSet))
			{
				if (config.getOpenFd(g_servers) > MAX_FD)
					s->refuseConnection();
				else
					s->acceptConnection();
			}
			for (std::vector<Client*>::iterator c(s->_clients.begin()); c != s->_clients.end(); ++c)
			{
				client = *c;
				if (FD_ISSET(client->fd, &readSet))
					if (!s->readRequest(c))
						break ;
				if (FD_ISSET(client->fd, &writeSet))
					if (!s->writeResponse(c))
						break ;
				// if (FD_ISSET(client->file_fd, &readSet))
				// 	client->readFile();
				// if (FD_ISSET(client->file_fd, &writeSet))
				// 	client->writeFile();
			}	
		}
	}
	return(0);
}