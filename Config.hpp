#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fcntl.h>
#include <unistd.h>
#include <string>

class Config
{
	friend class Listener;

	private:
		int		_port;

	public:
		Config();
		~Config();

		int		parse(char *file);
};

#endif