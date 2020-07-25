#include <string>
#include <iostream>
#include "Config.hpp"
#include "Server.hpp"

namespace ft
{

	bool	isspace(int c)
	{
		if (c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' ||
c == ' ')
			return (true);
		return (false);
	}

	void	getline(std::string &buffer, std::string &line)
	{
		size_t					pos;

		pos = buffer.find("\n");
		if (pos != std::string::npos)
		{
			line = std::string (buffer, 0, pos++);
			buffer = buffer.substr(pos);
		}
		else
		{
			if (buffer[buffer.size() - 1] == '\n')
				buffer = buffer.substr(buffer.size());
			else
			{
				line = buffer;
				buffer = buffer.substr(buffer.size());
			}
		}
	}

	void	getline(std::string &buffer, std::string &line, char delim)
	{
		size_t					pos;

		pos = buffer.find(delim);
		if (pos != std::string::npos)
		{
			line = std::string (buffer, 0, pos++);
			buffer = buffer.substr(pos);
		}
		else
		{
			if (buffer[buffer.size() - 1] == delim)
				buffer = buffer.substr(buffer.size());
			else
			{
				line = buffer;
				buffer = buffer.substr(buffer.size());
			}
		}
	}

	int		getpower(int nb, int power)
	{
		if (power < 0)
			return (0);
		if (power == 0)
			return (1);
		return (nb * getpower(nb, power - 1));
	}

	void	freeAll(char **args, char **env)
	{
		free(args[0]);
		free(args[1]);
		free(args);
		int i = 0;
		while (env[i])
		{
			free(env[i]);
			++i;
		}
		free(env);
	}

	void	print_exception(std::exception &e)
	{
		std::cerr << "Error: ";
		try
		{
			Config::InvalidConfigFileException &ie = dynamic_cast<Config::InvalidConfigFileException&>(e);
			if (ie.getLine())
				std::cerr << "line " << ie.getLine() << ": ";
		}
		catch (std::bad_cast &b)
		{
			try
			{
				Server::ServerException &se = dynamic_cast<Server::ServerException&>(e);
				if (!se.getFunction().empty())
					std::cerr << se.getFunction() << ": ";
			}
			catch (std::bad_cast &b)
			{
			}
		}
		std::cerr << e.what() << std::endl;
	}
}
