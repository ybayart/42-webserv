#include "Handler.hpp"

void	Handler::handleGet(Client &client, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (client._conf["methods"].find(client._req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = client._conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || client._conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(client._fd, res);
	if (client._conf.find("CGI") != client._conf.end()
	&& client._req.uri.find(client._conf["CGI"]) != std::string::npos
	&& res.status_code == OK)
		execCGI(client);
	else
	{
		if (res.status_code == OK)
		{
			res.headers["Last-Modified"] = getLastModified(path);
			res.headers["Content-Type"] = findType(client._req);
		}
		res.headers["Date"] = getDate();
		res.headers["Server"] = "webserv";
		while ((ret = read(fd, buf, 4095)) > 0)
		{
			buf[ret] = '\0';
			result += buf;
		}
		close(fd);
		res.headers["Content-Length"] = std::to_string(result.size());
		res.body = result;
		write(client._fd, toString(res).c_str(), toString(res).size());
	}
}

void	Handler::handleHead(Client &client, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (client._conf["methods"].find(client._req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = client._conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || client._conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(client._fd, res);
	if (res.status_code == OK)
	{
		res.headers["Last-Modified"] = getLastModified(path);
		res.headers["Content-Type"] = findType(client._req);
	}
	res.headers["Date"] = getDate();
	res.headers["Server"] = "webserv";
	while ((ret = read(fd, buf, 4095)) > 0)
	{
		buf[ret] = '\0';
		result += buf;
	}
	close(fd);
	res.headers["Content-Length"] = std::to_string(result.size());
	write(client._fd, toString(res).c_str(), toString(res).size());
}

void	Handler::handlePost(Client &client, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (client._conf["methods"].find(client._req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = client._conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || client._conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(client._fd, res);
	if (client._conf.find("CGI") != client._conf.end()
	&& client._req.uri.find(client._conf["CGI"]) != std::string::npos
	&& res.status_code == OK)
		execCGI(client);
	else
	{
		if (res.status_code == OK)
		{
			res.headers["Last-Modified"] = getLastModified(path);
			res.headers["Content-Type"] = findType(client._req);
		}
		res.headers["Date"] = getDate();
		res.headers["Server"] = "webserv";
		while ((ret = read(fd, buf, 4095)) > 0)
		{
			buf[ret] = '\0';
			result += buf;
		}
		close(fd);
		res.headers["Content-Length"] = std::to_string(result.size());
		res.body = result;
		write(client._fd, toString(res).c_str(), toString(res).size());
	}
}

void	Handler::handlePut(Client &client, Response &res)
{
	int 			fd;
	int				len;
	std::string		path;

	res.version = "HTTP/1.1";
	if (client._conf["methods"].find(client._req.method) == std::string::npos)
		res.status_code = NOTALLOWED;
	else
	{
		path = client._conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (client._conf["isdir"] == "true")
			res.status_code = NOTFOUND;
		else if (fd == -1)
			res.status_code = CREATED;
		else
			res.status_code = NOCONTENT;
		close(fd);
	}
	writeStatus(client._fd, res);
	res.headers["Date"] = getDate();
	res.headers["Server"] = "webserv";
	len = atoi(client._req.headers["Content-Length"].c_str());
	fd = open(client._conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
	write(fd, client._req.body.c_str(), len);
	close(fd);
	write(client._fd, toString(res).c_str(), toString(res).size());
}