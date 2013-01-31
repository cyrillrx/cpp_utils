#pragma once

#include <string>

class Logger
{
public:
	Logger(const std::string& filename);
	
	void setLogFile(const std::string& filename);
	void debug(const std::string& message);
	void info(const std::string& message);
	void warning(const std::string& message);
	void error(const std::string& message);

private:
	std::string m_logFile;

	void write(const int& severity, const std::string& message);
	void writeInFile(const std::string& filename, const std::string& message);
};