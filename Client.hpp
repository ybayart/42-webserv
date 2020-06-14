#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include "string"
#include "map"
#include "messages.h"

#define BUFFER_SIZE 4096

#define PARSING		1
#define BODYPARSING 2
#define CODE 		3
#define HEADERS 	4
#define CGI 		5
#define BODY 		6
#define STANDBY		7
#define DONE 		8

class Client
{
	typedef std::map<std::string, std::string> t_conf;
	friend class Server;
	friend class Handler;
	friend class Helper;

	public:	
		struct t_chunk
		{
			unsigned int	len;
			bool			done;
			bool			found;
		};

	private:
		int			fd;
		std::string	ip;
		int			port;
		int			pid;
		int			fileFd;
		std::string	file_str;
		std::string	tmp_path;
		Request		req;
		Response	res;
		t_conf 		conf;
		char		*rBuf;
		char		*wBuf;
		fd_set		*rSet;
		fd_set		*wSet;
		int			status;
		std::string	lastDate;
		t_chunk		chunk;

	public:
		Client(int filed, fd_set *r, fd_set *w, struct sockaddr_in info);
		~Client();

		int		getFd() const;
		bool	getReadState();
		bool	getWriteState();
		void	setReadState(bool state);
		void	setWriteState(bool state);
		void	setToStandBy();
};

#endif