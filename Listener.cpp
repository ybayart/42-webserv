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

void	Listener::init()
{
	int port = atoi(_conf._elmts["server|"]["listen"].c_str());

	_info.sin_family = AF_INET;
	_info.sin_addr.s_addr = INADDR_ANY;
	_info.sin_port = htons(port);
	bind(_fd, (struct sockaddr *)&_info, sizeof(_info));
    listen(_fd, 5);
	fcntl(_fd, F_SETFL, O_NONBLOCK);
    FD_SET(_fd, &_rSet);
    _maxFd = _fd;
}

int		Listener::getMaxFd() const
{
	return (_maxFd);
}

void	Listener::select()
{
    _readSet = _rSet;
	_writeSet = _wSet;
	::select(_maxFd + 1, &_readSet, &_writeSet, NULL, NULL);
}

void	Listener::handleRequest(int fd)
{
    if (FD_ISSET(fd, &_readSet))
	{
		if (fd == this->_fd)
			acceptConnection();
		else
			readRequest(fd);
	}
	if (FD_ISSET(fd, &_writeSet) && fd != this->_fd)
		writeResponse(fd);
}

void	Listener::acceptConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;

	memset(&info, 0, sizeof(info));
	fd = accept(_fd, (struct sockaddr *)&info, &len);
	_clients[fd] = new Client(fd, &_rSet, &_wSet);
	_clients[fd]->ip = inet_ntoa(info.sin_addr);
	if (fd > _maxFd)
		_maxFd = fd;
	std::cout << "new connection from "
	<< _clients[fd]->ip << ":" << htons(info.sin_port) << std::endl;
}

void	Listener::readRequest(int fd)
{
	int 		bytes;
	int			ret;
	Client		*client;

	client = _clients[fd];
	if (client->hasBody == false)
	{
		bytes = strlen(client->rBuf);
		ret = read(fd, client->rBuf + bytes, BUFFER_SIZE - 1);
		bytes += ret;
		if (ret <= 0)
		{
			if (ret == -1)
				std::cout << "reading error\n";
			else
			{
				std::cout << "connection closed\n";
				delete client;
				_clients.erase(fd);
			}
		}
		else
		{
			client->rBuf[bytes] = '\0';
			if (strstr(client->rBuf, "\r\n\r\n") != NULL)
			{
				std::cout << "[" << client->rBuf << "]" << std::endl;
				client->lastDate = _handler._helper.getDate();
				client->status = CODE;
				client->setWriteState(false);
				_handler.parseRequest(*client, _conf);
			}
		}
	}
	else
		_handler.parseBody(*client);
}

void	Listener::writeResponse(int fd)
{
	Client	*client;
	int		bytes;

	if (_clients.find(fd) != _clients.end())
		client = _clients[fd];
	else
		return ;
	if (client->status != STANDBY)
		_handler.dispatcher(*client);
	if (strlen(client->wBuf) > 0)
	{
		bytes = write(client->fd, client->wBuf, strlen(client->wBuf));
		std::cout << "sent : [" << client->wBuf << "]\n";
		memset(client->wBuf, 0, BUFFER_SIZE);
	}
	if (client->status == STANDBY)
	{
		if (getTimeDiff(client->lastDate) >= TIMEOUT)
			client->status = DONE;
		else
			client->setReadState(true);
	}
	if (client->status == DONE)
	{
		std::cout << "done\n";
		delete client;
		_clients.erase(fd);
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
