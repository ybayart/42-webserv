#ifndef UTILS_H
# define UTILS_H

# include <string>

namespace ft
{
	bool		isspace(int c);
	void		getline(std::string &buffer, std::string &line);
	void		getline(std::string &buffer, std::string &line, char delim);
	int			getpower(int nb, int power);
	void		freeAll(char **args, char **env);
	void		print_exception(std::exception &e);
}

#endif
