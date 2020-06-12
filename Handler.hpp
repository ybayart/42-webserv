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
#include "Client.hpp"
#include "Helper.hpp"

class Handler
{
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	public:
		Handler();
		~Handler();

		Helper	_helper;

		void			parseRequest(Client &client, config &conf);
		void			parseBody(Client &client);
		void			dispatcher(Client &client);

	private:
		void			handleGet(Client &client);
		void			handleHead(Client &client);
		void			handlePost(Client &client);
		void			handlePut(Client &client);
		void			handleBadRequest(Client &client);

		void			getConf(Client &client, Request &req, config &conf);
		void			negotiate(Client &client);
		bool			checkSyntax(const Request &request);
		void			parseHeaders(std::stringstream &buf, Request &req);
		void			getBody(Client &client);
		void			dechunkBody(Client &client);
		void			execCGI(Client &client);
		bool			readCGIResult(Client &client);
		void			parseCGIResult(Client &client);

};

#endif