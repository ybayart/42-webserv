#include "Config.hpp"
#include <cstdlib>

Config::Config() : _port(-1)
{

}

Config::~Config()
{

}

int		Config::parse(char *file)
{
	int 		fd;
	int			ret;
	char		buf[4096];
	std::string	parsed;

	fd = open(file, O_RDONLY);
	while ((ret = read(fd, buf, 4095)) > 0)
	{
		buf[ret] = '\0';
		parsed += buf;
	}
	close(fd);
	_port = 8080;
	return (0);
}