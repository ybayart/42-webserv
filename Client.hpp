#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "string"
#include "map"
#include "messages.h"

class Client
{
	typedef std::map<std::string, std::string> conf;
	friend class Listener;
	friend class Handler;

	private:
		int			_fd;
		Request		_req;
		conf 		_conf;
		char		_rBuf[4096];
		char		_wBuf[4096];
		int			_rBytes;
		int			_wBytes;

	public:
		Client(int fd);
		~Client();
};

#endif