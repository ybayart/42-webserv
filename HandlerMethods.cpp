#include "Handler.hpp"

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;
	std::string	credential;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("errorPages/405.html", O_RDONLY);
		}
		else if (client.conf.find("auth") != client.conf.end())
		{
			client.res.status_code = UNAUTHORIZED;
			client.fileFd = open("errorPages/401.html", O_RDONLY);
			if (client.req.headers.find("Authorization") != client.req.headers.end())
			{
				credential = _helper.decode64(client.req.headers["Authorization"].c_str());
				if (credential == client.conf["auth"])
					client.res.status_code = OK;
			}
		}
		if (client.res.status_code != NOTALLOWED && client.res.status_code != UNAUTHORIZED)
		{
			fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == OK)
		{
			client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		}
		else if (client.res.status_code == UNAUTHORIZED)
			client.res.headers["WWW-Authenticate"] = "Basic realm=\"webserv\"";
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE - 1);
		client.wBuf[bytes] = '\0';
		if (bytes == 0)
		{
			close(client.fileFd);
			client.setToStandBy();
		}
	}
}

void	Handler::handleHead(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("errorPages/405.html", O_RDONLY);
		}
		else
		{
			fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == OK)
		{
			client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		}
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
		client.setToStandBy();
	}
}

void	Handler::handlePost(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
		{
			client.res.status_code = NOTALLOWED;
			client.fileFd = open("errorPages/405.html", O_RDONLY);
		}
		else
		{
			if (client.conf.find("CGI") != client.conf.end()
			&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
			&& client.conf.find("exec") != client.conf.end())
				fd = open(client.conf["exec"].c_str(), O_RDONLY);
			else
				fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 || client.conf["isdir"] == "true")
			{
				client.res.status_code = NOTFOUND;
				client.fileFd = open("errorPages/404.html", O_RDONLY);
			}
			else
			{
				client.res.status_code = OK;
				client.fileFd = fd;
			}
		}
		fillStatus(client);
		client.status = PARSING;
	}
	else if (client.status == PARSING)
	{
		client.setReadState(true);
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
		&& client.res.status_code == OK)
		{
			if (client.status != CGI)
			{
				client.res.headers["Date"] = _helper.getDate();
				execCGI(client);
			}
			client.status = CGI;
		}
		parseBody(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == OK)
		{
			client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		}
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE - 1);
		client.wBuf[bytes] = '\0';
		if (bytes <= 0)
		{
			close(client.fileFd);
			client.setToStandBy();
		}
	}
}

void	Handler::handlePut(Client &client)
{
	int 			ret;
	std::string		path;
	struct stat	file_info;

	if (client.status == PARSING)
		client.setReadState(true);
	else if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
			client.res.status_code = NOTALLOWED;
		else
		{
			ret = lstat(client.conf["path"].c_str(), &file_info);
			if (client.conf["isdir"] == "true")
				client.res.status_code = NOTFOUND;
			else
				client.res.status_code = NOCONTENT;
		}
		fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		fillHeaders(client);
		client.setToStandBy();
	}
}

void	Handler::handleBadRequest(Client &client)
{
	int				bytes;
	std::string		result;
	struct stat		file_info;
	int				i;
	std::map<std::string, std::string>::const_iterator b;

	if (client.status != BODY)
	{
		client.fileFd = open("errorPages/400.html", O_RDONLY);
		fstat(client.fileFd, &file_info);
		client.res.version = "HTTP/1.1";
		client.res.status_code = BADREQUEST;
		result = client.res.version + " " + client.res.status_code + "\n";
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		b = client.res.headers.begin();
		while (b != client.res.headers.end())
		{
			if (b->second != "")
				result += b->first + ": " + b->second + "\n";
			++b;
		}
		result += "\n";
		i = 0;
		while (result[i])
		{
			client.wBuf[i] = result[i];
			++i;	
		}
		client.wBuf[i] = '\0';
		client.status = BODY;
	}
	else
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE - 1);
		client.wBuf[bytes] = '\0';
		close(client.fileFd);
		client.status = DONE;
		client.setWriteState(false);
	}
}