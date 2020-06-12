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

class Config
{
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	public:
		Config();
		~Config();

		int			parse(char *file, std::vector<Server> &servers);
		void		init(fd_set *readSet, fd_set *writeSet, fd_set *rSet, fd_set *wSet);
		void		select(fd_set *readSet, fd_set *writeSet, fd_set *rSet, fd_set *wSet);
		int			getMaxFd(std::vector<Server> &Servers);

	private:
		std::string	readFile(char *file);
		int			getContent(std::stringstream &is, std::string &context,
		std::string prec, config &config);
		int			checkContent(config &tmp);
};

#endif