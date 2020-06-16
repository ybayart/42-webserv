#include "Client.hpp"

Client::Client(int filed, fd_set *r, fd_set *w, struct sockaddr_in info)
: fd(filed), file_fd(-1), status(PARSING), rSet(r), wSet(w)
{
	ip = inet_ntoa(info.sin_addr);
	port = htons(info.sin_port);
	rBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	wBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	memset(rBuf, 0, BUFFER_SIZE + 1);
	memset(wBuf, 0, BUFFER_SIZE + 1);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	FD_SET(fd, rSet);
	chunk.len = 0;
	chunk.done = false;
	chunk.found = false;
	g_logger.log("new connection from " + ip + std::to_string(port), LOW);
}

Client::~Client()
{
	free(rBuf);
	free(wBuf);
	close(fd);
	close(file_fd);
	unlink(tmp_path.c_str());
	if (FD_ISSET(fd, rSet))
		FD_CLR(fd, rSet);
	if (FD_ISSET(fd, wSet))
		FD_CLR(fd, wSet);
	g_logger.log("connection closed from " + ip + ":" + std::to_string(port), LOW);
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

void	Client::setFileToRead(int fd, bool state)
{
	if (state)
		FD_SET(fd, rSet);
	else
		FD_CLR(fd, rSet);
}

void	Client::setFileToWrite(int fd, bool state)
{
	if (state)
		FD_SET(fd, wSet);
	else
		FD_CLR(fd, wSet);
}


void	Client::setToStandBy()
{
	g_logger.log(req.method + " from " + ip + ":" + std::to_string(port) + " answered", LOW);
	status = STANDBY;
	setReadState(true);
	close(file_fd);
	file_fd = -1;
	memset(rBuf, 0, BUFFER_SIZE + 1);
	file_str.clear();
	conf.clear();
	req.body.clear();
	res.status_code.clear();
	res.headers.clear();
	req.headers.clear();
}