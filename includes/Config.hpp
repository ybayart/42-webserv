 #ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include "Server.hpp"

#define MAX_FD 256 - 20

class Config
{
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	public:
		Config();
		~Config();

		static void	exit(int sig);

		int			parse(char *file, std::vector<Server> &servers);
		void		init(fd_set *rSet, fd_set *wSet, fd_set *readSet, fd_set *writeSet, struct timeval *timeout);
		int			getMaxFd(std::vector<Server> &Servers);
		int			getOpenFd(std::vector<Server> &servers);

	private:
		std::string	readFile(char *file);
		int			getContent(std::string &buffer, std::string &context, std::string prec, config &config);
		int			checkContent(config &tmp);
};

#endif
