#include "utils.h"
#include "Handler.hpp"

Handler::Handler()
{
	
}

Handler::~Handler()
{
	
}

void			Handler::parseRequest(Client &client, std::vector<config> &conf)
{
	Request				request;
	std::string			tmp;
	std::string			buffer;

	buffer = std::string(client.rBuf);
	if (buffer[0] == '\r')
		buffer.erase(buffer.begin());
	if (buffer[0] == '\n')
		buffer.erase(buffer.begin());
	ft::getline(buffer, request.method, ' ');
	ft::getline(buffer, request.uri, ' ');
	ft::getline(buffer, request.version);
	if (parseHeaders(buffer, request))
		request.valid = checkSyntax(request);
	if (request.uri != "*" || request.method != "OPTIONS")
		getConf(client, request, conf);
	if (request.valid)
	{
		if (client.conf["root"][0] != '\0')
			chdir(client.conf["root"].c_str());
		if (request.method == "POST" || request.method == "PUT")
			client.status = Client::BODYPARSING;
		else
			client.status = Client::CODE;
	}
	else
	{
		request.method = "BAD";
		client.status = Client::CODE;
	}
	client.req = request;
	tmp = client.rBuf;
	tmp = tmp.substr(tmp.find("\r\n\r\n") + 4);
	strcpy(client.rBuf, tmp.c_str());
}

bool			Handler::parseHeaders(std::string &buf, Request &req)
{
	size_t		pos;
	std::string	line;
	std::string	key;
	std::string	value;

	while (!buf.empty())
	{
		ft::getline(buf, line);
		if (line.size() < 1 || line[0] == '\n' || line[0] == '\r')
			break ;
		if (line.find(':') != std::string::npos)
		{
			pos = line.find(':');
			key = line.substr(0, pos);
			if (line[pos + 1] == ' ')
				value = line.substr(pos + 2);
			else
				value = line.substr(pos + 1);
			if (ft::isspace(value[0]) || ft::isspace(key[0]) || value.empty() || key.empty())
			{
				req.valid = false;
				return (false);
			}
 			req.headers[key] = value;
 			req.headers[key].pop_back(); //remove '\r'
		}
		else
		{
			req.valid = false;
			return (false);
		}
	}
	return (true);
}

void			Handler::parseBody(Client &client)
{
	if (client.req.headers.find("Content-Length") != client.req.headers.end())
		getBody(client);
	else if (client.req.headers["Transfer-Encoding"] == "chunked")
		dechunkBody(client);
	else
	{
		client.req.method = "BAD";
		client.status = Client::CODE;
	}
	if (client.status == Client::CODE)
		g_logger.log("body size parsed from " + client.ip + ":" + std::to_string(client.port) + ": " + std::to_string(client.req.body.size()), MED);
}

void			Handler::getBody(Client &client)
{
	unsigned int	bytes;

	if (client.chunk.len == 0)
		client.chunk.len = atoi(client.req.headers["Content-Length"].c_str());
	if (client.chunk.len < 0)
	{
		client.req.method = "BAD";
		client.status = Client::CODE;
		return ;
	}
	bytes = strlen(client.rBuf);
	if (bytes >= client.chunk.len)
	{
		memset(client.rBuf + client.chunk.len, 0, BUFFER_SIZE - client.chunk.len);
		client.req.body += client.rBuf;
		client.chunk.len = 0;
		client.status = Client::CODE;
	}
	else
	{
		client.chunk.len -= bytes;
		client.req.body += client.rBuf;
		memset(client.rBuf, 0, BUFFER_SIZE + 1);
	}
}

void			Handler::dechunkBody(Client &client)
{
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
		client.status = Client::CODE;
		client.chunk.found = false;
		client.chunk.done = false;
		return ;
	}
}

