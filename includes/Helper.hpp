#ifndef HELPER_HPP
#define HELPER_HPP

#include <sys/stat.h>
#include <sys/time.h>

#include "statusCode.h"
#include "Client.hpp"

class Helper
{
	public:
		std::map<std::string, std::string> MIMETypes;

		Helper();
		~Helper();

		std::string		getLastModified(std::string path);
		std::string		findType(Client &client);
		void			getErrorPage(Client &client);
		int				findLen(Client &client);
		void			fillBody(Client &client);
		int				fromHexa(const char *nb);
		char			**setEnv(Client &client);
		std::string		decode64(const char *data);
		void			parseAcceptLanguage(Client &client, std::multimap<std::string, std::string> &map);
		void			parseAcceptCharset(Client &client, std::multimap<std::string, std::string> &map);
		void			assignMIME();

		int				getStatusCode(Client &client);
		int				GETStatus(Client &client);
		int				POSTStatus(Client &client);
		int				PUTStatus(Client &client);
		int				CONNECTStatus(Client &client);
		int				TRACEStatus(Client &client);
		int				OPTIONSStatus(Client &client);
		int				DELETEStatus(Client &client);


};

#endif
