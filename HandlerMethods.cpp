#include "Handler.hpp"

void	Handler::dispatcher(Client &client)
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

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	std::string	credential;

	switch (client.status)
	{
		case Client::CODE:
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			fstat(client.read_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.conf["listing"] == "on")
			{
				close(client.read_fd);
				client.read_fd = -1;
				createListing(client);
			}
			else if (client.res.status_code == NOTFOUND)
				negotiate(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				close(client.read_fd);
				client.read_fd = -1;
				execCGI(client);
				client.status = Client::CGI;
			}
			else
				client.status = Client::HEADERS;
			if (client.read_fd != -1)
				client.setFileToRead(client.read_fd, true);	
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
				client.res.headers["Content-Type"] = _helper.findType(client.req);
			if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			client.res.headers["Date"] = _helper.getDate();
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
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			if (client.res.status_code == NOTFOUND)
				negotiate(client);
			fstat(client.read_fd, &file_info);
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
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				close(client.read_fd);
				client.read_fd = -1;
				execCGI(client);
				client.status = Client::CGI;
			}
			else 
				client.status = Client::HEADERS;
			client.setFileToRead(client.read_fd, true);
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
				client.res.headers["WWW-Authenticate"] = "Basic realm=\"webserv\"";
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.headers.find("Content-Type") == client.res.headers.end() && client.read_fd != -1)
				client.res.headers["Content-Type"] = _helper.findType(client.req);
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
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
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			else
				client.setFileToWrite(client.write_fd, true);
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == CREATED || client.res.status_code == NOCONTENT)
			{
				client.res.headers["Location"] = client.conf["path"];
				if (client.res.status_code == CREATED)
				{
					client.res.body = "Ressource created\n";
					client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
				}

			}
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.write_fd == -1)
			{
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
			client.setFileToRead(client.read_fd, true);
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			client.res.headers["Content-Length"] = std::to_string(client.res.body.size());
			client.status = Client::BODY;
			break ;
		case Client::BODY:
			if (client.read_fd == -1)
			{
				createResponse(client);
				client.status = Client::RESPONSE;
			}
			break ;
	}
}