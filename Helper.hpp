#ifndef HELPER_HPP
#define HELPER_HPP

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

class Helper
{
	public:
		std::map<std::string, std::string> MIMETypes;

		Helper();
		~Helper();

		std::string		getDate();
		std::string		getLastModified(std::string path);
		std::string		findType(Request &req);
		int				findLen(Client &client);
		void			fillBody(Client &client, int *len, bool *found);
		int				ft_power(int nb, int power);
		int				fromHexa(const char *nb);
		char			**setEnv(Client &client);
		void			freeAll(char **args, char **env);
		std::string		decode64(const char *data);
		void			negotiate(Client &client);
		void			assignMIME();

};

#endif