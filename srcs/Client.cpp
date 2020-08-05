#include "utils.h"
#include "Client.hpp"

Client::Client(int filed, fd_set *r, fd_set *w, struct sockaddr_in info)
: fd(filed), read_fd(-1), write_fd(-1), status(STANDBY), cgi_pid(-1), tmp_fd(-1), rSet(r), wSet(w)
{
	ip = inet_ntoa(info.sin_addr);
	port = htons(info.sin_port);
	rBuf = (char *)malloc(sizeof(char) * (BUFFER_SIZE + 1));
	memset((void *)rBuf, 0, BUFFER_SIZE + 1);
	fcntl(fd, F_SETFL, O_NONBLOCK);
	FD_SET(fd, rSet);
	FD_SET(fd, wSet);
	chunk.len = 0;
	chunk.done = false;
	chunk.found = false;
	last_date = ft::getDate();
	g_logger.log("new connection from " + ip + ":" + std::to_string(port), LOW);
}

Client::~Client()
{
	free(rBuf);
	rBuf = NULL;
	if (fd != -1)
	{
		close(fd);
		FD_CLR(fd, rSet);
		FD_CLR(fd, wSet);
	}
	if (read_fd != -1)
	{
		close(read_fd);
		FD_CLR(read_fd, rSet);
	}
	if (write_fd != -1)
	{
		close(write_fd);
		FD_CLR(write_fd, wSet);	
	}
	if (tmp_fd != -1)
	{
		close(tmp_fd);
		unlink(TMP_PATH);
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

void	Client::setFileToRead(bool state)
{
	if (read_fd != -1)
	{
		if (state)
			FD_SET(read_fd, rSet);
		else
			FD_CLR(read_fd, rSet);
	}
}

void	Client::setFileToWrite(bool state)
{
	if (write_fd != -1)
	{
		if (state)
			FD_SET(write_fd, wSet);
		else
			FD_CLR(write_fd, wSet);
	}
}

void	Client::readFile()
{
	char			buffer[BUFFER_SIZE + 1];
	int				ret = 0;
	int				status = 0;

	if (cgi_pid != -1)
	{
		if (waitpid((pid_t)cgi_pid, (int *)&status, (int)WNOHANG) == 0)
			return ;
		else
		{
			if (WEXITSTATUS(status) == 1)
			{
				close(tmp_fd);
				tmp_fd = -1;
				cgi_pid = -1;
				close(read_fd);
				unlink(TMP_PATH);
				setFileToRead(false);
				read_fd = -1;
				res.body = "Error with cgi\n";
				return ;
			}
		}
	}
	ret = read(read_fd, buffer, BUFFER_SIZE);
	if (ret >= 0)
		buffer[ret] = '\0';
	std::string	tmp(buffer, ret);
	res.body += tmp;
	if (ret == 0)
	{
		close(read_fd);
		unlink(TMP_PATH);
		setFileToRead(false);
		read_fd = -1;
	}
}

void	Client::writeFile()
{
	int ret = 0;

	ret = write(write_fd, req.body.c_str(), req.body.size());
	if (cgi_pid != -1)
		g_logger.log("sent " + std::to_string(ret) + " bytes to CGI stdin", MED);
	else
		g_logger.log("write " + std::to_string(ret) + " bytes in file", MED);
	if ((unsigned long)ret < req.body.size())
		req.body = req.body.substr(ret);
	else
	{
		req.body.clear();
		close(write_fd);
		setFileToWrite(false);
		write_fd = -1;
	}
}

void	Client::setToStandBy()
{
	g_logger.log(req.method + " from " + ip + ":" + std::to_string(port) + " answered", MED);
	status = STANDBY;
	setReadState(true);
	if (read_fd != -1)
		close(read_fd);
	read_fd = -1;
	if (write_fd != -1)
		close(write_fd);
	write_fd = -1;
	memset((void *)rBuf, (int)0, (size_t)(BUFFER_SIZE + 1));
	conf.clear();
	req.clear();
	res.clear();
}
