#include "Handler.hpp"

Handler::Handler()
{
	assignMIME();
}

Handler::~Handler()
{
	
}

void			Handler::parseRequest(Client &client, Config &conf)
{
	std::stringstream	is;
	Request				request;

	is << client._rBuf;
	std::getline(is, request.method, ' ');
	std::getline(is, request.uri, ' ');
	std::getline(is, request.version,  '\n');
	parseHeaders(is, request);
	request.valid = checkSyntax(request);
	if (request.valid)
	{
		getConf(client, request, conf);
		if (request.method == "POST" || request.method == "PUT")
			parseBody(is, request);
	}
	else
		handleBadRequest(client._fd, request);
	client._req = request;
}

void			Handler::handleBadRequest(int fd, Request &req)
{
	char			buf[4096];
	int				ret;
	int				file_fd;
	std::string		result;
	Response		res;

	res.version = "HTTP/1.1";
	res.status_code = BADREQUEST;
	writeStatus(fd, res);
	res.headers["Date"] = getDate();
	res.headers["Server"] = "webserv";
	file_fd = open("errorPages/400.html", O_RDONLY);
	while ((ret = read(file_fd, buf, 4095)) > 0)
	{
		buf[ret] = '\0';
		result += buf;
	}
	close(file_fd);
	res.headers["Content-Length"] = std::to_string(result.size());
	res.body = result;
	write(fd, toString(res).c_str(), toString(res).size());
}

void			Handler::getConf(Client &client, Request &req, Config &conf)
{
	std::string		directory;
	std::string		subdir;
	std::string		tmp;
	std::string		file;
	std::map<std::string, std::string> elmt;
	struct stat		info;

	directory = req.uri.substr(0, req.uri.find_last_of('/'));
	file = req.uri.substr(req.uri.find_last_of('/'), req.uri.find('?'));
	if (directory == "")
		directory = "/";
	tmp = directory;
	if (conf._elmts.find("server|location " + req.uri + "|") != conf._elmts.end())
	{
		elmt = conf._elmts["server|location " + req.uri + "|"];
		tmp = req.uri;
	}
	else if (conf._elmts.find("server|location " + directory + "|") != conf._elmts.end())
		elmt = conf._elmts["server|location " + directory + "|"];
	else
	{
		subdir = directory;
		while (tmp != "/")
		{
			tmp = tmp.substr(0, tmp.find_last_of('/'));
			if (tmp == "")
				tmp = "/";
			if (conf._elmts.find("server|location " + tmp + "|") != conf._elmts.end())
			{
				elmt = conf._elmts["server|location " + tmp + "|"];
				break ;
			}
		}
	}
	if (elmt.size() > 0)
	{
		if (elmt.find("methods") != elmt.end())
			client._conf["methods"] = elmt["methods"];
		else
			client._conf["methods"] = "GET";
		if (elmt.find("CGI") != elmt.end())
			client._conf["CGI"] = elmt["CGI"];
		if (elmt.find("upload") != elmt.end())
			client._conf["upload"] = elmt["upload"];
		if (elmt.find("root") != elmt.end())
			client._conf["path"] = elmt["root"] + subdir + file;
		else
			client._conf["path"] = directory.substr(1) + subdir + file;
		lstat(client._conf["path"].c_str(), &info);
		if (S_ISDIR(info.st_mode))
		{
			if ((elmt.find("root") != elmt.end()
			&& client._conf["path"] == (elmt["root"] + "/"))
			|| client._conf["path"] == tmp.substr(1))
			{
				if (elmt.find("default") != elmt.end())
				{
					if (client._conf["path"].back() != '/')
						client._conf["path"] += "/";
					client._conf["path"] += elmt["default"];
				}
			}
			else
				client._conf["isdir"] = "true";
		}
		std::cout << "path: " << client._conf["path"] << std::endl;
	}
}

void			Handler::dispatcher(Client &client)
{
	Response		res;
	std::string		result;
	typedef void	(Handler::*ptr)(Client &client, Response &res);

	ptr				ptr_array[4] = {
		&Handler::handleGet, &Handler::handleHead, &Handler::handlePost,
		&Handler::handlePut
	};
	std::string		str_array[4] = {
		"GET", "HEAD", "POST", "PUT"
	};
	
	if (client._req.valid)
	{
		for (int i = 0; i < 4; ++i)
			if (str_array[i] == client._req.method)
				(this->*ptr_array[i])(client, res);
	}
}

void			Handler::sendResponse(Client &client)
{
	dispatcher(client);
}

void			Handler::writeStatus(int fd, Response &res)
{
	std::string		status;

	status = res.version + " " + res.status_code + "\n";
	write(fd, status.c_str(), status.size());
}

std::string		Handler::toString(const Response &response)
{
	std::map<std::string, std::string>::const_iterator b;
	std::string		result;

	b = response.headers.begin();
	while (b != response.headers.end())
	{
		if (b->second != "")
			result += b->first + ": " + b->second + "\n";
		++b;
	}
	result += "\n";
	result += response.body;
	return (result);
}

std::string			Handler::getDate()
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[4096];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, 4095, "%a, %d %b %G %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

std::string			Handler::getLastModified(std::string path)
{
	char		buf[4096];
	int			ret;
	struct tm	*tm;
	int			fd;
	struct stat	file_info;

	fd = open(path.c_str(), O_RDONLY);
	fstat(fd, &file_info);
	tm = localtime(&file_info.st_mtime);
	ret = strftime(buf, 4095, "%a, %d %b %G %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

//TO COMPLETE
bool			Handler::checkSyntax(const Request &req)
{
	if (req.method.size() == 0 || req.uri.size() == 0
		|| req.version.size() == 0)
		return (false);
	if (req.method != "GET" && req.method != "POST"
		&& req.method != "HEAD" && req.method != "PUT")
		return (false);
	if (req.uri[0] != '/')
		return (false);
	if (req.version != "HTTP/1.1\r" && req.version != "HTTP/1.1")
		return (false);
	if (req.headers.find("Host") == req.headers.end())
		return (false);
	return (true);
}

std::string		Handler::findType(Request &req)
{
	std::string 	extension;

	if (req.uri.find_last_of('.') != std::string::npos)
	{
		extension = req.uri.substr(req.uri.find_last_of('.'));		
		if (MIMETypes.find(extension) != MIMETypes.end())
			return (MIMETypes[extension]);
		else
			return (MIMETypes[".bin"]);
	}
	return ("");
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
 			req.headers[line.substr(0, pos)] = line.substr(pos + 2, std::string::npos);
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

void			Handler::execCGI(Client &client)
{
	char			**args = NULL;
	char			**env = NULL;
	std::string		path;
	int				ret;
	int				tubes[2];

	path = client._conf["path"];
	args = (char **)(malloc(sizeof(char *) * 2));
	args[0] = strdup(path.c_str());
	args[1] = NULL;
	env = setEnv(client._req);

	pipe(tubes);
	if (fork() == 0)
	{
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(client._fd, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
			std::cout << "Error with CGI: " << strerror(errno) << std::endl;
	}
	else
	{
		close(tubes[0]);
		write(tubes[1], client._req.body.c_str(), client._req.body.size() + 1);
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
