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
	std::string			tmp;

	is << client.rBuf;
	if (is.peek() == '\r')
		is.get();
	if (is.peek() == '\n')
		is.get();
	std::getline(is, request.method, ' ');
	std::getline(is, request.uri, ' ');
	std::getline(is, request.version,  '\n');
	parseHeaders(is, request);
	request.valid = checkSyntax(request);
	if (request.valid)
	{
		getConf(client, request, conf);
		if (request.method == "POST" || request.method == "PUT")
		{
			client.hasBody = true;
			client.setWriteState(true);
			if (request.method == "PUT")
				client.status = PARSING;
			if (request.method == "PUT"
			&& client.conf["isdir"] != "true"
			&& client.conf["methods"].find(request.method) != std::string::npos)
				client.fileFd = open(client.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
		}
		else
		{
			client.setReadState(false);
			client.setWriteState(true);
			client.status = CODE;
		}
	}
	else
	{
		request.method = "BAD";
		client.setReadState(false);
		client.setWriteState(true);
		client.status = CODE;
	}
	client.req = request;
	tmp = client.rBuf;
	tmp = tmp.substr(tmp.find("\r\n\r\n") + 4);
	strcpy(client.rBuf, tmp.c_str());
}

void			Handler::getConf(Client &client, Request &req, Config &conf)
{
	std::map<std::string, std::string> elmt;
	std::string		tmp;
	std::string 	file;
	struct stat		info;

	file = req.uri.substr(req.uri.find_last_of('/') + 1, req.uri.find('?'));
	tmp = req.uri;
	do
	{
		if (conf._elmts.find("server|location " + tmp + "|") != conf._elmts.end())
		{
			elmt = conf._elmts["server|location " + tmp + "|"];
			break ;
		}
		tmp = tmp.substr(0, tmp.find_last_of('/'));
	} while (tmp != "");
	if (elmt.size() == 0)
		if (conf._elmts.find("server|location /|") != conf._elmts.end())
			elmt = conf._elmts["server|location /|"];
	if (elmt.size() > 0)
	{
		if (elmt.find("methods") != elmt.end())
			client.conf["methods"] = elmt["methods"];
		else
			client.conf["methods"] = "GET";
		if (elmt.find("CGI") != elmt.end())
			client.conf["CGI"] = elmt["CGI"];
		if (elmt.find("exec") != elmt.end())
			client.conf["exec"] = elmt["exec"];
		if (elmt.find("upload") != elmt.end())
			client.conf["upload"] = elmt["upload"];
		if (elmt.find("root") != elmt.end())
			client.conf["path"] = req.uri.replace(0, tmp.size(), elmt["root"]);
		else
			client.conf["path"] = req.uri;
	}
	lstat(client.conf["path"].c_str(), &info);
	if (S_ISDIR(info.st_mode))
	{
		if (elmt.find("default") != elmt.end())
			client.conf["path"] += "/" + elmt["default"];
		else
			client.conf["isdir"] = "true";
	}
	std::cout << "p: " << client.conf["path"] << std::endl;
}

void			Handler::dispatcher(Client &client)
{
	typedef void	(Handler::*ptr)(Client &client);
	std::map<std::string, ptr> map;

	map["GET"] = &Handler::handleGet;
	map["HEAD"] = &Handler::handleHead;
	map["PUT"] = &Handler::handlePut;
	map["POST"] = &Handler::handlePost;
	map["BAD"] = &Handler::handleBadRequest;

	(this->*map[client.req.method])(client);
}

void			Handler::fillStatus(Client &client)
{
	std::string		status;

	status = client.res.version + " " + client.res.status_code + "\r\n";
	strcpy(client.wBuf, status.c_str());
	client.status = HEADERS;
}

void			Handler::fillHeaders(Client &client)
{
	std::string		result;
	std::map<std::string, std::string>::const_iterator b;

	b = client.res.headers.begin();
	while (b != client.res.headers.end())
	{
		if (b->second != "")
			result += b->first + ": " + b->second + "\r\n";
		++b;
	}
	result += "\r\n";
	strcpy(client.wBuf, result.c_str());
	client.status = BODY;
}

std::string			Handler::getDate()
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[BUFFER_SIZE];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, BUFFER_SIZE - 1, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

