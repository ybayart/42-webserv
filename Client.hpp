#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <map>

#include "messages.h"
#include "Logger.hpp"

#define BUFFER_SIZE 4096

extern Logger g_logger;

class Client
{
	friend class Server;
	friend class Handler;
	friend class Helper;

	typedef std::map<std::string, std::string> t_conf;
	struct t_chunk
	{
		unsigned int	len;
		bool			done;
		bool			found;
	};

	public:	
		enum status
		{
			PARSING,
			BODYPARSING,
			CODE,
			HEADERS,
			CGI,
			BODY,
			STANDBY,
			DONE
		};

		int			fd;
		int			file_fd;

		void		readFile();
		void		writeFile();

	private:
		int			port;
		int			status;
		int			cgi_pid;
		char		*rBuf;
		char		*wBuf;
		fd_set		*rSet;
		fd_set		*wSet;
		Request		req;
		Response	res;
		std::string	ip;
		std::string	file_str;
		std::string	tmp_path;
		std::string	last_date;
		t_conf 		conf;
		t_chunk		chunk;

	public:
		Client(int filed, fd_set *r, fd_set *w, struct sockaddr_in info);
		~Client();

		void	setReadState(bool state);
		void	setWriteState(bool state);
		void	setFileToRead(int fd, bool state);
		void	setFileToWrite(int fd, bool state);
		void	setToStandBy();
};

#endif