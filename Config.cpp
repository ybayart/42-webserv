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
	std::string			context;
	std::stringstream	is;
	std::string			parsed;
	std::string			line;

	parsed = readFile(file);
	if (parsed == "")
		return (-1);
	is << parsed;
	while (!is.eof())
	{
		std::getline(is, line);
		while (line[0] == ' ' || line[0] == '\t')
			line.erase(line.begin());
		if (line.back() == '{')
			if (getContent(is, context, line) == -1)
				return (-1);
	}
	return (checkContent());
}

int		Config::getContent(std::stringstream &is, std::string &context, std::string prec)
{
	std::string			line;
	std::string			key;
	std::string			value;
	elmt				cur;

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
			return (-1);
		if (line.back() == '{')
			getContent(is, context, line);
		else
		{
			key = line.substr(0, line.find(' '));
			value = line.substr(line.find(' ') + 1);
			value.pop_back();
			std::cout << key + " : " + value + " / c: " + context << std::endl;
			std::pair<std::string, std::string>	tmp(key, value);
			_elmts[context].insert(tmp);
		}
	}
	if (line != "}")
		return (-1);
	return (0);
}

//TO COMPLETE
int		Config::checkContent()
{
	if (_elmts.find("server|") == _elmts.end())
		return (-1);
	return (0);
}