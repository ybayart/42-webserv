#ifndef Server_HPP
#define Server_HPP

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
#include "Client.hpp"

#define TIMEOUT 10

class Server
{
	friend class Config;
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	private:
		int						_fd;
		int						_maxFd;
		struct sockaddr_in		_info;
		fd_set					*_readSet;
		fd_set					*_writeSet;
		fd_set					*_rSet;
		fd_set					*_wSet;
		Handler					_handler;
		config					_conf;

	public:
		std::map<int, Client*>		_clients;
		
		Server();
		~Server();

		int		getMaxFd() const;
		int		getFd() const;
		void	init(fd_set *readSet, fd_set *writeSet, fd_set *rSet, fd_set *wSet);
		bool	checkIfClient(int fd);
		void	acceptConnection();
		void	readRequest(int fd);
		void	writeResponse(int fd);

	private:
		int		getTimeDiff(std::string start);

};

#endif