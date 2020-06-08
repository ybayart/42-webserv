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
    listen(_fd, 5);
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
		{
			if (client->hasBody == false)
				readRequest(client);
			else
				readBody(client);
		}
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
	_clients[fd] = newOne;
	std::cout << "new connection from " << newOne->ip << ":"
	<< htons(info.sin_port) << std::endl;
}

void	Listener::readRequest(Client *client)
{
	int 		bytes;
	int			ret;

	bytes = strlen(client->rBuf);
	errno = 0;
	ret = read(client->fd, client->rBuf + bytes, 2);
	bytes += ret;
	if (ret <= 0)
	{
		if (ret == -1)
			std::cout << strerror(errno) << std::endl;
		else
		{
			std::cout << "connection closed\n";
			delete client;
			_clients.erase(client->fd);
		}
	}
	else
	{
		client->rBuf[bytes] = '\0';
		if (strstr(client->rBuf, "\r\n\r\n") != NULL)
		{
			std::cout << "[" << client->rBuf << "]" << std::endl;
			client->lastDate = _handler._helper.getDate();
			_handler.parseRequest(*client, _conf);
			if (client->status == CODE)
			{
				client->setReadState(false);
				client->setWriteState(true);
			}
		}
	}
}

void	Listener::readBody(Client *client)
{
	_handler.parseBody(*client);
	if (client->status == CODE)
	{
		client->hasBody = false;
		client->setReadState(false);
		client->setWriteState(true);
	}
}

void	Listener::writeResponse(Client *client)
{
	int		bytes;

	if (strlen(client->wBuf) > 0)
	{
		bytes = write(client->fd, client->wBuf, strlen(client->wBuf));
		std::cout << "sent : [" << client->wBuf << "]\n";
		memset(client->wBuf, 0, BUFFER_SIZE);
	}
	if (client->status != STANDBY)
		_handler.dispatcher(*client);
	else
		if (getTimeDiff(client->lastDate) >= TIMEOUT)
			client->status = DONE;
	if (client->status == DONE)
	{
		std::cout << "done\n";
		delete client;
		_clients.erase(client->fd);
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
