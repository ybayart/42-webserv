#ifndef LISTENER_HPP
#define LISTENER_HPP

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
#include <map>
#include "Handler.hpp"

class Listener
{
	private:
		int					_fd;
		int					_maxFd;
		struct sockaddr_in	_info;
		fd_set				_readSet;
		fd_set				_writeSet;
		fd_set				_rSet;
		fd_set				_wSet;
		Handler				_handler;

	public:
		Listener();
		~Listener();

		int		getMaxFd() const;
		void	select();
		void	getRequest(int fd);
		void	sendResponse(int fd);

	private:
		void	acceptConnection(int fd);
		void	readRequest(int fd);
};

#endif