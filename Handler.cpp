#include "Handler.hpp"

Handler::Handler()
{
	
}

Handler::~Handler()
{
	
}

void			Handler::parseRequest(int fd, char *buf)
{
	Request				request;
	std::stringstream	is;
	char				content[128];

	//checkSyntax(buf);
	is << buf;
	is.getline(content, 128, ' ');
	request.method = content;
	is.getline(content, 128, ' ');
	request.uri = content;
	is.getline(content, 128, ' ');
	request.version = content;
	_requests[fd] = request;
}

std::string		Handler::generateResponse(int fd)
{
	Response	response;
	Request		request;
	int			file_fd;

	response.version = "HTTP/1.1";
	request = _requests[fd];
	file_fd = open(request.uri.substr(1, std::string::npos).c_str(), O_RDONLY);
	if (file_fd == -1)
		response.status_code = NOTFOUND;
	else
		response.status_code = OK;
	response.body = readFile(file_fd);
	return (toString(response));
}

std::string		Handler::toString(Response response)
{
	std::string		result;

	result = response.version + " " + response.status_code;
	// result += response.headers;
	result += "\n\n";
	result += response.body;
	return (result);
}

std::string		Handler::readFile(int file_fd)
{
	char		buf[4096];
	std::string	result;
	int			ret;

	if (file_fd == -1)
		return ("404: File not found\n");
	else
	{
		while ((ret = read(file_fd, buf, 4095)))
		{
			buf[ret] = '\0';
			result += buf;
		}
		return (result);
	}
}

