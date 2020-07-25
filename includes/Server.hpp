#ifndef Server_HPP
#define Server_HPP

#include <queue>
#include "Handler.hpp"

#define TIMEOUT 10
#define RETRY	"25"

class Server
{
	friend class Config;
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	private:
		int						_fd;
		int						_maxFd;
		int						_port;
		struct sockaddr_in		_info;
		fd_set					*_readSet;
		fd_set					*_writeSet;
		fd_set					*_rSet;
		fd_set					*_wSet;
		Handler					_handler;
		std::vector<config>		_conf;

	public:
		std::vector<Client*>	_clients;
		std::queue<int>			_tmp_clients;
		
		Server();
		~Server();

		int		getMaxFd();
		int		getFd() const;
		int		getOpenFd();
		void	init(fd_set *readSet, fd_set *writeSet, fd_set *rSet, fd_set *wSet);
		void	refuseConnection();
		void	acceptConnection();
		int		readRequest(std::vector<Client*>::iterator it);
		int		writeResponse(std::vector<Client*>::iterator it);
		void	send503(int fd);

	class		ServerException: public std::exception
	{
		private:
			std::string function;
			std::string error;

		public:
			ServerException(void);
			ServerException(std::string function, std::string error);
			virtual	~ServerException(void) throw();	
			virtual const char		*what(void) const throw();
	};

	private:
		int		getTimeDiff(std::string start);

};

#endif