void			Handler::getConf(Client &client, Request &req, std::vector<config> &conf)
{
	std::map<std::string, std::string> elmt;
	std::string		tmp;
	std::string 	file;
	struct stat		info;
	config			to_parse;

	if (!req.valid)
	{
		client.conf["error"] = conf[0]["server|"]["error"];
		return ;
	}
	std::vector<config>::iterator it(conf.begin());
	while (it != conf.end())
	{
		if (req.headers["Host"] == (*it)["server|"]["server_name"])
		{
			to_parse = *it;
			break ;
		}
		++it;
	}
	if (it == conf.end())
		to_parse = conf[0];
	file = req.uri.substr(req.uri.find_last_of('/') + 1, req.uri.find('?'));
	tmp = req.uri;
	do
	{
		if (to_parse.find("server|location " + tmp + "|") != to_parse.end())
		{
			elmt = to_parse["server|location " + tmp + "|"];
			break ;
		}
		tmp = tmp.substr(0, tmp.find_last_of('/'));
	} while (tmp != "");
	if (elmt.size() == 0)
		if (to_parse.find("server|location /|") != to_parse.end())
			elmt = to_parse["server|location /|"];
	if (elmt.size() > 0)
	{
		client.conf = elmt;
		client.conf["path"] = req.uri.substr(0, req.uri.find("?"));
		if (elmt.find("root") != elmt.end())
			client.conf["path"].replace(0, tmp.size(), elmt["root"]);
	}
	for (std::map<std::string, std::string>::iterator it(to_parse["server|"].begin()); it != to_parse["server|"].end(); ++it)
	{
		if (client.conf.find(it->first) == client.conf.end())
			client.conf[it->first] = it->second;
	}
	lstat(client.conf["path"].c_str(), &info);
	if (S_ISDIR(info.st_mode))
		if (client.conf["index"][0] && client.conf["listing"] != "on")
			client.conf["path"] += "/" + elmt["index"];
	if (req.method == "GET")
		client.conf["savedpath"] = client.conf["path"];
	g_logger.log("path requested from " + client.ip + ":" + std::to_string(client.port) + ": " + client.conf["path"], MED);
}

void			Handler::negotiate(Client &client)
{
	std::multimap<std::string, std::string> 	languageMap;
	std::multimap<std::string, std::string> 	charsetMap;
	int				fd = -1;
	std::string		path;
	std::string		ext;

	if (client.req.headers.find("Accept-Language") != client.req.headers.end())
		_helper.parseAcceptLanguage(client, languageMap);
	if (client.req.headers.find("Accept-Charset") != client.req.headers.end())
		_helper.parseAcceptCharset(client, charsetMap);
	if (!languageMap.empty())
	{
		for (std::multimap<std::string, std::string>::reverse_iterator it(languageMap.rbegin()); it != languageMap.rend(); ++it)
		{
			if (!charsetMap.empty())
			{
				for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
				{
					ext = it->second + "." + it2->second;
					path = client.conf["savedpath"] + "." + ext;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
					{
						client.res.headers["Content-Language"] = it->second;
						break ;
					}
					ext = it2->second + "." + it->second;
					path = client.conf["savedpath"] + "." + ext;
					fd = open(path.c_str(), O_RDONLY);
					if (fd != -1)
					{
						client.res.headers["Content-Language"] = it->second;
						break ;
					}
				}
			}
			else
			{
				ext = it->second;
				path = client.conf["savedpath"] + "." + ext;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
				{
					client.res.headers["Content-Language"] = it->second;
					break ;
				}
			}
		}
	}
	else if (languageMap.empty())
	{
		if (!charsetMap.empty())
		{
			for (std::multimap<std::string, std::string>::reverse_iterator it2(charsetMap.rbegin()); it2 != charsetMap.rend(); ++it2)
			{
				ext = it2->second;
				path = client.conf["savedpath"] + "." + it2->second;
				fd = open(path.c_str(), O_RDONLY);
				if (fd != -1)
					break ;
			}
		}
	}
	if (fd != -1)
	{
		client.conf["path"] = path;
		client.res.headers["Content-Location"] = client.req.uri + "." + ext;
		if (client.read_fd != -1)
			close(client.read_fd);
		client.read_fd = fd;
		client.res.status_code = OK;
	}
}

void			Handler::createListing(Client &client)
{
	DIR				*dir;
	struct dirent	*cur;

	close(client.read_fd);
	client.read_fd = -1;
	dir = opendir(client.conf["path"].c_str());
	client.res.body = "<html>\n<body>\n";
	client.res.body += "<h1>Directory listing</h1>\n";
	while ((cur = readdir(dir)) != NULL)
	{
		if (cur->d_name[0] != '.')
		{
			client.res.body += "<a href=\"" + client.req.uri;
			if (client.req.uri != "/")
				client.res.body += "/";
			client.res.body += cur->d_name;
			client.res.body += "\">";
			client.res.body += cur->d_name;
			client.res.body += "</a><br>\n";
		}
	}
	closedir(dir);
	client.res.body += "</body>\n</html>\n";
}

