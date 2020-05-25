#ifndef CLIENT_HPP
#define CLIENT_HPP

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

#define BUFFER_SIZE 33000

class Client
{
	typedef std::map<std::string, std::string> conf;
	friend class Listener;
	friend class Handler;

	private:
		int			_fd;
		Request		_req;
		conf 		_conf;
		char		*_rBuf;
		char		*_wBuf;
		int			_rBytes;
		int			_wBytes;
		fd_set		*_rSet;
		fd_set		*_wSet;
		bool		hasBody;

	public:
		Client(int fd, fd_set *r, fd_set *w);
		~Client();

		bool	getReadState();
		bool	getWriteState();
		void	setReadState(bool state);
		void	setWriteState(bool state);
};

#endif