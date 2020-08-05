#ifndef UTILS_H
# define UTILS_H

# include <string>

namespace ft
{
	bool		isspace(int c);
	void		getline(std::string &buffer, std::string &line);
	void		getline(std::string &buffer, std::string &line, char delim);
	int			getpower(int nb, int power);
	std::string	getDate();
	void		freeAll(char **args, char **env);
}

#endif
