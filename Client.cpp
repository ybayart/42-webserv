#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _rBytes(0), _wBytes(0)
{

}

Client::~Client()
{

}