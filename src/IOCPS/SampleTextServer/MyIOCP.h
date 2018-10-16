#pragma once

#include "MacroDefine.h"

typedef enum {
	IAPITYPE_NONE = (-1),
	IAPITYPE_NULL = 0,
	IAPITYPE_SUBSCRIBE,
	IAPITYPE_UNSUBSCRIBE,
}IAPITYPE;

static std::map<std::string, IAPITYPE> g_InterfaceMap = {
	{ "--", IAPITYPE_NULL },
	{ "subscribe", IAPITYPE_SUBSCRIBE },
	{ "unsubscribe", IAPITYPE_UNSUBSCRIBE },
};

class MyIOCP:public CTextIOCPServer
{
public:
	MyIOCP(void);
	MyIOCP(boost::mutex *p_deque_lock, std::deque<std::string> *p_deque_list);
	~MyIOCP(void);
	
public:

	virtual VOID NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext);
	virtual VOID NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer);
	virtual VOID NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText);

private:
	//Tick实例
	CMTTickInstance * m_pTickInstance;
	//默认(0),订阅-1,取消订阅-2
	std::map<LONGLONG, IAPITYPE> m_SessionList;

	// 行情队列锁
	boost::mutex		*m_p_deque_lock;
	// 行情队列
	std::deque<std::string> *m_p_deque_list;
public:
	CMTTickInstance * getTickInstance() { return m_pTickInstance; }
	void setTickInstance(CMTTickInstance *pTickInstance) { m_pTickInstance = pTickInstance; }
	std::map<LONGLONG, IAPITYPE> * getSessionList() { return &m_SessionList; }

	// 设置行情队列及队列锁
	void setDequeParams(boost::mutex *p_deque_lock, std::deque<std::string> *p_deque_list) { m_p_deque_lock = p_deque_lock; m_p_deque_list = p_deque_list; }
	// 行情队列锁
	boost::mutex		*getDequeLock() { return m_p_deque_lock; }
	// 行情队列
	std::deque<std::string> *getDequeList() { return m_p_deque_list; }
};

