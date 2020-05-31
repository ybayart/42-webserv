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
		void			parseBody(Client &client);
		void			dispatcher(Client &client);

	private:
		void			assignMIME();
		void			getConf(Client &client, Request &req, Config &conf);

		void			handleGet(Client &client);
		void			handleHead(Client &client);
		void			handlePost(Client &client);
		void			handlePut(Client &client);
		void			handleBadRequest(Client &client);

		std::string		getDate();
		std::string		getLastModified(std::string path);
		void			fillStatus(Client &client);
		void			fillHeaders(Client &client);
		bool			checkSyntax(const Request &request);
		std::string		findType(Request &req);
		void			parseHeaders(std::stringstream &buf, Request &req);
		void			dechunkBody(Client &client);
		int				ft_power(int nb, int power);
		int				fromHexa(const char *nb);
		char			**setEnv(Client &client);
		void			execCGI(Client &client);
		void			freeAll(char **args, char **env);

};

#endif