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
	getConf(client, request, conf);
	if (request.valid)
	{
		if (client.conf["root"][0] != '\0')
			chdir(client.conf["root"].c_str());
		if (request.method == "POST" || request.method == "PUT")
			client.hasBody = true;
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
	if (client.req.headers.find("Content-Length") != client.req.headers.end())
		getBody(client);
	else if (client.req.headers.find("Transfer-Encoding") != client.req.headers.end()
	&& client.req.headers["Transfer-Encoding"] == "chunked\r")
		dechunkBody(client);
	else
	{
		client.req.method = "BAD";
		client.status = CODE;
	}
}

void			Handler::getBody(Client &client)
{
	int			to_read;
	int			ret;
	int			bytes;

	to_read = atoi(client.req.headers["Content-Length"].c_str());
	bytes = strlen(client.rBuf);
	if (bytes >= to_read)
	{
		memset(client.rBuf + to_read, 0, BUFFER_SIZE - to_read);
		std::cout << "b: " << client.rBuf << std::endl;
		client.req.body = client.rBuf;
		client.hasBody = false;
		client.status = CODE;
	}
}

void			Handler::dechunkBody(Client &client)
{
	int				bytes;
	int				ret;

	if (strstr(client.rBuf, "\r\n") && client.chunk.found == false)
	{
		client.chunk.len = _helper.findLen(client);
		if (client.chunk.len == 0)
			client.chunk.done = true;
		else
			client.chunk.found = true;
	}
	else if (client.chunk.found == true)
		_helper.fillBody(client);
	if (client.chunk.done)
	{
		memset(client.rBuf, 0, BUFFER_SIZE + 1);
		client.hasBody = false;
		client.status = CODE;
		client.chunk.found = false;
		client.chunk.done = false;
		return ;
	}
}

void			Handler::getConf(Client &client, Request &req, Config &conf)
{
	std::map<std::string, std::string> elmt;
	std::string		tmp;
	std::string 	file;
	struct stat		info;

	if (!req.valid)
	{
		client.conf["error"] = conf._elmts["server|"]["error"];
		return ;
	}
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
		client.conf = elmt;
		if (elmt.find("root") != elmt.end())
			client.conf["path"] = req.uri.replace(0, tmp.size(), elmt["root"]);
		else
			client.conf["path"] = req.uri;
	}
	lstat(client.conf["path"].c_str(), &info);
	if (S_ISDIR(info.st_mode))
	{
		if (elmt.find("index") != elmt.end())
			client.conf["path"] += "/" + elmt["index"];
		else
			client.conf["isdir"] = "true";
	}
	for (std::map<std::string, std::string>::iterator it(conf._elmts["server|"].begin()); it != conf._elmts["server|"].end(); ++it)
	{
		if (client.conf.find(it->first) == client.conf.end())
			client.conf[it->first] = it->second;
	}
	// std::cout << "p: " << client.conf["path"] << std::endl;
}

void			Handler::negotiate(Client &client)
{
	std::multimap<std::string, std::string> 	languageMap;
	std::multimap<std::string, std::string> 	charsetMap;
	int									fd = -1;
	std::string							path;

	if (client.req.headers.find("Accept-Language") != client.req.headers.end())
		_helper.parseAcceptLanguage(client, languageMap);
	if (client.req.headers.find("Accept-Charset") != client.req.headers.end())
		_helper.parseAcceptCharsets(client, charsetMap);
	if (!languageMap.empty())
	{
		for (std::multimap<std::string, std::string>::reverse_iterator it(languageMap.rbegin()); it != languageMap.rend(); ++it)
		{
			if (!charsetMap.empty())
			{
				for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
				{
					path = client.conf["path"] + "." + it->second + "." + it2->second;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
						break ;
					path = client.conf["path"] + "." + it2->second + "." + it->second;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
						break ;
				}
			}
			else
			{
				path = client.conf["path"] + "." + it->second;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
					break ;
			}
			if (fd != -1)
				break ;
		}
	}
	else if (languageMap.empty())
	{
		if (!charsetMap.empty())
		{
			for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
			{
				path = client.conf["path"] + "." + it2->second;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
					break ;
			}
		}
	}
	if (fd != -1)
	{
		client.conf["path"] = path;
		if (client.fileFd != -1)
			close(client.fileFd);
		client.fileFd = fd;
		client.res.status_code = OK;
	}
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
	int				file_tmp;

	if (client.conf.find("exec") != client.conf.end())
		path = client.conf["exec"];
	else
		path = client.conf["path"];
	args = (char **)(malloc(sizeof(char *) * 3));
	args[0] = strdup(path.c_str());
	args[1] = strdup(client.conf["path"].c_str());
	args[2] = NULL;
	env = _helper.setEnv(client);
	client.tmp_path = "/tmp/cgi" + client.lastDate.substr(client.lastDate.size() - 7, 2) + ".tmp";
	// std::cout << "tmp: " << client.tmp_path << "\n";
	file_tmp = open(client.tmp_path.c_str(), O_WRONLY | O_CREAT, 0666);
	pipe(tubes);
	if ((client.pid = fork()) == 0)
	{
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(file_tmp, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
			std::cout << "Error with CGI: " << strerror(errno) << std::endl;
	}
	else
	{
		close(tubes[0]);
		bytes = write(tubes[1], client.req.body.c_str(), client.req.body.size());
		close(file_tmp);
		close(tubes[1]);
		// std::cout << "sent "<< bytes << " bytes to cgi\n";
	}
	_helper.freeAll(args, env);
}

bool		Handler::readCGIResult(Client &client)
{
	char			buffer[BUFFER_SIZE + 1];
	int				bytes;

	if (waitpid(client.pid, NULL, WNOHANG) == 0)
		return (false);
	if (client.fileFd == -1)
		client.fileFd = open(client.tmp_path.c_str(), O_RDONLY);
	bytes = read(client.fileFd, buffer, BUFFER_SIZE);
	buffer[bytes] = '\0';
	client.file_str += buffer;
	if (bytes == 0)
	{
		close(client.fileFd);
		client.fileFd = -1;
		unlink(client.tmp_path.c_str());
		return (true);
	}
	else
		return (false);
}

void		Handler::parseCGIResult(Client &client)
{
	int				pos;

	pos = client.file_str.find("Status");
	if (pos != std::string::npos)
	{
		client.res.status_code.clear();
		pos += 8;
		while (client.file_str[pos] != '\r')
		{
			client.res.status_code += client.file_str[pos];
			pos++;
		}
	}
	pos = client.file_str.find("Content-Type");
	if (pos != std::string::npos)
	{
		client.res.headers["Content-Type"].clear();
		pos += 14;
		while (client.file_str[pos] != '\r')
		{
			client.res.headers["Content-Type"] += client.file_str[pos];
			pos++;
		}
	}
	pos = client.file_str.find("\r\n\r\n");
	pos += 4;
	client.file_str = client.file_str.substr(pos);
	// std::cout << "size: " << client.file_str.size() << std::endl;
	client.res.headers["Content-Length"] = std::to_string(client.file_str.size());
}