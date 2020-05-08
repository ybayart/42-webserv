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
#include <time.h>
#include <sys/time.h>
#include <map>
#include <sys/stat.h>
#include <sys/errno.h>
#include "statusCode.h"
#include "Config.hpp"

class Handler
{
	public:
		struct Request
		{
			bool								valid;
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

		std::map<std::string, std::string> MIMETypes;

	public:
		Handler();
		~Handler();

		void			parseRequest(int fd, std::string buf);
		void			sendResponse(int fd, Config &conf);

	private:
		std::map<int, Request>	_requests;
		Config					_conf;

		void			sendStatusCode(int fd, Request &req, Response &res, Config &conf);
		void			fillBody(Response &response, Request &req, Config &conf);
		std::string		toString(const Response &response, Request req);
		bool			checkSyntax(const Request &request);
		void			fillHeaders(Response &res, Request &req);
		std::string		findType(Request &req);
		std::string		findPath(std::string uri, Config &conf);
		void			parseHeaders(std::stringstream &buf, Request &req);
		void			parseBody(std::stringstream &buf, Request &req);
		char			**setEnv(Request &req);
		void			execCGI(int fd, Request &req, Config &conf);
		void			freeAll(char **args, char **env);
		void			assignMIME();

};

#endif