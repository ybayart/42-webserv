#include "Handler.hpp"

void	Handler::handleGet(Client &client)
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
			client.res.headers["Last-Modified"] = getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = findType(client.req);
		}
		client.res.headers["Date"] = getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, 4095);
		client.wBuf[bytes] = '\0';
		if (bytes == 0)
		{
			close(client.fileFd);
			client.status = DONE;
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
			client.res.headers["Last-Modified"] = getLastModified(client.conf["path"]);
			client.res.headers["Content-Type"] = findType(client.req);
		}
		client.res.headers["Date"] = getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		fillHeaders(client);
		client.status = DONE;
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
	}
	else if (client.status == HEADERS || client.status == CGI)
	{
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
		&& client.res.status_code == OK)
		{
			if (client.status != CGI)
				execCGI(client);
			client.status = CGI;
			parseBody(client);
		}
		else
		{
			fstat(client.fileFd, &file_info);
			if (client.res.status_code == OK)
			{
				client.res.headers["Last-Modified"] = getLastModified(client.conf["path"]);
				client.res.headers["Content-Type"] = findType(client.req);
			}
			client.res.headers["Date"] = getDate();
			client.res.headers["Server"] = "webserv";
			client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
			fillHeaders(client);
		}
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, 4095);
		client.wBuf[bytes] = '\0';
		if (bytes == 0)
		{
			close(client.fileFd);
			client.status = DONE;
		}
	}
}

void	Handler::handlePut(Client &client)
{
	int 			ret;
	std::string		path;
	struct stat	file_info;

	if (client.status == CODE)
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
		client.res.headers["Date"] = getDate();
		client.res.headers["Server"] = "webserv";
		fillHeaders(client);
		client.status = DONE;
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
		client.res.headers["Date"] = getDate();
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