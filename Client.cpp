#include "Client.hpp"

Client::Client(int filed, fd_set *r, fd_set *w, struct sockaddr_in info)
: fd(filed), file_fd(-1), status(PARSING), cgi_pid(-1), rSet(r), wSet(w)
{
	ip = inet_ntoa(info.sin_addr);
	port = htons(info.sin_port);
	rBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	memset(rBuf, 0, BUFFER_SIZE + 1);
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
	close(fd);
	close(file_fd);
	unlink(tmp_path.c_str());
	FD_CLR(fd, rSet);
	FD_CLR(fd, wSet);
	if (file_fd != -1)
	{
		FD_CLR(file_fd, rSet);
		FD_CLR(file_fd, wSet);	
	}
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

void	Client::readFile()
{
	char	buffer[BUFFER_SIZE + 1];
	int		ret;

	if (cgi_pid != -1)
	{
		waitpid(cgi_pid, NULL, 0);
		cgi_pid = -1;
	}
	ret = read(file_fd, buffer, BUFFER_SIZE);
	if (ret >= 0)
		buffer[ret] = '\0';
	res.body += buffer;
	if (ret == 0)
	{
		close(file_fd);
		unlink(tmp_path.c_str());
		setFileToRead(file_fd, false);
		file_fd = -1;
	}
}

void	Client::writeFile()
{
	unsigned long ret;

	ret = write(file_fd, req.body.c_str(), req.body.size());
	if (ret < req.body.size())
		req.body = req.body.substr(ret);
	else
	{
		req.body.clear();
		close(file_fd);
		setFileToWrite(file_fd, false);
		file_fd = -1;
	}
}

void	Client::setToStandBy()
{
	g_logger.log(req.method + " from " + ip + ":" + std::to_string(port) + " answered", LOW);
	status = STANDBY;
	setReadState(true);
	close(file_fd);
	file_fd = -1;
	memset(rBuf, 0, BUFFER_SIZE + 1);
	conf.clear();
	req.body.clear();
	res.body.clear();
	res.status_code.clear();
	res.headers.clear();
	req.headers.clear();
}