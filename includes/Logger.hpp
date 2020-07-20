#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>

#define LOW 	1
#define MED 	2
#define HIGH	3

class Logger
{
	public:
		Logger(bool on, std::string fileName, int param);
		~Logger();
		
		void	log(std::string const &message, int value);
		
	private:
		void			logToConsole(const std::string text);
		void			logToFile(const std::string text);
		std::string		getTimestamp(void);
		std::string		makeLogEntry(const std::string text);

		std::string		_fileName;
		int				_param;
		bool			_on;
};

#endif