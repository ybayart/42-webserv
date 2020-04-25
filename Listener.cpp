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

void	Listener::config(char *file)
{
	_conf.parse(file);
}

void	Listener::init()
{
	_info.sin_family = AF_INET;
	_info.sin_addr.s_addr = INADDR_ANY;
	_info.sin_port = htons(_conf._port);
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

void	Listener::getRequest(int fd)
{
    if (FD_ISSET(fd, &_readSet))
	{
		if (fd == this->_fd)
			acceptConnection();
		else
			readRequest(fd);
	}
}

void	Listener::sendResponse(int fd)
{
	std::string			response;

	if (FD_ISSET(fd, &_writeSet) && fd != this->_fd)
	{
		response = _handler.generateResponse(fd);
		send(fd, response.c_str(), response.size(), 0);
		close(fd);
		FD_CLR(fd, &_wSet);
	}
}

void	Listener::acceptConnection()
{
	int 				client;
	struct sockaddr_in	info;
	socklen_t			len;

	client = accept(_fd, (struct sockaddr *)&info, &len);
	fcntl(client, F_SETFL, O_NONBLOCK);
	FD_SET(client, &_rSet);
	if (client > _maxFd)
		_maxFd = client;
	std::cout << "new connection!\n";
}

void	Listener::readRequest(int fd)
{
	int 		bytes;
	char		buf[4096];
	std::string	result;

	while ((bytes = recv(fd, buf, 4095, 0)) > 0)
	{
		if (bytes <= 0)
		{
			if (bytes == -1)
				std::cout << "reading error\n";
			else
			{
				std::cout << "connection closed\n";
				close(fd);
				FD_CLR(fd, &_rSet);
			}
		}
		else
		{
			buf[bytes] = '\0';
			result += buf;
		}
	}
	std::cout << result;
	_handler.parseRequest(fd, result);
	FD_CLR(fd, &_rSet);
	FD_SET(fd, &_wSet);
}
