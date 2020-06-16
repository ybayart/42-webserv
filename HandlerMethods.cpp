#include "Handler.hpp"

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	int			bytes;
	std::string	credential;

	switch (client.status)
	{
		case Client::CODE:
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			fstat(client.file_fd, &file_info);
			if (S_ISDIR(file_info.st_mode) && client.conf["listing"] == "on")
				createListing(client);
			else if (client.res.status_code == NOTFOUND)
				negotiate(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				client.file_fd = -1;
				execCGI(client);
				client.status = Client::CGI;
			}
			else 
				_helper.fillStatus(client);
			break ;
		case Client::CGI:
			if (readCGIResult(client))
			{
				parseCGIResult(client);
				_helper.fillStatus(client);
			}
			break ;
		case Client::HEADERS:
			fstat(client.file_fd, &file_info);
			if (client.file_fd != -1 && !S_ISDIR(file_info.st_mode))
			{
				client.res.headers["Last-Modified"] = _helper.getLastModified(client.conf["path"]);
				client.res.headers["Content-Type"] = _helper.findType(client.req);
			}
			if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic";
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			if ((S_ISDIR(file_info.st_mode) && client.conf["listing"] == "on")
				|| client.file_fd == -1)
				client.res.headers["Content-Length"] = std::to_string(client.file_str.size());
			else
				client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
			_helper.fillHeaders(client);
			break ;
		case Client::BODY:
			fstat(client.file_fd, &file_info);
			if (!S_ISDIR(file_info.st_mode) && client.file_fd != -1)
			{
				bytes = read(client.file_fd, client.wBuf, BUFFER_SIZE);
				if (bytes >= 0)
					client.wBuf[bytes] = '\0';
				if (bytes == 0)
					client.setToStandBy();
			}
			else if (S_ISDIR(file_info.st_mode) || client.file_fd == -1)
			{
				if (client.file_str.size() > BUFFER_SIZE)
				{
					strncpy(client.wBuf, client.file_str.c_str(), BUFFER_SIZE);
					client.file_str = client.file_str.substr(BUFFER_SIZE);
				}
				else
				{
					strcpy(client.wBuf, client.file_str.c_str());
					client.setToStandBy();
				}
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
			_helper.fillStatus(client);
			break ;
		case Client::HEADERS:
			fstat(client.file_fd, &file_info);
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
			_helper.fillHeaders(client);
			client.setToStandBy();
			break ;
	}
}

void	Handler::handlePost(Client &client)
{
	struct stat	file_info;
	int			bytes;
	int			size;

	switch (client.status)
	{
		case Client::BODYPARSING:
			parseBody(client);
		case Client::CODE:
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			if (((client.conf.find("CGI") != client.conf.end() && client.req.uri.find(client.conf["CGI"]) != std::string::npos)
			|| (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos))
			&& client.res.status_code == OK)
			{
				client.file_fd = -1;
				execCGI(client);
				client.status = Client::CGI;
			}
			else 
				_helper.fillStatus(client);
			break ;
		case Client::CGI:
			if (readCGIResult(client))
			{
				parseCGIResult(client);
				_helper.fillStatus(client);
			}
			break ;
		case Client::HEADERS:
			fstat(client.file_fd, &file_info);
			if (client.res.status_code == UNAUTHORIZED)
				client.res.headers["WWW-Authenticate"] = "Basic realm=\"webserv\"";
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.headers.find("Content-Length") == client.res.headers.end())
				client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
			if (client.res.headers.find("Content-Type") == client.res.headers.end() && client.file_fd != -1)
				client.res.headers["Content-Type"] = _helper.findType(client.req);
			_helper.fillHeaders(client);
			break ;
		case Client::BODY:
			client.last_date = _helper.getDate();
			if (client.file_fd != -1)
			{
				size = strlen(client.wBuf);
				bytes = read(client.file_fd, client.wBuf + size, BUFFER_SIZE - size);
				client.wBuf[bytes + size] = '\0';
				if (bytes == 0)
					client.setToStandBy();
			}
			else
			{
				size = client.file_str.size();
				bytes = write(client.fd, client.file_str.c_str(), size);
				if (bytes == -1)
					return ;
				if (bytes < size)
					client.file_str = client.file_str.substr(bytes);
				else
					client.setToStandBy();
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
		case Client::CODE:
			if (!_helper.getStatusCode(client))
				_helper.getErrorPage(client);
			_helper.fillStatus(client);
			break ;
		case Client::HEADERS:
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			if (client.res.status_code == CREATED || client.res.status_code == NOCONTENT)
			{
				client.res.headers["Content-Location"] = client.conf["path"];
				if (client.res.status_code == CREATED)
				{
					body = "Ressource created\n";
					client.res.headers["Content-Length"] = std::to_string(body.size());
				}

			}
			_helper.fillHeaders(client);
			break ;
		case Client::BODY:
			if (client.res.status_code == CREATED)
			{
				body = "Ressource created\n";
				strcpy(client.wBuf, body.c_str());
			}
			client.last_date = _helper.getDate();
			write(client.file_fd, client.req.body.c_str(), client.req.body.size());
			client.setToStandBy();
	}
}

void	Handler::handleBadRequest(Client &client)
{
	int				bytes;
	struct stat		file_info;

	switch (client.status)
	{
		case Client::CODE:
			client.res.version = "HTTP/1.1";
			client.res.status_code = BADREQUEST;
			_helper.getErrorPage(client);
			_helper.fillStatus(client);
			break ;
		case Client::HEADERS:
			fstat(client.file_fd, &file_info);
			client.res.headers["Date"] = _helper.getDate();
			client.res.headers["Server"] = "webserv";
			client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
			_helper.fillHeaders(client);
			break ;
		case Client::BODY:
			bytes = read(client.file_fd, client.wBuf, BUFFER_SIZE);
			client.wBuf[bytes] = '\0';
			client.status = Client::DONE;
			break ;
	}
}