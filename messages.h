#ifndef MESSAGES_H
#define MESSAGES_H

#include <map>
#include <string>

struct Request
{
	bool								valid;
	std::string							method;
	std::string							uri;
	std::string							version;
	std::map<std::string, std::string> 	headers;
	std::string							body;
};

struct Response
{
	std::string							version;
	std::string							status_code;
	std::map<std::string, std::string> 	headers;
	std::string							body;
};

#endif