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

class Config
{
	friend class Listener;

	public:
		typedef std::map<std::string, std::string> elmt;

	private:
		std::map<std::string, elmt> _elmts;

	public:
		Config();
		~Config();

		int			parse(char *file);

	private:
		std::string	readFile(char *file);
};

#endif