#include "StdAfx.h"
#include "MyIOCP.h"
#include "IOCPQueue.h"
#include <thread>

MyIOCP::MyIOCP(boost::mutex *p_deque_lock, std::deque<std::string> *p_deque_list)
{
	setDequeParams(p_deque_lock, p_deque_list);
	// �������������߳�
	std::thread([](void *p)->void {
		MyIOCP * pMyIOCP = (MyIOCP *)p;
		while (1)
		{
			PER_SOCKET_CONTEXT_LIST * pPSCL = pMyIOCP->getPerSocketContextList();
			if (pPSCL)
			{
				if (pMyIOCP->getSessionList()->size())
				{
					for (auto it = pMyIOCP->getSessionList()->begin(); it != pMyIOCP->getSessionList()->end(); )
					{
						PPER_SOCKET_CONTEXT pPSC = pPSCL->GetContext(it->first);
						if (pPSC)
						{
							if (it->second == IAPITYPE_SUBSCRIBE)
							{
								// ��ѯ����
								while (pMyIOCP->getDequeList()->size())
								{
									pMyIOCP->getDequeLock()->lock();
									std::string s = pMyIOCP->getDequeList()->front();
									pMyIOCP->getDequeList()->pop_front();
									pMyIOCP->getDequeLock()->unlock();
									pMyIOCP->SendEx(pPSC, s.c_str());
								}
							}
							it++;
						}
						else
						{
							it = pMyIOCP->getSessionList()->erase(it);
						}
					}
				}
				else
				{
					if (pMyIOCP->getDequeList()->size())
					{
						pMyIOCP->getDequeList()->clear();
					}
				}
			}
			// �ͷ�CPU��Ƭ
			Sleep(200);
		}

		return;
	}, this).detach();
}

MyIOCP::MyIOCP(void)
{
	// �������������߳�
	std::thread([](void *p)->void {
		MyIOCP * pMyIOCP = (MyIOCP *)p;
		while (1)
		{
			PER_SOCKET_CONTEXT_LIST * pPSCL = pMyIOCP->getPerSocketContextList();
			if (pPSCL)
			{
				if (pMyIOCP->getSessionList()->size())
				{
					for (auto it = pMyIOCP->getSessionList()->begin(); it != pMyIOCP->getSessionList()->end(); )
					{
						PPER_SOCKET_CONTEXT pPSC = pPSCL->GetContext(it->first);
						if (pPSC)
						{
							if (it->second == IAPITYPE_SUBSCRIBE)
							{
								// ��ѯ����
								while (pMyIOCP->getDequeList()->size())
								{
									pMyIOCP->getDequeLock()->lock();
									std::string s = pMyIOCP->getDequeList()->front();
									pMyIOCP->getDequeList()->pop_front();
									pMyIOCP->getDequeLock()->unlock();
									pMyIOCP->SendEx(pPSC, s.c_str());
								}
							}
							it++;
						}
						else
						{
							it = pMyIOCP->getSessionList()->erase(it);
						}
					}
				}
				else
				{
					if (pMyIOCP->getDequeList()->size())
					{
						pMyIOCP->getDequeList()->clear();
					}
				}
			}
			// �ͷ�CPU��Ƭ
			Sleep(200);
		}

		return;
	}, this).detach();
}


MyIOCP::~MyIOCP(void)
{
}

VOID MyIOCP::NotifyNewConnection(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	__super::NotifyNewConnection(lpPerSocketContext);

	// ������ӻỰ���б�
	m_SessionList.insert(std::map<LONGLONG, IAPITYPE>::value_type(lpPerSocketContext->m_guid, IAPITYPE_NULL));
}

VOID MyIOCP::NotifyDisconnectedClient(PPER_SOCKET_CONTEXT lpPerSocketContext)
{
	__super::NotifyDisconnectedClient(lpPerSocketContext);

	auto it = m_SessionList.find(lpPerSocketContext->m_guid);
	if (it != m_SessionList.end())
	{
		// �޸��б����ӻỰΪ����
		it->second = IAPITYPE_NONE;
	}
}

VOID MyIOCP::NotifyReceivedPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, CIOCPBuffer* pBuffer)
{
	__super::NotifyReceivedPackage(lpPerSocketContext, pBuffer);
}

VOID MyIOCP::NotifyReceivedFormatPackage(PPER_SOCKET_CONTEXT lpPerSocketContext, LPCSTR lpszText)
{
	__super::NotifyReceivedFormatPackage(lpPerSocketContext, lpszText);

	//sockaddr_in sin;
	//int sin_len = sizeof(sin);
	//getpeername(lpPerSocketContext->m_Socket,(sockaddr *)&sin,&sin_len);
	//printf("ReceivePackage:%s from %s\n",lpszText,inet_ntoa(sin.sin_addr));
	auto it = g_InterfaceMap.find(lpszText);
	if (it != g_InterfaceMap.end())
	{
		switch ((IAPITYPE)it->second)
		{
		case IAPITYPE_SUBSCRIBE://����
		{
			auto sit = m_SessionList.find(lpPerSocketContext->m_guid);
			if (sit != m_SessionList.end())
			{
				sit->second = IAPITYPE_SUBSCRIBE;
			}
		}
		break;
		case IAPITYPE_UNSUBSCRIBE://ȡ������
		{
			auto sit = m_SessionList.find(lpPerSocketContext->m_guid);
			if (sit != m_SessionList.end())
			{
				sit->second = IAPITYPE_UNSUBSCRIBE;
			}
		}
		break;
		default:
		{

		}
		break;
		}
	}
	//SendEx(lpPerSocketContext,lpszText);
}