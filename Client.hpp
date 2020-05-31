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

#define CODE 	1
#define HEADERS 2
#define BODY 	3
#define DONE 	4
#define CGI 	5
#define STANDBY	6

class Client
{
	typedef std::map<std::string, std::string> t_conf;
	friend class Listener;
	friend class Handler;

	private:
		int			fd;
		int			fileFd;
		Request		req;
		Response	res;
		t_conf 		conf;
		char		*rBuf;
		char		*wBuf;
		int			rBytes;
		fd_set		*rSet;
		fd_set		*wSet;
		bool		hasBody;
		int			status;

	public:
		Client(int filed, fd_set *r, fd_set *w);
		~Client();

		bool	getReadState();
		bool	getWriteState();
		void	setReadState(bool state);
		void	setWriteState(bool state);
};

#endif