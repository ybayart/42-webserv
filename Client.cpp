#include "Client.hpp"

Client::Client(int filed, fd_set *r, fd_set *w)
: fd(filed), rSet(r), wSet(w), hasBody(false), fileFd(-1), status(CODE)
{
	rBuf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	wBuf = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	memset(rBuf, 0, BUFFER_SIZE);
	memset(wBuf, 0, BUFFER_SIZE);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	FD_SET(fd, rSet);
}

Client::~Client()
{
	free(rBuf);
	free(wBuf);
	close(fd);
	if (FD_ISSET(fd, rSet))
		FD_CLR(fd, rSet);
	if (FD_ISSET(fd, wSet))
		FD_CLR(fd, wSet);
}

bool	Client::getReadState()
{
	if (FD_ISSET(fd, rSet))
		return (true);
	else
		return (false);
}

bool	Client::getWriteState()
{
	if (FD_ISSET(fd, wSet))
		return (true);
	else
		return (false);
}

void	Client::setReadState(bool state)
{
	if (state)
		FD_SET(fd, rSet);
	else
		FD_CLR(fd, rSet);
}

void	Client::setWriteState(bool state)
{
	if (state)
		FD_SET(fd, wSet);
	else
		FD_CLR(fd, wSet);
}

void	Client::setToStandBy()
{
	std::map<std::string, std::string>::iterator b;

	std::cout << "standing by\n";
	status = STANDBY;
	setReadState(false);
	fileFd = -1;
	memset(rBuf, 0, BUFFER_SIZE);
	hasBody = false;
	conf.clear();
	b = res.headers.begin();
	while (b != res.headers.end())
	{
		if (b->first != "Date")
			res.headers.erase(b);
		++b;
	}
	req.headers.clear();
}