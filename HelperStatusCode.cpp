#include "Helper.hpp"

int			Helper::getStatusCode(Client &client)
{
	typedef int	(Helper::*ptr)(Client &client);
	std::map<std::string, ptr> map;

	map["GET"] = &Helper::GETStatus;
	map["HEAD"] = &Helper::GETStatus;
	map["PUT"] = &Helper::PUTStatus;
	map["POST"] = &Helper::POSTStatus;

	return ((this->*map[client.req.method])(client));
}

int			Helper::GETStatus(Client &client)
{
	std::string		credential;

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
	if (client.res.status_code == OK)
	{
		client.fileFd = open(client.conf["path"].c_str(), O_RDONLY);
		if (client.fileFd == -1 || client.conf["isdir"] == "true")
			client.res.status_code = NOTFOUND;
		else
			return (1);
	}
	return (0);
}

int			Helper::POSTStatus(Client &client)
{
	std::string		credential;

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
	if (client.res.status_code == OK && client.conf.find("max_body") != client.conf.end()
	&& client.req.body.size() > atoi(client.conf["max_body"].c_str()))
		client.res.status_code = REQTOOLARGE;
	if (client.res.status_code == OK)
	{
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
		&& client.conf.find("exec") != client.conf.end())
			client.fileFd = open(client.conf["exec"].c_str(), O_RDONLY);
		else
			client.fileFd = open(client.conf["path"].c_str(), O_RDONLY);
		if (client.fileFd == -1 || client.conf["isdir"] == "true")
			client.res.status_code = NOTFOUND;
		else
			return (1);
	}
	return (0);
}

int			Helper::PUTStatus(Client &client)
{
	int ret;

	client.res.version = "HTTP/1.1";
	if (client.conf["methods"].find(client.req.method) == std::string::npos)
		client.res.status_code = NOTALLOWED;
	else if (client.conf.find("max_body") != client.conf.end()
	&& client.req.body.size() > atoi(client.conf["max_body"].c_str()))
		client.res.status_code = REQTOOLARGE;
	else
	{
		ret = open(client.conf["path"].c_str(), O_RDONLY);
		if (client.conf["isdir"] == "true")
			client.res.status_code = NOTFOUND;
		else
		{ 
			if (ret == -1)
				client.res.status_code = CREATED;
			else
			{
				client.res.status_code = NOCONTENT;
				close(ret);
			}
			client.fileFd = open(client.conf["path"].c_str(), O_WRONLY | O_CREAT, 0666);
			return (1);
		}
	}
	return (0);
}