//TO COMPLETE
bool			Handler::checkSyntax(const Request &req)
{
	if (req.method.size() == 0 || req.uri.size() == 0
		|| req.version.size() == 0)
		return (false);
	if (req.method != "GET" && req.method != "POST"
		&& req.method != "HEAD" && req.method != "PUT"
		&& req.method != "CONNECT" && req.method != "TRACE"
		&& req.method != "OPTIONS" && req.method != "DELETE")
		return (false);
	if (req.method != "OPTIONS" && req.uri[0] != '/') //OPTIONS can have * as uri
		return (false);
	if (req.version != "HTTP/1.1\r" && req.version != "HTTP/1.1")
		return (false);
	if (req.headers.find("Host") == req.headers.end())
		return (false);
	return (true);
}

void			Handler::execCGI(Client &client)
{
	char			**args = NULL;
	char			**env = NULL;
	std::string		path;
	int				ret;
	int				tubes[2];

	if (client.conf["php"][0] && client.conf["path"].find(".php") != std::string::npos)
		path = client.conf["php"];
	else if (client.conf["exec"][0])
		path = client.conf["exec"];
	else
		path = client.conf["path"];
	close(client.read_fd);
	client.read_fd = -1;	
	args = (char **)(malloc(sizeof(char *) * 3));
	args[0] = strdup(path.c_str());
	args[1] = strdup(client.conf["path"].c_str());
	args[2] = NULL;
	env = _helper.setEnv(client);
	client.tmp_fd = open(TMP_PATH, O_WRONLY | O_CREAT, 0666);
	pipe(tubes);
	g_logger.log("executing CGI for " + client.ip + ":" + std::to_string(client.port), MED);
	if ((client.cgi_pid = fork()) == 0)
	{
		close(tubes[1]);
		dup2(tubes[0], 0);
		dup2(client.tmp_fd, 1);
		errno = 0;
		ret = execve(path.c_str(), args, env);
		if (ret == -1)
		{
			std::cerr << "Error with CGI: " << strerror(errno) << std::endl;
			exit(1);
		}
	}
	else
	{
		close(tubes[0]);
		client.write_fd = tubes[1];
		client.read_fd = open(TMP_PATH, O_RDONLY);
		client.setFileToWrite(true);
	}
	ft::freeAll(args, env);
}

void		Handler::parseCGIResult(Client &client)
{
	size_t			pos;
	std::string		headers;
	std::string		key;
	std::string		value;

	if (client.res.body.find("\r\n\r\n") == std::string::npos)
		return ;
	headers = client.res.body.substr(0, client.res.body.find("\r\n\r\n") + 1);
	pos = headers.find("Status");
	if (pos != std::string::npos)
	{
		client.res.status_code.clear();
		pos += 8;
		while (headers[pos] != '\r')
		{
			client.res.status_code += headers[pos];
			pos++;
		}
	}
	pos = 0;
	while (headers[pos])
	{
		while (headers[pos] && headers[pos] != ':')
		{
			key += headers[pos];
			++pos;
		}
		++pos;
		while (headers[pos] && headers[pos] != '\r')
		{
			value += headers[pos];
			++pos;
		}
		client.res.headers[key] = value;
		key.clear();
		value.clear();
		if (headers[pos] == '\r')
			pos++;
		if (headers[pos] == '\n')
			pos++;
	}
	pos = client.res.body.find("\r\n\r\n") + 4;
	client.res.body = client.res.body.substr(pos);
	client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
}

void		Handler::createResponse(Client &client)
{
	std::map<std::string, std::string>::const_iterator b;

	client.response = client.res.version + " " + client.res.status_code + "\r\n";
	b = client.res.headers.begin();
	while (b != client.res.headers.end())
	{
		if (b->second != "")
			client.response += b->first + ": " + b->second + "\r\n";
		++b;
	}
	client.response += "\r\n";
	client.response += client.res.body;
	client.res.clear();
}
