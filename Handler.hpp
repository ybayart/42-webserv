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
#include "messages.h"
#include "statusCode.h"
#include "Config.hpp"
#include "Client.hpp"

class Handler
{
	public:
		std::map<std::string, std::string> MIMETypes;

	public:
		Handler();
		~Handler();

		void			parseRequest(Client &client, Config &conf);
		void			sendResponse(Client &client);

	private:
		void			assignMIME();
		void			getConf(Client &client, Request &req, Config &conf);
		void			dispatcher(Client &client);
		void			handleBadRequest(int fd, Request &req);

		void			handleGet(Client &client, Response &res);
		void			handleHead(Client &client, Response &res);
		void			handlePost(Client &client, Response &res);
		void			handlePut(Client &client, Response &res);

		std::string		getDate();
		std::string		getLastModified(std::string path);
		void			writeStatus(int fd, Response &res);
		std::string		toString(const Response &response);
		bool			checkSyntax(const Request &request);
		std::string		findType(Request &req);
		void			parseHeaders(std::stringstream &buf, Request &req);
		void			parseBody(std::stringstream &buf, Request &req);
		char			**setEnv(Request &req);
		void			execCGI(Client &client);
		void			freeAll(char **args, char **env);

};

#endif