#include "Handler.hpp"

Handler::Handler()
{
	
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
		client.setReadState(false);
		client.setWriteState(true);
		if (request.method == "POST" || request.method == "PUT")
		{
			client.hasBody = true;
			if (request.method == "PUT")
				client.status = PARSING;
			if (request.method == "PUT"
			&& client.conf["isdir"] != "true"
			&& client.conf["methods"].find(request.method) != std::string::npos)
				client.fileFd = open(client.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
		}
		else
			client.status = CODE;
	}
	else
	{
		request.method = "BAD";
		client.status = CODE;
	}
	client.req = request;
	tmp = client.rBuf;
	tmp = tmp.substr(tmp.find("\r\n\r\n") + 4);
	strcpy(client.rBuf, tmp.c_str());
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
		write(client.fileFd, client.rBuf, bytes);
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
	if (ret <= 0)
	{
		if (ret == 0)
		{
			close(client.fileFd);
			client.status = DONE;
			found = false;
			done = 0;
		}
		return ;
	}
	if (done)
	{
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
		len = _helper.fromHexa(to_convert.c_str());
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
		if (elmt.find("auth") != elmt.end())
			client.conf["auth"] = elmt["auth"];
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
	env = _helper.setEnv(client);
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
	_helper.freeAll(args, env);
}