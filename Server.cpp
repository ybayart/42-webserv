#include "Server.hpp"

Server::Server() : _port(-1)
{

}

Server::~Server()
{
	if (_port != -1)
	{
		for (std::vector<Client*>::iterator it(_clients.begin()); it != _clients.end(); ++it)
			delete *it;
		_clients.clear();
		close(_fd);
		std::cout << "closed server listening on port " << _port << "\n";
	}
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
	int				yes = 1;
	std::string		to_parse;
	std::string		host;

	_readSet = readSet;
	_writeSet = writeSet;
	_wSet = wSet;
	_rSet = rSet;

	to_parse = _conf[0]["server|"]["listen"];
	_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (to_parse.find(":") != std::string::npos)
    {
    	host = to_parse.substr(0, to_parse.find(":"));
    	_port = atoi(to_parse.substr(to_parse.find(":") + 1).c_str());
		_info.sin_addr.s_addr = inet_addr(host.c_str());
		_info.sin_port = htons(_port);

    }
    else
    {
		_info.sin_addr.s_addr = INADDR_ANY;
		_port = atoi(to_parse.c_str());
		_info.sin_port = htons(_port);

    }
	_info.sin_family = AF_INET;
	bind(_fd, (struct sockaddr *)&_info, sizeof(_info));
    listen(_fd, 1000);
	fcntl(_fd, F_SETFL, O_NONBLOCK);
	FD_SET(_fd, _rSet);
    _maxFd = _fd;
    std::cout << "Listening on port " << _port << "...\n";
}

void	Server::refuseConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;


	fd = accept(_fd, (struct sockaddr *)&info, &len);
	close(fd);
	std::cout << "fd limit reached, connection refused\n";
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
	newOne = new Client(fd, _rSet, _wSet, info);
	_clients.push_back(newOne);
	std::cout << "nb of clients: " << _clients.size() << " [" << _port << "]\n";
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
	if (ret > 0)
	{
		client->rBuf[bytes] = '\0';
		if (strstr(client->rBuf, "\r\n\r\n") != NULL
			&& client->status != BODYPARSING)
		{
			std::cout << "[" << client->rBuf << "]" << std::endl;
			client->lastDate = _handler._helper.getDate();
			_handler.parseRequest(*client, _conf);
			client->setWriteState(true);
		}
		if (client->status == BODYPARSING)
			_handler.parseBody(*client);
		return (1);
	}
	else
	{
		delete client;
		_clients.erase(it);
		std::cout << "nb of clients: " << _clients.size() << " [" << _port << "]\n";
		return (0);
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
		delete client;
		_clients.erase(it);
		std::cout << "nb of clients: " << _clients.size() << " [" << _port << "]\n";
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
