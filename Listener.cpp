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
	Client *client;

	if (FD_ISSET(fd, &_writeSet) && fd != this->_fd)
	{
		client = _clients[fd];
		_handler.dispatcher(*client);
		if (client->getWriteState() == false)
		{
			delete client;
			_clients.erase(fd);
		}
	}
}

void	Listener::acceptConnection()
{
	int 				fd;
	struct sockaddr_in	info;
	socklen_t			len;

	memset(&info, 0, sizeof(info));
	fd = accept(_fd, (struct sockaddr *)&info, &len);
	_clients[fd] = new Client(fd, &_rSet, &_wSet);
	if (fd > _maxFd)
		_maxFd = fd;
	std::cout << "new connection from "
	<< inet_ntoa(info.sin_addr) << ":" << htons(info.sin_port) << std::endl;
}

void	Listener::readRequest(int fd)
{
	int 		bytes;
	Client		*client;

	client = _clients[fd];
	bytes = read(fd, client->_rBuf + client->_rBytes, 3);
	client->_rBytes += bytes;
	if (bytes <= 0)
	{
		if (bytes == -1)
			std::cout << "reading error\n";
		else
		{
			std::cout << "connection closed\n";
			delete client;
			_clients.erase(fd);
		}
	}
	else
		client->_rBuf[client->_rBytes] = '\0';
	if (strstr(client->_rBuf, "\r\n\r\n") != NULL)
	{
		std::cout << client->_rBuf << std::endl;
		_handler.parseRequest(*client, _conf);
	}
}
