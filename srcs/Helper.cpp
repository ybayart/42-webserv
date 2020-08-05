#include "utils.h"
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

std::string		Helper::findType(Client &client)
{
	std::string 	extension;
	size_t			pos;

	if (client.conf["path"].find_last_of('.') != std::string::npos)
	{
		pos = client.conf["path"].find('.');
		extension = client.conf["path"].substr(pos, client.conf["path"].find('.', pos + 1) - pos);
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
	client.conf["path"] = path;
	client.read_fd = open(path.c_str(), O_RDONLY);
}

std::string		Helper::getLastModified(std::string path)
{
	char		buf[BUFFER_SIZE + 1];
	int			ret;
	struct tm	*tm;
	struct stat	file_info;

	if (lstat(path.c_str(), &file_info) == -1)
		return ("");
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
	while (to_convert[0] == '\n')
		to_convert.erase(to_convert.begin());
	if (to_convert.size() == 0)
		len = 0;
	else
		len = fromHexa(to_convert.c_str());
	len = fromHexa(to_convert.c_str());
	tmp = client.rBuf;
	tmp = tmp.substr(tmp.find("\r\n") + 2);
	strcpy(client.rBuf, tmp.c_str());
	return (len);
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
		result += index * ft::getpower(16, (strlen(nb) - 1) - i);
		i++;
	}
	return (result);
}

std::string		Helper::decode64(const char *data)
{
	while (*data != ' ')
		data++;
	data++;
	unsigned int len = strlen(data);
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
	int i = 0;
	while (to_parse[i] != '\0')
	{
		language.clear();
		q.clear();
		while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != ';')
		{
			language += to_parse[i];
			++i;
		}
		if (to_parse[i] == ',' || to_parse[i] == '\0')
			q = "1";
		else if (to_parse[i] == ';')
		{
			i += 3;
			while (to_parse[i] && to_parse[i] != ',')
			{
				q += to_parse[i];
				++i;
			}
		}
		if (to_parse[i])
			++i;
		std::pair<std::string, std::string>	pair(q, language);
		map.insert(pair);
	}
}

void			Helper::parseAcceptCharset(Client &client, std::multimap<std::string, std::string> &map)
{
	std::string							charset;
	std::string							to_parse;
	std::string							q;

	to_parse = client.req.headers["Accept-Charset"];
	int i = 0;
	while (to_parse[i] != '\0')
	{
		charset.clear();
		q.clear();
		while (to_parse[i] && to_parse[i] != ',' && to_parse[i] != ';')
		{
			charset += to_parse[i];
			++i;
		}
		if (to_parse[i] == ',' || to_parse[i] == '\0')
			q = "1";
		else if (to_parse[i] == ';')
		{
			i += 3;
			while (to_parse[i] && to_parse[i] != ',')
			{
				q += to_parse[i];
				++i;
			}
		}
		if (to_parse[i])
			++i;
		std::pair<std::string, std::string>	pair(q, charset);
		map.insert(pair);
	}
}

char			**Helper::setEnv(Client &client)
{
	char											**env;
	std::map<std::string, std::string> 				envMap;
	size_t											pos;

	envMap["GATEWAY_INTERFACE"] = "CGI/1.1";
	envMap["SERVER_PROTOCOL"] = "HTTP/1.1";
	envMap["SERVER_SOFTWARE"] = "webserv";
	envMap["REQUEST_URI"] = client.req.uri;
	envMap["REQUEST_METHOD"] = client.req.method;
	envMap["REMOTE_ADDR"] = client.ip;
	envMap["PATH_INFO"] = client.req.uri;
	envMap["PATH_TRANSLATED"] = client.conf["path"];
	envMap["CONTENT_LENGTH"] = std::to_string(client.req.body.size());

	if (client.req.uri.find('?') != std::string::npos)
		envMap["QUERY_STRING"] = client.req.uri.substr(client.req.uri.find('?') + 1);
	else
		envMap["QUERY_STRING"];
	if (client.req.headers.find("Content-Type") != client.req.headers.end())
		envMap["CONTENT_TYPE"] = client.req.headers["Content-Type"];
	if (client.conf.find("exec") != client.conf.end())
		envMap["SCRIPT_NAME"] = client.conf["exec"];
	else
		envMap["SCRIPT_NAME"] = client.conf["path"];
	if (client.conf["listen"].find(":") != std::string::npos)
	{
		envMap["SERVER_NAME"] = client.conf["listen"].substr(0, client.conf["listen"].find(":"));
		envMap["SERVER_PORT"] = client.conf["listen"].substr(client.conf["listen"].find(":") + 1);
	}
	else
		envMap["SERVER_PORT"] = client.conf["listen"];
	if (client.req.headers.find("Authorization") != client.req.headers.end())
	{
		pos = client.req.headers["Authorization"].find(" ");
		envMap["AUTH_TYPE"] = client.req.headers["Authorization"].substr(0, pos);
		envMap["REMOTE_USER"] = client.req.headers["Authorization"].substr(pos + 1);
		envMap["REMOTE_IDENT"] = client.req.headers["Authorization"].substr(pos + 1);
	}
	if (client.conf.find("php") != client.conf.end() && client.req.uri.find(".php") != std::string::npos)
		envMap["REDIRECT_STATUS"] = "200";

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
