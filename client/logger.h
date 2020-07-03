#pragma once

#include <stdlib.h>
//#include <log4cpp/Category.hh>

#define XT_LOG_LEVEL_DEBUG	1
#define XT_LOG_LEVEL_INFO	2
#define XT_LOG_LEVEL_NOTICE	3
#define XT_LOG_LEVEL_WARN	4
#define XT_LOG_LEVEL_ERROR	5
#define XT_LOG_LEVEL_FATAL	6

//给外部应用层创建
class CLoggerInterface{
public:
	virtual void log(const char *name, int level, const char *format) = 0;
};

class CLoggerHolder{
	friend class CLoggerApplication;
public:
	struct node_t{
		node_t *next;
		node_t *prev;
		CLoggerHolder *pthis;
	}node;
public:
	CLoggerHolder(const char *name);
	//##__VA_ARGS__
	void log(int level, const char *fmt, ...);
	bool can(int level) { return level >= m_level && m_logger != NULL; }
private:
	CLoggerHolder(){};
	CLoggerHolder(CLoggerHolder &other){}
private:
	const char *m_name;
	CLoggerInterface *m_logger;
	int m_level;
};


#define LOGGER_Declare(_name)		static CLoggerHolder _logger(_name)

#define LOGGER_log(level, _Fmt, ...)	if(_logger.can(level))	_logger.log(level,_Fmt, ##__VA_ARGS__)

#define LOGGER_Debug(_Fmt, ...)		LOGGER_log(XT_LOG_LEVEL_DEBUG,_Fmt, ##__VA_ARGS__)
#define LOGGER_Info(_Fmt, ...)		LOGGER_log(XT_LOG_LEVEL_INFO,_Fmt, ##__VA_ARGS__)
#define LOGGER_Notice(_Fmt, ...)	LOGGER_log(XT_LOG_LEVEL_NOTICE,_Fmt, ##__VA_ARGS__)
#define LOGGER_Warn(_Fmt, ...)		LOGGER_log(XT_LOG_LEVEL_WARN,_Fmt, ##__VA_ARGS__)
#define LOGGER_Error(_Fmt, ...)		LOGGER_log(XT_LOG_LEVEL_ERROR,_Fmt, ##__VA_ARGS__)
#define LOGGER_Fatal(_Fmt, ...)		LOGGER_log(XT_LOG_LEVEL_FATAL,_Fmt, ##__VA_ARGS__)


int  logger_startup(const char *pathname);
void logger_cleanup();


