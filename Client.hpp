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

#define BUFFER_SIZE 50000

#define CODE 	1
#define HEADERS 2
#define BODY 	3
#define DONE 	4
#define CGI 	5
#define STANDBY	6
#define PARSING	7

class Client
{
	typedef std::map<std::string, std::string> t_conf;
	friend class Listener;
	friend class Handler;
	friend class Helper;

	private:
		int			fd;
		std::string	ip;
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
		bool		hasBody;
		int			status;
		std::string	lastDate;

	public:
		Client(int filed, fd_set *r, fd_set *w);
		~Client();

		bool	getReadState();
		bool	getWriteState();
		void	setReadState(bool state);
		void	setWriteState(bool state);
		void	setToStandBy();
};

#endif