#include "Helper.hpp"

static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

Helper::Helper()
{
	assignMIME();
}

Helper::~Helper()
{

}

std::string		Helper::findType(Request &req)
{
	std::string 	extension;

	if (req.uri.find_last_of('.') != std::string::npos)
	{
		extension = req.uri.substr(req.uri.find_last_of('.'));		
		if (MIMETypes.find(extension) != MIMETypes.end())
			return (MIMETypes[extension]);
		else
			return (MIMETypes[".bin"]);
	}
	return ("");
}

void			Helper::getErrorPage(Client &client)
{
	std::string		path;

	path = client.conf["error"] + "/" + client.res.status_code.substr(0, 3) + ".html";
	client.fileFd = open(path.c_str(), O_RDONLY);
}

std::string		Helper::getDate()
{
	struct timeval	time;
	struct tm		*tm;
	char			buf[BUFFER_SIZE + 1];
	int				ret;

	gettimeofday(&time, NULL);
	tm = localtime(&time.tv_sec);
	ret = strftime(buf, BUFFER_SIZE, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

std::string		Helper::getLastModified(std::string path)
{
	char		buf[BUFFER_SIZE + 1];
	int			ret;
	struct tm	*tm;
	struct stat	file_info;

	lstat(path.c_str(), &file_info);
	tm = localtime(&file_info.st_mtime);
	ret = strftime(buf, BUFFER_SIZE, "%a, %d %b %Y %T %Z", tm);
	buf[ret] = '\0';
	return (buf);
}

int				Helper::findLen(Client &client)
{
	std::string		to_convert;
	int				len;
	std::string		tmp;

	to_convert = client.rBuf;
	to_convert = to_convert.substr(0, to_convert.find("\r\n"));
	// std::cout << to_convert << ";" << std::endl;
	len = fromHexa(to_convert.c_str());
	// std::cout << "l: " << len << " / " << client.port << std::endl;
	tmp = client.rBuf;
	tmp = tmp.substr(tmp.find("\r\n") + 2);
	strcpy(client.rBuf, tmp.c_str());
	return (len);
}

void			Helper::fillStatus(Client &client)
{
	std::string		status;

	status = client.res.version + " " + client.res.status_code + "\r\n";
	strcpy(client.wBuf, status.c_str());
	client.status = HEADERS;
}

void			Helper::fillHeaders(Client &client)
{
	std::string		result;
	std::map<std::string, std::string>::const_iterator b;

	b = client.res.headers.begin();
	while (b != client.res.headers.end())
	{
		if (b->second != "")
			result += b->first + ": " + b->second + "\r\n";
		++b;
	}
	result += "\r\n";
	strcpy(client.wBuf, result.c_str());
	client.status = BODY;
}

void			Helper::fillBody(Client &client)
{
	std::string		tmp;

	tmp = client.rBuf;
	if (tmp.size() > client.chunk.len)
	{
		client.req.body += tmp.substr(0, client.chunk.len);
		tmp = tmp.substr(client.chunk.len + 1);
		memset(client.rBuf, 0, BUFFER_SIZE + 1);
		strcpy(client.rBuf, tmp.c_str());
		client.chunk.len = 0;
		client.chunk.found = false;
	}
	else
	{
		client.req.body += tmp;
		client.chunk.len -= tmp.size();
		memset(client.rBuf, 0, BUFFER_SIZE + 1);
	}
	// std::cout << "body size: " << client.req.body.size() << " [" << client.ip << ":" << client.port << "]\n";
}			

int				Helper::ft_power(int nb, int power)
{
	if (power < 0)
		return (0);
	if (power == 0)
		return (1);
	return (nb * ft_power(nb, power - 1));
}

int				Helper::fromHexa(const char *nb)
{
	char	base[17] = "0123456789abcdef";
	char	base2[17] = "0123456789ABCDEF";
	int		result = 0;
	int		i;
	int		index;

	i = 0;
	while (nb[i])
	{
		int j = 0;
		while (base[j])
		{
			if (nb[i] == base[j])
			{
				index = j;
				break ;
			}
			j++;
		}
		if (j == 16)
		{
			j = 0;
			while (base2[j])
			{
				if (nb[i] == base2[j])
				{
					index = j;
					break ;
				}
				j++;
			}
		}
		result += index * ft_power(16, (strlen(nb) - 1) - i);
		i++;
	}
	return (result);
}

std::string		Helper::decode64(const char *data)
{
	while (*data != ' ')
		data++;
	data++;
	int len = strlen(data);
	unsigned char* p = (unsigned char*)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;
    std::string str(L / 4 * 3 + pad, '\0');

    for (size_t i = 0, j = 0; i < L; i += 4)
    {
        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad)
    {
        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
        str[str.size() - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=')
        {
            n |= B64index[p[L + 2]] << 6;
            str.push_back(n >> 8 & 0xFF);
        }
    }
    if (str.back() == 0)
    	str.pop_back();
    return (str);
}

void			Helper::parseAcceptLanguage(Client &client, std::multimap<std::string, std::string> &map)
{
	std::string							language;
	std::string							to_parse;
	std::string							q;

	to_parse = client.req.headers["Accept-Language"];
	std::cout << to_parse << std::endl;
	int i = 0;
	while (to_parse[i] != '\0' && to_parse[i] != '\r')
	{
		language.clear();
		q.clear();
		while (to_parse[i] && to_parse[i] != ',' && 
		to_parse[i] != ';' && to_parse[i] != '\r')
		{
			language += to_parse[i];
			++i;
		}
		if (to_parse[i] == ',' || to_parse[i] == '\r')
			q = "1";
		else if (to_parse[i] == ';')
		{
			i += 3;
			while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != '\r')
			{
				q += to_parse[i];
				++i;
			}
		}
		++i;
		std::pair<std::string, std::string>	pair(q, language);
		map.insert(pair);
	}
	// for (std::multimap<std::string, std::string>::iterator it(map.begin()); it != map.end(); ++it)
	// {
	// 	std::cout << it->first + ":" + it->second << std::endl;
	// }
}

void			Helper::parseAcceptCharsets(Client &client, std::multimap<std::string, std::string> &map)
{
	std::string							charset;
	std::string							to_parse;
	std::string							q;

	to_parse = client.req.headers["Accept-Charsets"];
	std::cout << to_parse << std::endl;
	// int i = 0;
	// while (to_parse[i] != '\0' && to_parse[i] != '\r')
	// {
	// 	charset.clear();
	// 	q.clear();
	// 	while (to_parse[i] && to_parse[i] != ';' && to_parse[i] != '\r')
	// 	{
	// 		charset += to_parse[i];
	// 		++i;
	// 	}
	// 	if (to_parse[i] == ';')
	// 	{
	// 		if (to_parse[i + 1] == 'q' && to_parse[i + 2] == '=')
	// 			i += 3;
	// 	}
	// 	while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != '\r')
	// 	{
	// 		q += to_parse[i];
	// 		++i;
	// 	}
	// 	if (to_parse[i] == ',')
	// 		++i;
	// 	map[q] = charset;
	// }
}



char			**Helper::setEnv(Client &client)
{
	char											**env;
	std::map<std::string, std::string> 				envMap;

	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";

	envMap["CONTENT_LENGTH"] = std::to_string(client.req.body.size());
	if (client.req.headers.find("Content-Type") != client.req.headers.end())
		envMap["CONTENT_TYPE"] = client.req.headers["Content-Type"];
	envMap["PATH_INFO"] = client.req.uri;
	envMap["PATH_TRANSLATED"] = client.conf["path"];
	envMap["QUERY_STRING"] = client.req.uri.substr(client.req.uri.find('?') + 1);
	if (client.conf.find("exec") != client.conf.end())
		envMap["SCRIPT_NAME"] = client.conf["exec"];
	else
		envMap["SCRIPT_NAME"] = client.req.uri.substr(client.req.uri.find_last_of('/'));
	envMap["SERVER_NAME"] = "localhost";
	envMap["SERVER_PORT"] = "8080";
	envMap["REQUEST_URI"] = client.req.uri;
	envMap["REQUEST_METHOD"] = client.req.method;
	envMap["REMOTE_ADDR"] = client.ip;

	std::map<std::string, std::string>::iterator b = client.req.headers.begin();
	while (b != client.req.headers.end())
	{
		envMap["HTTP_" + b->first] = b->second;
		++b;
	}
	env = (char **)malloc(sizeof(char *) * (envMap.size() + 1));
	std::map<std::string, std::string>::iterator it = envMap.begin();
	int i = 0;
	while (it != envMap.end())
	{
		env[i] = strdup((it->first + "=" + it->second).c_str());
		++i;
		++it;
	}
	env[i] = NULL;
	return (env);
}

void			Helper::freeAll(char **args, char **env)
{
	free(args[0]);
	free(args);
	int i = 0;
	while (env[i])
	{
		free(env[i]);
		++i;
	}
	free(env);
}

void			Helper::assignMIME()
{
	MIMETypes[".txt"] = "text/plain";
	MIMETypes[".bin"] = "application/octet-stream";
	MIMETypes[".jpeg"] = "image/jpeg";
	MIMETypes[".jpg"] = "image/jpeg";
	MIMETypes[".html"] = "text/html";
	MIMETypes[".htm"] = "text/html";
	MIMETypes[".png"] = "image/png";
	MIMETypes[".bmp"] = "image/bmp";
	MIMETypes[".pdf"] = "application/pdf";
	MIMETypes[".tar"] = "application/x-tar";
	MIMETypes[".json"] = "application/json";
	MIMETypes[".css"] = "text/css";
	MIMETypes[".js"] = "application/javascript";
	MIMETypes[".mp3"] = "audio/mpeg";
	MIMETypes[".avi"] = "video/x-msvideo";
}