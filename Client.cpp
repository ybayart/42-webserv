#include "Client.hpp"

Client::Client(int filed, fd_set *r, fd_set *w)
: fd(filed), rSet(r), wSet(w), hasBody(false), fileFd(-1), status(PARSING)
{
	rBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	wBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	memset(rBuf, 0, BUFFER_SIZE + 1);
	memset(wBuf, 0, BUFFER_SIZE + 1);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	FD_SET(fd, rSet);
	chunk.len = 0;
	chunk.done = false;
	chunk.found = false;
}

Client::~Client()
{
	free(rBuf);
	free(wBuf);
	close(fd);
	close(fileFd);
	if (FD_ISSET(fd, rSet))
		FD_CLR(fd, rSet);
	if (FD_ISSET(fd, wSet))
		FD_CLR(fd, wSet);
}

int		Client::getFd() const
{
	return (fd);
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
	// std::cout << "standing by\n";
	// std::cout << ip << ":" << port << ": " << req.method << ": " << "DONE" << std::endl;
	status = STANDBY;
	setReadState(true);
	close(fileFd);
	fileFd = -1;
	memset(rBuf, 0, BUFFER_SIZE + 1);
	hasBody = false;
	conf.clear();
	req.body.clear();
	res.status_code.clear();
	res.headers.clear();
	req.headers.clear();
}