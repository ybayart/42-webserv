#include "Handler.hpp"

Handler::Handler()
{
	assignMIME();
}

Handler::~Handler()
{
	
}

void			Handler::parseRequest(int fd, std::string req)
{
	Request				request;
	std::stringstream	is;

	is << req;
	std::getline(is, request.method, ' ');
	std::getline(is, request.uri, ' ');
	std::getline(is, request.version,  '\n');
	parseHeaders(is, request);
	request.valid = checkSyntax(request);
	if (request.valid && request.method == "POST")
		parseBody(is, request);
	_requests[fd] = request;
}

void			Handler::sendStatusCode(int fd, Request &req, Response &res)
{
	int			file_fd = -1;
	std::string	result;
	std::string	path;

	res.version = "HTTP/1.1";
	if (!req.valid)
		res.status_code = BADREQUEST;
	else
	{
		path = req.uri.substr(1, req.uri.find('?') - 1);
		file_fd = open(path.c_str(), O_RDONLY);
		if (file_fd == -1)
			res.status_code = NOTFOUND;
		else
			res.status_code = OK;
	}
	close(file_fd);
	result = res.version + " " + res.status_code + "\n";
	write(fd, result.c_str(), result.size());
}

void			Handler::sendResponse(int fd)
{
	std::string		result;
	Request			request;
	Response		response;

	request = _requests[fd];
	sendStatusCode(fd, request, response);
	if (request.valid && request.uri.find(".cgi") != std::string::npos)
		execCGI(fd, request);
	else
	{
		fillHeaders(response, request);
		fillBody(response, request);
		result = toString(response, request);
		write(fd, result.c_str(), result.size());
	}
	_requests.erase(fd);
}

std::string		Handler::toString(const Response &response, Request req)
{
	std::map<std::string, std::string>::const_iterator b;
	std::string		result;

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
	std::string	path;

	if (res.status_code == OK)
	{
		path = req.uri.substr(1, req.uri.find('?') - 1);
		file_fd = open(path.c_str(), O_RDONLY);
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
	while ((ret = read(file_fd, buf, 4095)) > 0)
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
	if (req.version != "HTTP/1.1\r" && req.version != "HTTP/1.1")
		return (false);
	if (req.headers.find("Host") == req.headers.end())
		return (false);
	return (true);
}

void			Handler::fillHeaders(Response &res, Request &req)
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[4096];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, 4095, "%a, %d %b %G %T %Z", tm);
	buf[ret] = '\0';
	res.headers["Date"] = buf;
	res.headers["Server"] = "webserv";
	res.headers["Content-Type"] = findType(req);
}

std::string		Handler::findType(Request &req)
{
	std::string 	extension;

	extension = req.uri.substr(req.uri.find_last_of('.'));
	if (MIMETypes.find(extension) != MIMETypes.end())
		return (MIMETypes[extension]);
	else
		return (MIMETypes[".bin"]);
}

void			Handler::parseHeaders(std::stringstream &buf, Request &req)
{
	size_t		pos;
	std::string	line;

	while (!buf.eof())
	{
		std::getline(buf, line);
		if (line.size() < 1 || line[0] == '\n' || line[0] == '\r')
			break ;
		if (line.find(':') != std::string::npos)
		{
			pos = line.find(':');
 			req.headers[line.substr(0, pos)] = line.substr(pos, std::string::npos);
		}
	}
}

void			Handler::parseBody(std::stringstream &buf, Request &req)
{
	std::string	line;

	while (!buf.eof())
	{
		std::getline(buf, line, '\n');
		req.body += line;
	}
}

void			Handler::execCGI(int fd, Request &req)
{
	char			**args = NULL;
	char			**env = NULL;
	std::string		path;
	int				ret = fd;
	int				tubes[2];

	path = req.uri.substr(1, req.uri.find('?') - 1);
	args = (char **)(malloc(sizeof(char *) * 2));
	args[0] = strdup(path.c_str());
	args[1] = NULL;
	env = setEnv(req);

	pipe(tubes);
	if (fork() == 0)
	{
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(fd, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
			std::cout << "Error with CGI: " << strerror(errno) << std::endl;
	}
	else
	{
		close(tubes[0]);
		write(tubes[1], req.body.c_str(), req.body.size() + 1);
	}
	freeAll(args, env);
}

char			**Handler::setEnv(Request &req)
{
	char								**env;
	std::map<std::string, std::string> 	envMap;

	envMap["CONTENT_LENGTH"] = std::to_string(req.body.size());
	// envMap["CONTENT_TYPE"] = "text/html";
	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	// envMap["PATH_INFO"] = req.uri.substr(req.uri.find(".cgi") + 1, req.uri.find('?') - 1);
	// envMap["PATH_TRANSLATED"] = "";
	envMap["QUERY_STRING"] = req.uri.substr(req.uri.find('?') + 1);
	// envMap["SCRIPT_NAME"] = req.uri.substr();
	envMap["SERVER_NAME"] = "localhost";
	envMap["SERVER_PORT"] = "8080";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
	envMap["REQUEST_URI"] = req.uri;
	envMap["REQUEST_METHOD"] = req.method;

	env = (char **)malloc(sizeof(char *) * (envMap.size() + 1));
	std::map<std::string, std::string>::iterator it = envMap.begin();
	int i = 0;
	while (it != envMap.end())
	{
		env[i] = strdup((it->first + "=" + it->second).c_str());
		++i;
		++it;
	}
	env[i] = NULL;
	return (env);
}

void				Handler::freeAll(char **args, char **env)
{
	free(args[0]);
	free(args);
	int i = 0;
	while (env[i])
	{
		free(env[i]);
		++i;
	}
	free(env);
}

void				Handler::assignMIME()
{
	MIMETypes[".txt"] = "text/plain";
	MIMETypes[".bin"] = "application/octet-stream";
	MIMETypes[".jpeg"] = "image/jpeg";
	MIMETypes[".jpg"] = "image/jpeg";
	MIMETypes[".html"] = "text/html";
	MIMETypes[".htm"] = "text/html";
	MIMETypes[".png"] = "image/png";
	MIMETypes[".bmp"] = "image/bmp";
	MIMETypes[".pdf"] = "application/pdf";
	MIMETypes[".tar"] = "application/x-tar";
	MIMETypes[".json"] = "application/json";
	MIMETypes[".css"] = "text/css";
	MIMETypes[".js"] = "application/javascript";
	MIMETypes[".mp3"] = "audio/mpeg";
	MIMETypes[".avi"] = "video/x-msvideo";
}
