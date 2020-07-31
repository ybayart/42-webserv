#ifndef HANDLER_HPP
#define HANDLER_HPP

#include <vector>
#include <dirent.h>
#include <sys/stat.h>

#include "Helper.hpp"

class Client;
class Helper;

class Handler
{
	typedef std::map<std::string, std::string> 	elmt;
	typedef std::map<std::string, elmt>			config;

	public:
		Handler();
		~Handler();

		Helper	_helper;

		void			parseRequest(Client &client, std::vector<config> &conf);
		void			parseBody(Client &client);
		void			dispatcher(Client &client);
		void			createResponse(Client &client);

	private:
		void			handleGet(Client &client);
		void			handleHead(Client &client);
		void			handlePost(Client &client);
		void			handlePut(Client &client);
		void			handleConnect(Client &client);
		void			handleTrace(Client &client);
		void			handleOptions(Client &client);
		void			handleDelete(Client &client);
		void			handleBadRequest(Client &client);

		void			getConf(Client &client, Request &req, std::vector<config> &conf);
		void			negotiate(Client &client);
		void			createListing(Client &client);
		bool			checkSyntax(const Request &request);
		bool			parseHeaders(std::string &buf, Request &req);
		void			getBody(Client &client);
		void			dechunkBody(Client &client);
		void			execCGI(Client &client);
		void			parseCGIResult(Client &client);

};

#endif
