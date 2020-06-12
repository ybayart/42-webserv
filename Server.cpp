#include "Server.hpp"

Server::Server()
{

}

Server::~Server()
{
	close(_fd);
}

int		Server::getMaxFd() const
{
	return (_maxFd);
}

int		Server::getFd() const
{
	return (_fd);
}

void	Server::init(fd_set *readSet, fd_set *writeSet, fd_set *rSet, fd_set *wSet)
{
	int		port;
	int		yes = 1;

	_readSet = readSet;
	_writeSet = writeSet;
	_wSet = wSet;
	_rSet = rSet;

	_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	port = atoi(_conf["server|"]["listen"].c_str());
	_info.sin_family = AF_INET;
	_info.sin_addr.s_addr = INADDR_ANY;
	_info.sin_port = htons(port);
	bind(_fd, (struct sockaddr *)&_info, sizeof(_info));
    listen(_fd, 1000);
	fcntl(_fd, F_SETFL, O_NONBLOCK);
	FD_SET(_fd, _rSet);
    _maxFd = _fd;
    std::cout << "Listening on port " << port << std::endl;
}

bool	Server::checkIfClient(int fd)
{
	if (_clients.find(fd) != _clients.end())
		return (true);
	else
		return (false);
}

void	Server::acceptConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;
	Client				*newOne;

	memset(&info, 0, sizeof(info));
	fd = accept(_fd, (struct sockaddr *)&info, &len);
	if (fd > _maxFd)
		_maxFd = fd;
	newOne = new Client(fd, _rSet, _wSet);
	newOne->ip = inet_ntoa(info.sin_addr);
	newOne->port = htons(info.sin_port);
	_clients[fd] = newOne;
	std::cout << "new connection from " << newOne->ip << ":"
	<< newOne->port << std::endl;
	std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
}

void	Server::readRequest(int fd)
{
	int 		bytes;
	int			ret;
	Client		*client;

	client = _clients[fd];
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
			std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
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

void	Server::writeResponse(int fd)
{
	int				bytes;
	int				size;
	std::string		tmp;
	Client			*client;

	client = _clients[fd];
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
		std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
	}

}

int		Server::getTimeDiff(std::string start)
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
