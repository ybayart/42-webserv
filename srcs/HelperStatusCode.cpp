#include "Helper.hpp"

int			Helper::getStatusCode(Client &client)
{
	typedef int	(Helper::*ptr)(Client &client);
	std::map<std::string, ptr> 	map;
	std::string					credential;
	int							ret;
	map["GET"] = &Helper::GETStatus;
	map["HEAD"] = &Helper::GETStatus;
	map["PUT"] = &Helper::PUTStatus;
	map["POST"] = &Helper::POSTStatus;
	map["CONNECT"] = &Helper::CONNECTStatus;
	map["TRACE"] = &Helper::TRACEStatus;
	map["OPTIONS"] = &Helper::OPTIONSStatus;
	map["DELETE"] = &Helper::DELETEStatus;

	if (client.req.method != "CONNECT"
		&& client.req.method != "TRACE"
		&& client.req.method != "OPTIONS")
	{
		client.res.version = "HTTP/1.1";
		client.res.status_code = OK;
		if (client.conf["methods"].find(client.req.method) == std::string::npos)
			client.res.status_code = NOTALLOWED;
		else if (client.conf.find("auth") != client.conf.end())
		{
			client.res.status_code = UNAUTHORIZED;
			if (client.req.headers.find("Authorization") != client.req.headers.end())
			{
				credential = decode64(client.req.headers["Authorization"].c_str());
				if (credential == client.conf["auth"])
					client.res.status_code = OK;
			}
		}
	}

	ret = (this->*map[client.req.method])(client);

	if (ret == 0)
		getErrorPage(client);
	return (ret);
}

int			Helper::GETStatus(Client &client)
{
	std::string		credential;
	struct stat		info;

	if (client.res.status_code == OK)
	{
		errno = 0;
		client.read_fd = open(client.conf["path"].c_str(), O_RDONLY);
		if (client.read_fd == -1 && errno == ENOENT)
			client.res.status_code = NOTFOUND;
		else if (client.read_fd == -1)
			client.res.status_code = INTERNALERROR;
		else
		{
			fstat(client.read_fd, &info);
			if (!S_ISDIR(info.st_mode)
			|| (S_ISDIR(info.st_mode) && client.conf["listing"] == "on"))
				return (1);
			else
				client.res.status_code = NOTFOUND;
		}
	}
	return (0);
}

int			Helper::POSTStatus(Client &client)
{
	std::string		credential;
	int				fd;
	struct stat		info;

	if (client.res.status_code == OK && client.conf.find("max_body") != client.conf.end()
	&& client.req.body.size() > (unsigned long)atoi(client.conf["max_body"].c_str()))
		client.res.status_code = REQTOOLARGE;
	if (client.res.status_code == OK)
	{
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos)
		{
			if (client.conf["exec"][0])
				client.read_fd = open(client.conf["exec"].c_str(), O_RDONLY);
			else
				client.read_fd = open(client.conf["path"].c_str(), O_RDONLY);
			fstat(client.read_fd, &info);
			if (client.read_fd == -1 || S_ISDIR(info.st_mode))
				client.res.status_code = INTERNALERROR;
			else
				return (1);
		}
		else
		{
			errno = 0;
			fd = open(client.conf["path"].c_str(), O_RDONLY);
			if (fd == -1 && errno == ENOENT)
				client.res.status_code = CREATED;
			else if (fd != -1)
				client.res.status_code = OK;
			close(fd);
			client.write_fd = open(client.conf["path"].c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
			if (client.write_fd == -1)
				client.res.status_code = INTERNALERROR;
			else
				return (1);
		}
	}
	return (0);
}

int			Helper::PUTStatus(Client &client)
{
	int 		fd;
	struct stat	info;
	int			save_err;


	if (client.res.status_code == OK && client.conf.find("max_body") != client.conf.end()
	&& client.req.body.size() > (unsigned long)atoi(client.conf["max_body"].c_str()))
		client.res.status_code = REQTOOLARGE;
	else if (client.res.status_code == OK)
	{
		errno = 0;
		fd = open(client.conf["path"].c_str(), O_RDONLY);
		save_err = errno;
		fstat(fd, &info);
		if (S_ISDIR(info.st_mode))
			client.res.status_code = NOTFOUND;
		else
		{ 
			if (fd == -1 && save_err == ENOENT)
				client.res.status_code = CREATED;
			else if (fd == -1)
			{
				client.res.status_code = INTERNALERROR;
				return (0);
			}
			else
			{
				client.res.status_code = NOCONTENT;
				if (close(fd) == -1)
				{
					client.res.status_code = INTERNALERROR;
					return (0);
				}
			}
			client.write_fd = open(client.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
			if (client.write_fd == -1)
			{
				client.res.status_code = INTERNALERROR;
				return (0);
			}
			return (1);
		}
	}
	return (0);
}

int			Helper::CONNECTStatus(Client &client)
{
	client.res.version = "HTTP/1.1";
	client.res.status_code = NOTIMPLEMENTED;
	return (0);
}

int			Helper::TRACEStatus(Client &client)
{
	client.res.version = "HTTP/1.1";
	if (client.conf["methods"].find(client.req.method) == std::string::npos)
	{
		client.res.status_code = NOTALLOWED;
		return (0);
	}
	else
	{
		client.res.status_code = OK;
		return (1);
	}
}

int			Helper::OPTIONSStatus(Client &client)
{
	client.res.version = "HTTP/1.1";
	client.res.status_code = NOCONTENT;
	return (1);
}

int			Helper::DELETEStatus(Client &client)
{
	int 		fd;
	struct stat	info;
	int			save_err;

	if (client.res.status_code == OK)
	{
		errno = 0;
		fd = open(client.conf["path"].c_str(), O_RDONLY);
		save_err = errno;
		fstat(fd, &info);
		if ((fd == -1 && save_err == ENOENT) || S_ISDIR(info.st_mode))
			client.res.status_code = NOTFOUND;
		else if (fd == -1)
			client.res.status_code = INTERNALERROR;
		else
			return (1);
	}
	return (0);
}
