#include "Handler.hpp"

Handler::Handler()
{
	
}

Handler::~Handler()
{
	
}

void			Handler::parseRequest(int fd, char *req)
{
	Request				request;
	std::stringstream	is;
	char				content[128];

	request.valid = checkSyntax(req) ? true : false;
	if (request.valid == true)
	{
		is << req;
		is.getline(content, 128, ' ');
		request.method = content;
		is.getline(content, 128, ' ');
		request.uri = content;
		is.getline(content, 128, ' ');
		request.version = content;
	}
	_requests[fd] = request;
}

std::string		Handler::generateResponse(int fd)
{
	Response	response;
	Request		request;
	int			file_fd;
	std::string	result;

	response.version = "HTTP/1.1";
	request = _requests[fd];
	file_fd = open(request.uri.substr(1, std::string::npos).c_str(), O_RDONLY);
	if (request.valid)
	{
		if (file_fd == -1)
			response.status_code = NOTFOUND;
		else
			response.status_code = OK;
	}
	else
		response.status_code = BADREQUEST;
	close(file_fd);
	fillBody(response, request);
	fillHeaders(response, request);
	result = toString(response, request);
	_requests.erase(fd);
	return (result);
}

std::string		Handler::toString(const Response &response, Request req)
{
	std::string		result;
	std::map<std::string, std::string>::const_iterator b;

	result = response.version + " " + response.status_code + "\n";
	b = response.headers.begin();
	while (b != response.headers.end())
	{
		result += b->first + ": " + b->second + "\n";
		++b;
	}
	result += "\n";
	if (req.method != "HEAD")
		result += response.body;
	return (result);
}

void			Handler::fillBody(Response &res, Request req)
{
	char		buf[4096];
	std::string	result;
	int			ret;
	int			file_fd;

	if (res.status_code == OK)
		file_fd = open(req.uri.substr(1, std::string::npos).c_str(), O_RDONLY);
	else if (res.status_code == NOTFOUND)
		file_fd = open("errorPages/404.html", O_RDONLY);
	else if (res.status_code == BADREQUEST)
		file_fd = open("errorPages/400.html", O_RDONLY);
	while ((ret = read(file_fd, buf, 4095)))
	{
		buf[ret] = '\0';
		result += buf;
	}
	close(file_fd);
	res.body = result;
}

//TO COMPLETE
bool			Handler::checkSyntax(char *req)
{
	if (strncmp(req, "GET", 3)
		&& strncmp(req, "HEAD", 4))
		return (false);
	else
		return (true);
}

void			Handler::fillHeaders(Response &response, Request request)
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[4096];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, 4095, "%a, %d %b %G %T %Z", tm);
	buf[ret] = '\0';
	response.headers["Date"] = buf;
	response.headers["Server"] = "webserv";
	response.headers["Content-Length"] = std::to_string(response.body.size());
}

