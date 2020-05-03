#include "Config.hpp"

Config::Config()
{

}

Config::~Config()
{

}

std::string		Config::readFile(char *file)
{
	int 				fd;
	int					ret;
	char				buf[4096];
	std::string			parsed;

	fd = open(file, O_RDONLY);
	while ((ret = read(fd, buf, 4095)) > 0)
	{
		buf[ret] = '\0';
		parsed += buf;
	}
	close(fd);
	return (parsed);
}

int		Config::parse(char *file)
{
	std::string			line;
	std::stringstream	is;
	elmt				cur;
	std::string			key;
	std::string			value;
	std::string			parsed;

	parsed = readFile(file);
	if (parsed == "")
		return (1);
	is << parsed;
	while (!is.eof())
	{
		std::getline(is, line);
		if (line == "server")
		{
			std::getline(is, line);
			if (line != "{")
				return (1);
			else
			{
				while (line != "}" && !is.eof())
				{
					std::getline(is, line);
					if (line == "}" || is.eof())
						break ;
					key = line.substr(0, line.find(' '));
					value = line.substr(line.find(' ') + 1);
					value = value.substr(0, value.size() - 1);
					std::cout << key + " : " + value << std::endl;
					cur[key] = value;
				}
				if (line != "}")
					return (-1);
				_elmts["server"] = cur;
			}
		}
	}
	return (0);
}