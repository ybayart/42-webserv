#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include "Logger.hpp"

Logger::Logger(bool on, std::string fileName, int param)
: _fileName(fileName), _param(param), _on(false)
{
	if (on)
		_on = true;
}

Logger::~Logger()
{

}

void			Logger::logToConsole(const std::string text)
{
	std::cout << text << std::endl;
}

void			Logger::logToFile(const std::string text)
{
	std::ofstream	file;

	file.open(_fileName, std::ofstream::app);
	file << text << std::endl;
	file.close();
}

std::string		Logger::makeLogEntry(const std::string text)
{
	std::string log;

	log += getTimestamp();
	log += " " + text;
	return (log);
}

void			Logger::log(std::string const &message, int value)
{
	if (_on && value <= _param)
	{
		if (_fileName == "console")
			logToConsole(makeLogEntry(message));
		else
			logToFile(makeLogEntry(message));
	}
}

std::string		Logger::getTimestamp(void)
{
	time_t 		now = 0;
	tm 			*ltm = NULL;
	char		buffer[1024];
	std::string result;
	
	now = time(0);
	if (now)
		ltm = localtime(&now);
	strftime(buffer, 1024, "%d/%m/%y %T", ltm);
	result = buffer;
	result.insert(result.begin(), '[');
	result.insert(result.end(), ']');
   	return (result);
}