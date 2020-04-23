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

	is << req;
	is.getline(content, 128, ' ');
	request.method = content;
	is.getline(content, 128, ' ');
	request.uri = content;
	is.getline(content, 128, '\n');
	request.version = content;
	parseHeaders(is, request);
	request.valid = checkSyntax(request);
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
	fillHeaders(response);
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
	struct tm	*tm;
	int			file_fd = -1;
	struct stat	file_info;

	if (res.status_code == OK)
	{
		file_fd = open(req.uri.substr(1, std::string::npos).c_str(), O_RDONLY);
		fstat(file_fd, &file_info);
		tm = localtime(&file_info.st_mtime);
		ret = strftime(buf, 4095, "%a, %d %b %G %T %Z", tm);
		buf[ret] = '\0';
		res.headers["Last-Modified"] = buf;
	}
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
	res.headers["Content-Length"] = std::to_string(result.size());
	res.body = result;
}

//TO COMPLETE
bool			Handler::checkSyntax(const Request &req)
{
	if (req.method.size() == 0 || req.uri.size() == 0
		|| req.version.size() == 0)
		return (false);
	if (req.method != "GET" && req.method != "POST"
		&& req.method != "HEAD")
		return (false);
	if (req.uri[0] != '/')
		return (false);
	if (req.version != "HTTP/1.1")
		return (false);
	if (req.headers.find("Host") == req.headers.end())
		return (false);
	return (true);
}

void			Handler::fillHeaders(Response &response)
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
}

void			Handler::parseHeaders(std::stringstream &buf, Request &req)
{
	char		content[128];
	size_t		pos;
	std::string	line;

	while (!buf.eof())
	{
		buf.getline(content, 128);
		if (strlen(content) < 1)
			break ;
		line = content;
		if (line.find(':') != std::string::npos)
		{
			pos = line.find(':');
 			req.headers[line.substr(0, pos)] = line.substr(pos, std::string::npos);
		}
	}
}

