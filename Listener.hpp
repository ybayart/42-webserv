#ifndef LISTENER_HPP
#define LISTENER_HPP

#include <iostream>
#include <vector>
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
#include <map>
#include "Handler.hpp"
#include "Config.hpp"
#include "Client.hpp"

#define TIMEOUT 30

class Listener
{
	private:
		int						_fd;
		int						_maxFd;
		struct sockaddr_in		_info;
		fd_set					_readSet;
		fd_set					_writeSet;
		fd_set					_rSet;
		fd_set					_wSet;
		Handler					_handler;
		Config					_conf;

	public:
		std::map<int, Client*>		_clients;
		
		Listener();
		~Listener();

		int		getMaxFd() const;
		int		config(char *file);
		void	init();
		void	select();
		void	handleRequest(int fd);

	private:
		void	acceptConnection();
		void	readRequest(Client *client);
		void	readBody(Client *client);
		void	writeResponse(Client *client);
		int		getTimeDiff(std::string start);

};

#endif