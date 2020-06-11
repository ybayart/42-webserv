#include "Handler.hpp"

void	Handler::handleGet(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;
	std::string	credential;

	if (client.status == CODE)
	{
		if (!_helper.getStatusCode(client))
			_helper.getErrorPage(client);
		if (client.res.status_code == NOTFOUND)
			negotiate(client);
		_helper.fillStatus(client);
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
		_helper.fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE);
		if (bytes >= 0)
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
		if (!_helper.getStatusCode(client))
			_helper.getErrorPage(client);
		if (client.res.status_code == NOTFOUND)
			negotiate(client);
		_helper.fillStatus(client);
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
		_helper.fillHeaders(client);
		client.setToStandBy();
	}
}

void	Handler::handlePost(Client &client)
{
	struct stat	file_info;
	int			fd;
	int			bytes;
	int			size;

	if (client.status == PARSING)
		parseBody(client);
	if (client.status == CODE)
	{
		if (!_helper.getStatusCode(client))
			_helper.getErrorPage(client);
		if (client.conf.find("CGI") != client.conf.end()
		&& client.req.uri.find(client.conf["CGI"]) != std::string::npos
		&& client.res.status_code == OK)
		{
			client.fileFd = -1;
			execCGI(client);
			client.status = CGI;
		}
		else 
			_helper.fillStatus(client);
	}
	else if (client.status == CGI)
	{
		if (readCGIResult(client))
		{
			parseCGIResult(client);
			_helper.fillStatus(client);
		}
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		if (client.res.status_code == UNAUTHORIZED)
			client.res.headers["WWW-Authenticate"] = "Basic realm=\"webserv\"";
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		if (client.res.headers.find("Content-Length") == client.res.headers.end())
			client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		if (client.res.headers.find("Content-Type") == client.res.headers.end())
			client.res.headers["Content-Type"] = _helper.findType(client.req);
		_helper.fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		client.lastDate = _helper.getDate();
		if (client.fileFd != -1)
		{
			size = strlen(client.wBuf);
			bytes = read(client.fileFd, client.wBuf + size, BUFFER_SIZE - size);
			client.wBuf[bytes + size] = '\0';
			if (bytes == 0)
			{
				close(client.fileFd);
				client.setToStandBy();
			}
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
			{
				client.file_str.clear();
				client.setToStandBy();
			}
		}
	}
}

void	Handler::handlePut(Client &client)
{
	int 			ret;
	std::string		path;
	struct stat		file_info;
	std::string		body;

	if (client.status == PARSING)
		parseBody(client);
	if (client.status == CODE)
	{
		if (!_helper.getStatusCode(client))
			_helper.getErrorPage(client);
		_helper.fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
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
	}
	else if (client.status == BODY)
	{
		if (client.res.status_code == CREATED)
		{
			body = "Ressource created\n";
			strcpy(client.wBuf, body.c_str());
		}
		client.lastDate = _helper.getDate();
		write(client.fileFd, client.req.body.c_str(), client.req.body.size());
		client.setToStandBy();
	}
}

void	Handler::handleBadRequest(Client &client)
{
	int				bytes;
	struct stat		file_info;

	if (client.status == CODE)
	{
		client.res.version = "HTTP/1.1";
		client.res.status_code = BADREQUEST;
		_helper.getErrorPage(client);
		_helper.fillStatus(client);
	}
	else if (client.status == HEADERS)
	{
		fstat(client.fileFd, &file_info);
		client.res.headers["Date"] = _helper.getDate();
		client.res.headers["Server"] = "webserv";
		client.res.headers["Content-Length"] = std::to_string(file_info.st_size);
		_helper.fillHeaders(client);
	}
	else if (client.status == BODY)
	{
		bytes = read(client.fileFd, client.wBuf, BUFFER_SIZE);
		client.wBuf[bytes] = '\0';
		client.status = DONE;
	}
}