#include "Listener.hpp"

Listener::Listener()
{
	int yes = 1;

	FD_ZERO(&_rSet);
	FD_ZERO(&_wSet);
	FD_ZERO(&_readSet);
	FD_ZERO(&_writeSet);
	_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
}

Listener::~Listener()
{
	close(_fd);
}

int		Listener::config(char *file)
{
	return (_conf.parse(file));
}

int		Listener::getMaxFd() const
{
	return (_maxFd);
}

void	Listener::init()
{
	Client	*server;
	int		port;

	port = atoi(_conf._elmts["server|"]["listen"].c_str());
	_info.sin_family = AF_INET;
	_info.sin_addr.s_addr = INADDR_ANY;
	_info.sin_port = htons(port);
	bind(_fd, (struct sockaddr *)&_info, sizeof(_info));
    listen(_fd, 1000);
	server = new Client(_fd, &_rSet, &_wSet);
	_clients[_fd] = server;
    _maxFd = _fd;
}

void	Listener::select()
{
    _readSet = _rSet;
	_writeSet = _wSet;
	::select(_maxFd + 1, &_readSet, &_writeSet, NULL, NULL);
}

void	Listener::handleRequest(int fd)
{
	Client	*client;

	if (_clients.find(fd) != _clients.end())
		client = _clients[fd];
	else
		return ;
    if (FD_ISSET(client->fd, &_readSet))
	{
		if (client->fd == this->_fd)
			acceptConnection();
		else
			readRequest(client);
	}
	else if (FD_ISSET(client->fd, &_writeSet) && client->fd != this->_fd)
		writeResponse(client);
}

void	Listener::acceptConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;
	Client				*newOne;

	memset(&info, 0, sizeof(info));
	fd = accept(_fd, (struct sockaddr *)&info, &len);
	if (fd > _maxFd)
		_maxFd = fd;
	newOne = new Client(fd, &_rSet, &_wSet);
	newOne->ip = inet_ntoa(info.sin_addr);
	newOne->port = htons(info.sin_port);
	_clients[fd] = newOne;
	std::cout << "new connection from " << newOne->ip << ":"
	<< newOne->port << std::endl;
	std::cout << "nb of clients: " << _clients.size() - 1 << std::endl;
}

void	Listener::readRequest(Client *client)
{
	int 		bytes;
	int			ret;

	bytes = strlen(client->rBuf);
	ret = read(client->fd, client->rBuf + bytes, BUFFER_SIZE - bytes);
	bytes += ret;
	if (ret <= 0)
	{
		if (ret == -1)
			std::cout << "reading error" << std::endl;
		else
		{
			std::cout << "connection closed from " << client->ip << ":" << client->port << "\n";
			_clients.erase(client->fd);
			delete client;
			std::cout << "nb of clients: " << _clients.size() - 1 << std::endl;
		}
	}
	else
	{
		client->rBuf[bytes] = '\0';
		if (strstr(client->rBuf, "\r\n\r\n") != NULL
			&& client->hasBody == false)
		{
			// std::cout << "[" << client->rBuf << "]" << std::endl;
			client->lastDate = _handler._helper.getDate();
			_handler.parseRequest(*client, _conf);
			client->setWriteState(true);
		}
		if (client->hasBody == true)
		{
			client->status = PARSING;
			_handler.parseBody(*client);
		}
	}
}

void	Listener::writeResponse(Client *client)
{
	int				bytes;
	int				size;
	std::string		tmp;

	size = strlen(client->wBuf);
	if (size > 0)
	{
		bytes = write(client->fd, client->wBuf, size);
		// std::cout << "sent : [" << client->wBuf << "]\n";
		if (bytes < size)
		{
			tmp = client->wBuf;
			tmp = tmp.substr(bytes);
			strcpy(client->wBuf, tmp.c_str());
		}
		else
			memset(client->wBuf, 0, BUFFER_SIZE + 1);
	}
	if (client->status != STANDBY && client->status != DONE)
		_handler.dispatcher(*client);
	else if (client->status == STANDBY)
	{
		if (getTimeDiff(client->lastDate) >= TIMEOUT)
			client->status = DONE;
	}
	if (client->status == DONE)
	{
		std::cout << "done with " << client->ip << ":" << client->port << "\n";
		_clients.erase(client->fd);
		delete client;
		std::cout << "nb of clients: " << _clients.size() - 1 << std::endl;
	}

}

int		Listener::getTimeDiff(std::string start)
{
	struct tm		start_tm;
	struct tm		*now_tm;
	struct timeval	time;
	int				result;

	strptime(start.c_str(), "%a, %d %b %Y %T", &start_tm);
	gettimeofday(&time, NULL);
	now_tm = localtime(&time.tv_sec);
	result = (now_tm->tm_hour - start_tm.tm_hour) * 3600;
	result += (now_tm->tm_min - start_tm.tm_min) * 60;
	result += (now_tm->tm_sec - start_tm.tm_sec);
	// std::cout << "diff: " << result << std::endl;
	return (result);
}
