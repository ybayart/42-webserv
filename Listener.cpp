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
	// fcntl(_fd, F_SETFL, O_NONBLOCK);
 //    FD_SET(_fd, &_rSet);
    Client	client(_fd, &_rSet, &_wSet);
    _clients.push_back(client);
    _maxFd = _fd;
}

void	Listener::select()
{
    _readSet = _rSet;
	_writeSet = _wSet;
	::select(_maxFd + 1, &_readSet, &_writeSet, NULL, NULL);
}

void	Listener::handleRequest(std::vector<Client>::iterator it)
{
    if (FD_ISSET(it->fd, &_readSet))
	{
		if (it->fd == this->_fd)
			acceptConnection();
		else
		{
			if (it->hasBody == false)
				readRequest(it);
			else
				readBody(it);
		}
	}
	else if (FD_ISSET(it->fd, &_writeSet) && it->fd != this->_fd)
		writeResponse(it);
	else if (it->status == STANDBY)
	{
		if (getTimeDiff(it->lastDate) >= TIMEOUT)
			it->status = DONE;
		else
			it->setReadState(true);
	}
	else if (it->status == DONE)
	{
		std::cout << "done\n";
		_clients.erase(it);
	}
}

void	Listener::acceptConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;

	memset(&info, 0, sizeof(info));
	fd = accept(_fd, (struct sockaddr *)&info, &len);
	if (fd > _maxFd)
		_maxFd = fd;
	Client	newOne(fd, &_rSet, &_wSet);
	newOne.ip = inet_ntoa(info.sin_addr);
	_clients.push_back(newOne);
	std::cout << "new connection from " << newOne.ip << ":"
	<< htons(info.sin_port) << std::endl;
}

void	Listener::readRequest(std::vector<Client>::iterator it)
{
	int 		bytes;
	int			ret;

	bytes = strlen(it->rBuf);
	ret = read(it->fd, it->rBuf + bytes, BUFFER_SIZE - 1);
	bytes += ret;
	if (ret <= 0)
	{
		if (ret == -1)
			std::cout << "reading error\n";
		else
		{
			std::cout << "connection closed\n";
			_clients.erase(it);
		}
	}
	else
	{
		it->rBuf[bytes] = '\0';
		if (strstr(it->rBuf, "\r\n\r\n") != NULL)
		{
			std::cout << "[" << it->rBuf << "]" << std::endl;
			it->lastDate = _handler._helper.getDate();
			_handler.parseRequest(*it, _conf);
			if (it->status == CODE)
			{
				it->setReadState(false);
				it->setWriteState(true);
			}
		}
	}
}

void	Listener::readBody(std::vector<Client>::iterator it)
{
	_handler.parseBody(*it);
	if (it->status == CODE)
	{
		it->hasBody = false;
		it->setReadState(false);
		it->setWriteState(true);
	}
}

void	Listener::writeResponse(std::vector<Client>::iterator it)
{
	int		bytes;

	_handler.dispatcher(*it);
	if (strlen(it->wBuf) > 0)
	{
		bytes = write(it->fd, it->wBuf, strlen(it->wBuf));
		std::cout << "sent : [" << it->wBuf << "]\n";
		memset(it->wBuf, 0, BUFFER_SIZE);
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
