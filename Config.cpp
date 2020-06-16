#include "Config.hpp"

extern	std::vector<Server> g_servers;

Config::Config()
{

}

Config::~Config()
{

}

void			Config::exit(int sig)
{
	(void)sig;

	std::cout << "\n" << "exiting...\n";
	g_servers.clear();
	::exit(0);
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

int				Config::parse(char *file, std::vector<Server> &servers)
{
	std::string			context;
	std::stringstream	is;
	std::string			parsed;
	std::string			line;
	Server				server;
	config				tmp;

	parsed = readFile(file);

	if (parsed == "")
		return (0);
	is << parsed;
	while (!is.eof())
	{
		std::getline(is, line);
		while (line[0] == ' ' || line[0] == '\t')
			line.erase(line.begin());
		if (line == "server {" || line == "server\t{")
		{
			if (!getContent(is, context, line, tmp))
				return (0);
			else
			{
				if (!checkContent(tmp))
					return (0);
				else
				{
					std::vector<Server>::iterator it(servers.begin());
					while (it != servers.end())
					{
						if (tmp["server|"]["listen"] == it->_conf[0]["server|"]["listen"])
						{
							it->_conf.push_back(tmp);
							break ;
						}
						++it;
					}
					if (it == servers.end())
					{
						server._conf.push_back(tmp);
						servers.push_back(server);
					}
					server._conf.clear();
					tmp.clear();
					context.clear();
				}
			}
		}
	}
	return (1);
}

int				Config::getMaxFd(std::vector<Server> &servers)
{
	int		max = 0;
	int		fd;

	for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
	{
		fd = it->getMaxFd();
		if (fd > max)
			max = fd;
	}
	return (max);
}

int				Config::getOpenFd(std::vector<Server> &servers)
{
	int		nb = 0;

	for (std::vector<Server>::iterator it(servers.begin()); it != servers.end(); ++it)
	{
		nb += 1;
		nb += it->getOpenFd();
	}
	return (nb);
}

int				Config::getContent(std::stringstream &is, std::string &context, std::string prec, config &config)
{
	std::string			line;
	std::string			key;
	std::string			value;
	size_t				pos;

	prec.pop_back();
	while (prec.back() == ' ' || prec.back() == '\t')
		prec.pop_back();
	context += prec + "|";
	while (line[0] == ' ' || line[0] == '\t')
		line.erase(line.begin());
	while (line != "}" && !is.eof())
	{
		std::getline(is, line);
		while (line[0] == ' ' || line[0] == '\t')
			line.erase(line.begin());
		if (line == "}" || is.eof())
		{
			if (line == "}")
			{
				context.pop_back();
				context = context.substr(0, context.find_last_of('|') + 1);
			}
			break ;
		}
		if (line.back() != ';' && line.back() != '{')
			return (0);
		if (line.back() == '{')
			getContent(is, context, line, config);
		else
		{
			pos = 0;
			while (line[pos])
			{
				while (line[pos] && line[pos] != ' ' && line[pos] != '\t')
				{
					key += line[pos];
					++pos;
				}
				while (line[pos] == ' ' || line[pos] == '\t')
					++pos;
				while (line[pos] && line[pos] != ';')
				{
					value += line[pos];
					++pos;
				}
				if (line[pos])
					pos++;
			}
			// std::cout << key + " : " + value + " / c: " + context << std::endl;
			std::pair<std::string, std::string>	tmp(key, value);
			config[context].insert(tmp);
			key.clear();
			value.clear();
		}
	}
	if (line != "}")
		return (0);
	return (1);
}

//TO COMPLETE
int				Config::checkContent(config &tmp)
{
	if (tmp.find("server|") == tmp.end())
		return (0);
	return (1);
}