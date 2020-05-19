#include "Client.hpp"

Client::Client(int fd, fd_set *r, fd_set *w)
: _fd(fd), _rBytes(0), _wBytes(0), _rSet(r), _wSet(w)
{
	_rBuf = (char *)malloc(sizeof(char) * 4096);
	_wBuf = (char *)malloc(sizeof(char) * 4096);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	FD_SET(fd, _rSet);
}

Client::~Client()
{
	free(_rBuf);
	free(_wBuf);
	close(_fd);
	if (FD_ISSET(_fd, _rSet))
		FD_CLR(_fd, _rSet);
	if (FD_ISSET(_fd, _wSet))
		FD_CLR(_fd, _wSet);
}

bool	Client::getReadState()
{
	if (FD_ISSET(_fd, _rSet))
		return (true);
	else
		return (false);
}

bool	Client::getWriteState()
{
	if (FD_ISSET(_fd, _wSet))
		return (true);
	else
		return (false);
}

void	Client::setReadState(bool state)
{
	if (state)
		FD_SET(_fd, _rSet);
	else
		FD_CLR(_fd, _rSet);
}

void	Client::setWriteState(bool state)
{
	if (state)
		FD_SET(_fd, _wSet);
	else
		FD_CLR(_fd, _wSet);
}