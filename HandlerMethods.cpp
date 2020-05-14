#include "Handler.hpp"

void	Handler::handleGet(Request &req, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (req.conf["methods"].find(req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = req.conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || req.conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(req.fd, res);
	if (req.conf.find("CGI") != req.conf.end()
	&& req.uri.find(req.conf["CGI"]) != std::string::npos
	&& res.status_code == OK)
		execCGI(req);
	else
	{
		if (res.status_code == OK)
		{
			res.headers["Last-Modified"] = getLastModified(path);
			res.headers["Content-Type"] = findType(req);
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
		write(req.fd, toString(res).c_str(), toString(res).size());
	}
}

void	Handler::handleHead(Request &req, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (req.conf["methods"].find(req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = req.conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || req.conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(req.fd, res);
	if (res.status_code == OK)
	{
		res.headers["Last-Modified"] = getLastModified(path);
		res.headers["Content-Type"] = findType(req);
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
	write(req.fd, toString(res).c_str(), toString(res).size());
}

void	Handler::handlePost(Request &req, Response &res)
{
	int 			fd;
	std::string		path;
	int				ret;
	char			buf[4096];
	std::string		result;

	res.version = "HTTP/1.1";
	if (req.conf["methods"].find(req.method) == std::string::npos)
	{
		res.status_code = NOTALLOWED;
		fd = open("errorPages/405.html", O_RDONLY);
	}
	else
	{
		path = req.conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (fd == -1 || req.conf["isdir"] == "true")
		{
			res.status_code = NOTFOUND;
			fd = open("errorPages/404.html", O_RDONLY);
		}
		else
			res.status_code = OK;
	}
	writeStatus(req.fd, res);
	if (req.conf.find("CGI") != req.conf.end()
	&& req.uri.find(req.conf["CGI"]) != std::string::npos
	&& res.status_code == OK)
		execCGI(req);
	else
	{
		if (res.status_code == OK)
		{
			res.headers["Last-Modified"] = getLastModified(path);
			res.headers["Content-Type"] = findType(req);
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
		write(req.fd, toString(res).c_str(), toString(res).size());
	}
}

void	Handler::handlePut(Request &req, Response &res)
{
	int 			fd;
	int				len;
	std::string		path;

	res.version = "HTTP/1.1";
	if (req.conf["methods"].find(req.method) == std::string::npos)
		res.status_code = NOTALLOWED;
	else
	{
		path = req.conf["path"];
		fd = open(path.c_str(), O_RDONLY);
		if (req.conf["isdir"] == "true")
			res.status_code = NOTFOUND;
		else if (fd == -1)
			res.status_code = CREATED;
		else
			res.status_code = NOCONTENT;
		close(fd);
	}
	writeStatus(req.fd, res);
	res.headers["Date"] = getDate();
	res.headers["Server"] = "webserv";
	len = atoi(req.headers["Content-Length"].c_str());
	fd = open(req.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
	write(fd, req.body.c_str(), len);
	close(fd);
	write(req.fd, toString(res).c_str(), toString(res).size());
}