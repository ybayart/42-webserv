#include <stdlib.h>
#include "utils.h"
#include "Handler.hpp"

void	Handler::dispatcher(Client &client)
{
	typedef void	(Handler::*ptr)(Client &client);
	std::map<std::string, ptr> map;

	map["GET"] = &Handler::handleGet;
	map["HEAD"] = &Handler::handleHead;
	map["PUT"] = &Handler::handlePut;
	map["POST"] = &Handler::handlePost;
	map["CONNECT"] = &Handler::handleConnect;
	map["TRACE"] = &Handler::handleTrace;
	map["OPTIONS"] = &Handler::handleOptions;
	map["DELETE"] = &Handler::handleDelete;
	map["BAD"] = &Handler::handleBadRequest;

	(this->*map[client.req.method])(client);
}

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	std::string	credential;

	switch (client.status)
	{
		case Client::CODE:
			_helper.getStatusCode(client);
			fstat(client.read_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.conf["listing"] == "on")
				createListing(client);
			if (client.res.status_code == NOTFOUND)
				negotiate(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				execCGI(client);
				client.status = Client::CGI;
			}
			else
				client.status = Client::HEADERS;
			client.setFileToRead(true);	
			break ;
		case Client::CGI:
			if (client.read_fd == -1)
			{
				parseCGIResult(client);
				client.status = Client::HEADERS;
			}
			break ;
		case Client::HEADERS:
			lstat(client.conf["path"].c_str(), &file_info);
			if (!S_ISDIR(file_info.st_mode))
				client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
			if (client.res.headers["Content-Type"][0] == '\0')
				client.res.headers["Content-Type"] = _helper.findType(client);
			if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			else if (client.res.status_code == NOTALLOWED)
				client.res.headers["Allow"] = client.conf["methods"];
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break;
	}
}

void	Handler::handleHead(Client &client)
{
	struct stat	file_info;

	switch (client.status)
	{
		case Client::CODE:
			_helper.getStatusCode(client);
			fstat(client.read_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.conf["listing"] == "on")
				createListing(client);
			else if (client.res.status_code == NOTFOUND)
				negotiate(client);
			fstat(client.read_fd, &file_info);
			if (client.res.status_code == OK)
			{
				client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
				client.res.headers["Content-Type"] = _helper.findType(client);
			}
			else if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			else if (client.res.status_code == NOTALLOWED)
				client.res.headers["Allow"] = client.conf["methods"];
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
			createResponse(client);
			client.status = Client::RESPONSE;
			break ;
	}
}

void	Handler::handlePost(Client &client)
{
	switch (client.status)
	{
		case Client::BODYPARSING:
			parseBody(client);
			break ;
		case Client::CODE:
			_helper.getStatusCode(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				execCGI(client);
				client.status = Client::CGI;
				client.setFileToRead(true);
			}
			else
			{
				if (client.res.status_code == OK || client.res.status_code == CREATED)
					client.setFileToWrite(true);
				else
					client.setFileToRead(true);
				client.status = Client::HEADERS;
			}
			break ;
		case Client::CGI:
			if (client.read_fd == -1)
			{
				parseCGIResult(client);
				client.status = Client::HEADERS;
			}
			break ;
		case Client::HEADERS:
			if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			else if (client.res.status_code == NOTALLOWED)
				client.res.headers["Allow"] = client.conf["methods"];
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == CREATED)
				client.res.body = "File created\n";
			else if (client.res.status_code == OK && !((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos)))
				client.res.body = "File modified\n";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1 && client.write_fd == -1)
			{
				if (client.res.headers["Content-Length"][0] == '\0')
					client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}

void	Handler::handlePut(Client &client)
{
	std::string		path;
	std::string		body;

	switch (client.status)
	{
		case Client::BODYPARSING:
			parseBody(client);
			break ;
		case Client::CODE:
			if (_helper.getStatusCode(client))
				client.setFileToWrite(true);
			else
				client.setFileToRead(true);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == CREATED || client.res.status_code == NOCONTENT)
			{
				client.res.headers["Location"] = client.req.uri;
				if (client.res.status_code == CREATED)
					client.res.body = "Ressource created\n";
			}
			if (client.res.status_code == NOTALLOWED)
				client.res.headers["Allow"] = client.conf["methods"];
			else if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.write_fd == -1 && client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}

void	Handler::handleConnect(Client &client)
{
	switch (client.status)
	{
		case Client::CODE:
			_helper.getStatusCode(client);
			client.setFileToRead(true);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}

void	Handler::handleTrace(Client &client)
{
	switch (client.status)
	{
		case Client::CODE:
			_helper.getStatusCode(client);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == OK)
			{
				client.res.headers["Content-Type"] = "message/http";
				client.res.body = client.req.method + " " + client.req.uri + " " + client.req.version + "\r\n";
				for (std::map<std::string, std::string>::iterator it(client.req.headers.begin());
					it != client.req.headers.end(); ++it)
				{
					client.res.body += it->first + ": " + it->second + "\r\n";
				}
			}
			else
				client.setFileToRead(true);
			client.status = Client::BODY;	
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}

void	Handler::handleOptions(Client &client)
{
	switch (client.status)
	{
		case Client::CODE:
			_helper.getStatusCode(client);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			if (client.req.uri != "*")
				client.res.headers["Allow"] = client.conf["methods"];
			createResponse(client);
			client.status = Client::RESPONSE;
			break ;
	}
}

void	Handler::handleDelete(Client &client)
{
	switch (client.status)
	{
		case Client::CODE:
			std::cout << "here\n";
			if (!_helper.getStatusCode(client))
				client.setFileToRead(true);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == OK)
			{
				unlink(client.conf["path"].c_str());				
				client.res.body = "File deleted\n";
			}
			else if (client.res.status_code == NOTALLOWED)
				client.res.headers["Allow"] = client.conf["methods"];
			else if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}

void	Handler::handleBadRequest(Client &client)
{
	struct stat		file_info;

	switch (client.status)
	{
		case Client::CODE:
			client.res.version = "HTTP/1.1";
			client.res.status_code = BADREQUEST;
			_helper.getErrorPage(client);
			fstat(client.read_fd, &file_info);
			client.setFileToRead(true);
			client.res.headers["Date"] = ft::getDate();
			client.res.headers["Server"] = "webserv";
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}