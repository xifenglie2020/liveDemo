#include "logger.h"
#include <stdio.h>
#include <string.h>
#include "nodelist.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define XT_LOG_LEVEL_OVERFLOW	(XT_LOG_LEVEL_FATAL+1)

class CLoggerPrint : public CLoggerInterface{
public:
	void log(const char *name, int level, const char *data){
		printf("[%lld][%d][%s]:%s\n", GetTickCount64(), level, name, data);
	}
	void close() {}
};
static CLoggerPrint _loggerPrint;


class CLoggerApplication {
public:
	CLoggerApplication() : m_fStarted(false){
		NODE_LIST_INIT(m_head, m_tail);
	}
	//从配置文件读取
	int  startup(const char *pathname){
		printf("LoggerApplication startup\n");
		m_fStarted = true;
		for (CLoggerHolder::node_t *p = m_head.next; p != &m_tail; p = p->next){
			doRegist(p->pthis);
		}
		return 0;
	}
	void cleanup(){
		for (CLoggerHolder::node_t *p = m_head.next; p != &m_tail; p = p->next){
			p->pthis->m_level = XT_LOG_LEVEL_OVERFLOW;
		}
		//关闭日志文件
	}
	void registerLogger(CLoggerHolder *logger){
		printf("LoggerApplication registerLogger %s\n", logger->m_name);
		NODE_LIST_ADD_TAIL(m_tail, &logger->node);
		if (m_fStarted){
			doRegist(logger);
		}
	}

	static CLoggerApplication *instance() {
		static CLoggerApplication *logger = NULL;
		if (logger == NULL) {
			logger = new CLoggerApplication();
			printf("new LoggerApplication \n");
		}
		return logger;
	}

	static void destroy() {
		CLoggerApplication *papp = instance();
		if (papp != NULL) {
			papp->cleanup();
			delete papp;
		}
	}
private:
	void doRegist(CLoggerHolder *p){
		p->m_level = XT_LOG_LEVEL_DEBUG;
		p->m_logger = &_loggerPrint;
		printf("LoggerApplication init %s\n", p->m_name);
	}
	bool m_fStarted;
	CLoggerHolder::node_t m_head;
	CLoggerHolder::node_t m_tail;
};



CLoggerHolder::CLoggerHolder(const char *name)
: m_name(name)
, m_logger(NULL)
, m_level(XT_LOG_LEVEL_WARN)
{
	node.pthis = this;
	CLoggerApplication::instance()->registerLogger(this);
}

//##__VA_ARGS__
void CLoggerHolder::log(int level, const char *fmt, ...){
	//if (level >= m_level && m_logger != NULL){
		char buffer[2048];
		va_list va;
		va_start(va, fmt);
		vsprintf(buffer, fmt, va);
		va_end(va);
		m_logger->log(m_name, level, buffer);
	//}
}

int logger_startup(const char *pathname){
	CLoggerApplication *papp = CLoggerApplication::instance();
	return papp->startup(pathname);
}

void logger_cleanup(){
	CLoggerApplication::destroy();
}


