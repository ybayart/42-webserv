#include "Server.hpp"
#include "Config.hpp"
#include "Logger.hpp"

std::vector<Server>		g_servers;
Logger					g_logger(1, "console", LOW);
bool					g_state = true;

int		ret_error(std::string error)
{
	std::cerr << error << std::endl;
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
	try
	{
		config.parse(av[1], g_servers);
		config.init(&rSet, &wSet, &readSet, &writeSet, &timeout);
	}
	catch (std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}

	while (g_state)
	{
		readSet = rSet;
		writeSet = wSet;
		select(config.getMaxFd(g_servers) + 1, &readSet, &writeSet, NULL, &timeout);
		for (std::vector<Server>::iterator s(g_servers.begin()); s != g_servers.end(); ++s)
		{
			if (FD_ISSET(s->getFd(), &readSet))
			{
				try
				{
					if (!g_state)
						break ;
					if (config.getOpenFd(g_servers) > MAX_FD)
						s->refuseConnection();
					else
						s->acceptConnection();
				}
				catch (std::exception &e)
				{
					std::cerr << "Error: " << e.what() << std::endl;
				}
			}
			if (!s->_tmp_clients.empty())
				if (FD_ISSET(s->_tmp_clients.front(), &writeSet))
					s->send503(s->_tmp_clients.front());
			for (std::vector<Client*>::iterator c(s->_clients.begin()); c != s->_clients.end(); ++c)
			{
				client = *c;
				if (FD_ISSET(client->fd, &readSet))
					if (!s->readRequest(c))
						break ;
				if (FD_ISSET(client->fd, &writeSet))
					if (!s->writeResponse(c))
						break ;
				if (client->write_fd != -1)
					if (FD_ISSET(client->write_fd, &writeSet))
						client->writeFile();
				if (client->read_fd != -1)
					if (FD_ISSET(client->read_fd, &readSet))
						client->readFile();
			}	
		}
	}
	g_servers.clear();
	return(0);
}
