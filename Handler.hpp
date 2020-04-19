#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <sstream>
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
#include "statusCode.h"

class Handler
{
	public:
		struct Request
		{
			std::string							method;
			std::string							uri;
			std::string							version;
			std::map<std::string, std::string> 	headers;
			std::string							body;
		};

		struct Response
		{
			std::string							version;
			std::string							status_code;
			std::map<std::string, std::string> 	headers;
			std::string							body;
		};

	public:
		Handler();
		~Handler();

		void			parseRequest(int fd, char *buf);
		std::string		generateResponse(int fd);

	private:
		std::map<int, Request>	_requests;

		std::string		readFile(int file_fd);
		std::string		toString(Response response);

};

#endif