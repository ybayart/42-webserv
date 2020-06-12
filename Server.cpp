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

int		Server::getOpenFd()
{
	int 	nb = 0;
	Client	*client;

	for (std::vector<Client*>::iterator it(_clients.begin()); it != _clients.end(); ++it)
	{
		client = *it;
		nb += 1;
		if (client->fileFd != -1)
			nb += 1;
	}
	return (nb);
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

void	Server::refuseConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;


	fd = accept(_fd, (struct sockaddr *)&info, &len);
	close(fd);
	std::cout << "fd limit reached, refusing connection\n";
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
	_clients.push_back(newOne);
	std::cout << "new connection from " << newOne->ip << ":"
	<< newOne->port << std::endl;
	std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
}

int		Server::readRequest(std::vector<Client*>::iterator it)
{
	int 		bytes;
	int			ret;
	Client		*client;

	client = *it;
	bytes = strlen(client->rBuf);
	ret = read(client->fd, client->rBuf + bytes, BUFFER_SIZE - bytes);
	bytes += ret;
	if (ret <= 0)
	{
		std::cout << "connection closed from " << client->ip << ":" << client->port << "\n";
		delete client;
		_clients.erase(it);
		std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
		return (0);
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
		return (1);
	}
}

int		Server::writeResponse(std::vector<Client*>::iterator it)
{
	int				bytes;
	int				size;
	std::string		tmp;
	Client			*client;

	client = *it;
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
		delete client;
		_clients.erase(it);
		std::cout << "nb of clients: " << _clients.size() << " [" << _conf["server|"]["listen"] << "]\n";
		return (0);
	}
	return (1);
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