std::string			Handler::getLastModified(std::string path)
{
	char		buf[BUFFER_SIZE];
	int			ret;
	struct tm	*tm;
	struct stat	file_info;

	lstat(path.c_str(), &file_info);
	tm = localtime(&file_info.st_mtime);
	ret = strftime(buf, BUFFER_SIZE - 1, "%a, %d %b %Y %T %Z", tm);
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

void			Handler::parseBody(Client &client)
{
	std::string	tmp;
	int			i;
	int			to_read;
	int			bytes;

	if (client.req.headers.find("Content-Length") != client.req.headers.end())
	{
		to_read = atoi(client.req.headers["Content-Length"].c_str());
		bytes = strlen(client.rBuf);
		if (bytes >= to_read)
			memset(client.rBuf + to_read, 0, BUFFER_SIZE - to_read);
		else
		{
			to_read -= bytes;
			if (to_read > 0)
				bytes += read(client.fd, client.rBuf + bytes, to_read);
			if (bytes > 0)
				client.rBuf[bytes] = '\0';
		}
		std::cout << "b: " << client.rBuf << std::endl;
		write(client.fileFd, client.rBuf, strlen(client.rBuf));
		memset(client.rBuf, 0, BUFFER_SIZE);
		client.hasBody = false;
		client.setReadState(false);
		client.setWriteState(true);
		if (client.status == CGI)
		{
			close(client.fileFd);
			client.setToStandBy();
		}
		else if (client.status == PARSING)
		{
			if (client.req.method == "PUT")
				client.status = CODE;
			else
				client.status = HEADERS;
		}
	}
	else if (client.req.headers.find("Transfer-Encoding") != client.req.headers.end()
	&& client.req.headers["Transfer-Encoding"] == "chunked\r")
		dechunkBody(client);
}

void			Handler::dechunkBody(Client &client)
{
	static int 		len = 0;
	std::string		to_convert;
	int				bytes;
	std::string		tmp;
	static bool		found = false;
	static bool		done = 0;
	int				i;
	int				ret;

	bytes = strlen(client.rBuf);
	memset(client.rBuf + bytes, 0, BUFFER_SIZE - bytes);
	ret = read(client.fd, client.rBuf + bytes, len + 2);
	if (ret == -1)
		return ;
	if (done)
	{
		memset(client.rBuf, 0, BUFFER_SIZE);
		client.hasBody = false;
		client.setReadState(false);
		client.setWriteState(true);
		if (client.status == CGI)
		{
			close(client.fileFd);
			client.setToStandBy();
		}
		else if (client.status == PARSING)
		{
			if (client.req.method == "PUT")
				client.status = CODE;
			else
				client.status = HEADERS;
		}
		found = false;
		done = 0;
		return ;
	}
	bytes += ret;
	client.rBuf[bytes] = '\0';
	if (strstr(client.rBuf, "\r\n") && found == false)
	{
		to_convert = client.rBuf;
		to_convert = to_convert.substr(0, to_convert.find("\r\n"));
		std::cout << to_convert << ";" << std::endl;
		len = fromHexa(to_convert.c_str());
		std::cout << "l: " << len << std::endl;
		if (len == 0)
			done = 1;
		else
		{
			tmp = client.rBuf;
			tmp = tmp.substr(tmp.find("\r\n") + 2);
			strcpy(client.rBuf, tmp.c_str());
			found = true;
		}
	}
	else if (found == true)
	{
		tmp = client.rBuf;
		if (tmp.size() > len)
		{
			if (client.fileFd != -1)
				bytes = write(client.fileFd, tmp.substr(0, len).c_str(), len);
			tmp = tmp.substr(len + 1);
			memset(client.rBuf, 0, BUFFER_SIZE);
			strcpy(client.rBuf, tmp.c_str());
			len = 0;
			found = false;
		}
		else
		{
			if (client.fileFd != -1)
				bytes = write(client.fileFd, tmp.c_str(), tmp.size());
			len -= tmp.size();
			memset(client.rBuf, 0, BUFFER_SIZE);
		}
	}
}

int				Handler::ft_power(int nb, int power)
{
	if (power < 0)
		return (0);
	if (power == 0)
		return (1);
	return (nb * ft_power(nb, power - 1));
}

int				Handler::fromHexa(const char *nb)
{
	char	base[17] = "0123456789abcdef";
	char	base2[17] = "0123456789ABCDEF";
	int		result = 0;
	int		i;
	int		index;

	i = 0;
	while (nb[i])
	{
		int j = 0;
		while (base[j])
		{
			if (nb[i] == base[j])
			{
				index = j;
				break ;
			}
			j++;
		}
		if (j == 16)
		{
			j = 0;
			while (base2[j])
			{
				if (nb[i] == base2[j])
				{
					index = j;
					break ;
				}
				j++;
			}
		}
		result += index * ft_power(16, (strlen(nb) - 1) - i);
		i++;
	}
	return (result);
}

void			Handler::execCGI(Client &client)
{
	char			**args = NULL;
	char			**env = NULL;
	std::string		path;
	int				ret;
	int				tubes[2];
	int				bytes;
	std::string		dir;

	if (client.conf.find("exec") != client.conf.end())
		path = client.conf["exec"];
	else
		path = client.conf["path"];
	args = (char **)(malloc(sizeof(char *) * 2));
	args[0] = strdup(path.c_str());
	args[1] = NULL;
	env = setEnv(client);
	pipe(tubes);
	if (fork() == 0)
	{
		// dir = client.conf["path"].substr(0, client.conf["path"].find_last_of('/'));
		// ret = chdir(dir.c_str());
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(client.fd, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
			std::cout << "Error with CGI: " << strerror(errno) << std::endl;
	}
	else
	{
		close(tubes[0]);
		client.fileFd = tubes[1];
	}
	freeAll(args, env);
}

char			**Handler::setEnv(Client &client)
{
	char								**env;
	std::map<std::string, std::string> 	envMap;

	envMap["CONTENT_LENGTH"] = std::to_string(client.req.body.size());
	// envMap["CONTENT_TYPE"] = "text/html";
	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["PATH_INFO"] = client.req.uri;
	envMap["PATH_TRANSLATED"] = "";
	envMap["QUERY_STRING"] = client.req.uri.substr(client.req.uri.find('?') + 1);
	if (client.conf.find("exec") != client.conf.end())
		envMap["SCRIPT_NAME"] = client.conf["exec"];
	else
		envMap["SCRIPT_NAME"] = client.req.uri.substr(client.req.uri.find_last_of('/'));
	envMap["SERVER_NAME"] = "localhost";
	envMap["SERVER_PORT"] = "8080";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
	envMap["REQUEST_URI"] = client.req.uri;
	envMap["REQUEST_METHOD"] = client.req.method;

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