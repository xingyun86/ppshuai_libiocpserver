#pragma once

//--- headers
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#include <Winsock2.h>
//--- API
#include "..\Manager\API\MT5APIManager.h"
//+------------------------------------------------------------------+
#include "..\DealerSink.h"

//---
#define MT5_TIMEOUT_CONNECT   30000
#define MT5_TIMEOUT_RESCAN    10000
#define MT5_TIMEOUT_DEALER    10000

//+------------------------------------------------------------------+

#include <deque>
#include <string>
#include <functional>
#include <iostream>
//#include <mutex>
//#include <set>
//#include <map>
//#include <vector>
//#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <redisclient/redissyncclient.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/locale.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <redisclient/redisasyncclient.h>

#include <MarketServerPublisher.h>

using boost::property_tree::ptree;
using boost::property_tree::ptree_error;

typedef enum service_interface_api_type {
	si_apitype_test = 0,

	si_apitype_market_kline,
	si_apitype_market_kline_new,

	si_apitype_trade_symbol_query,
	si_apitype_trade_position_query,
	si_apitype_trade_history_order_query,
	si_apitype_trade_order_query,
	si_apitype_trade_position_open,
	si_apitype_trade_position_close,
	si_apitype_trade_position_tpsl,
	si_apitype_trade_order_pending,
	si_apitype_trade_order_update,
	si_apitype_trade_order_cancel,

	si_apitype_account_register,
	si_apitype_account_name_update,
	si_apitype_account_password_check,
	si_apitype_account_password_update,
	si_apitype_account_balance_deposit,
	si_apitype_account_balance_withdrawal,
	si_apitype_account_balance_query,

	si_apitype_mt5_kcharts,
} web_api_type;
static std::map<std::string, web_api_type> g_service_interface_apimap = {
	{ "/test", si_apitype_test },
	//K线数据申请
	{ "/v1/market/kline_new", si_apitype_market_kline_new },
	{ "/v1/market/kline", si_apitype_market_kline },

	//品种查询
	{ "/v1/trade/symbol/query", si_apitype_trade_symbol_query },
	//持仓查询
	{ "/v1/trade/position/query", si_apitype_trade_position_query },
	//历史订单查询
	{ "/v1/trade/historyorder/query", si_apitype_trade_history_order_query },
	//订单查询
	{ "/v1/trade/order/query", si_apitype_trade_order_query },
	//交易市价/限价开仓
	{ "/v1/trade/position/open", si_apitype_trade_position_open },
	//交易市价/限价平仓
	{ "/v1/trade/position/close", si_apitype_trade_position_close },
	//添加止盈止损
	{ "/v1/trade/position/tpsl", si_apitype_trade_position_tpsl },
	//交易限价挂单
	{ "/v1/trade/order/pending", si_apitype_trade_order_pending },
	//交易限价修改
	{ "/v1/trade/order/update", si_apitype_trade_order_update },
	//取消委托单
	{ "/v1/trade/order/cancel", si_apitype_trade_order_cancel },

	//注册账户
	{ "/v1/account/register", si_apitype_account_register },
	//更新账户名称
	{ "/v1/account/name/update", si_apitype_account_name_update },
	//账户密码检查
	{ "/v1/account/password/check", si_apitype_account_password_check },
	//更新账户密码
	{ "/v1/account/password/update", si_apitype_account_password_update },
	//入金
	{ "/v1/account/balance/deposit", si_apitype_account_balance_deposit },
	//出金
	{ "/v1/account/balance/withdrawal", si_apitype_account_balance_withdrawal },
	//账户查询
	{ "/v1/account/balance/query", si_apitype_account_balance_query },

	{ "/v1/mt5/kcharts", si_apitype_mt5_kcharts },
};

__inline static bool ParseJsonData(std::map<std::string, std::string> & odata, std::string &str)
{
	try
	{
		ptree pt;
		std::stringstream stream(str);
		read_json(stream, pt);
		BOOST_FOREACH(auto &v, odata)
		{
			v.second = pt.get<std::string>(v.first);
		}
	}
	catch (ptree_error pt)
	{
		pt.what();
		return false;
	}
	return true;
}
//+------------------------------------------------------------------+
//| Tick events notification interface                               |
//+------------------------------------------------------------------+

class CMTTickInstance : 
	public IMTTickSink,
	public IMTOrderSink, 
	public IMTConSymbolSink
{
public:
	//+------------------------------------------------------------------+
	//| Constructor                                                      |
	//+------------------------------------------------------------------+
	CMTTickInstance(void) : 
		m_apiFactory(NULL),
		m_managerApi(NULL),
		m_adminApi(NULL),
		m_redis_publisher(NULL),
		m_real_tick_thread(NULL)
	{
		//Initialize redis publish
		//putenv("REDIS_HOST=192.168.31.86");
		const char * host = getenv("REDIS_HOST");
		boost::asio::ip::address address = boost::asio::ip::address::from_string(host ? host : "127.0.0.1");
		const unsigned short port = 6379;
		
		m_redis_publisher = new CRedisPublisher(m_ioService, address, port);
		m_real_tick_thread = new CMTThread();
		m_apiFactory = new CMTManagerAPIFactory();
	}
	//+------------------------------------------------------------------+
	//| Destructor                                                       |
	//+------------------------------------------------------------------+
	~CMTTickInstance(void)
	{
		Stop();
	}
	//+------------------------------------------------------------------+
	//| CMTTickInstance release function                                 |
	//+------------------------------------------------------------------+
	void Release(void)
	{
		delete this;
	}

	// 接收实时Ticks线程处理函数
	static unsigned int __stdcall real_tick_recv(void * p)
	{
		unsigned int result = (0);
		CMTTickInstance * pPI = (CMTTickInstance *)p;
		if (pPI)
		{
			pPI->getRedisPublisher()->start();
			pPI->getIOServeice()->run();
		}
		std::cout << "done\n";

		return result;
	}
	//////////////////////////////////////////////////////////////////////
	// Usage使用说明提示
	void Usage(void)
	{
		wchar_t name[_MAX_PATH] = L"", *ptr = NULL;
		//--- get module file name
		GetModuleFileName(NULL, name, _countof(name));
		ptr = wcsrchr(name, L'\\');
		if (ptr != NULL) ptr++;
		else          ptr = name;
		//--- show usage
		wprintf(L"\nUsage:\n"
			L"  %s /server:address:port /login:login /password:password\n", ptr);
	}

	//+------------------------------------------------------------------+
	//| CMTTickInstance start notification function                      |
	//+------------------------------------------------------------------+
	MTAPIRES Start(WCHAR * wServer, UINT64 uLoginId, WCHAR * wPassword)
	{
		MTAPIRES	retcode = MT_RET_OK_NONE;
		UINT		version = 0;
		MTAPIRES	res = MT_RET_OK_NONE;
		MTAPISTR	strapi;
		//--- show the starting banner
#ifdef _WIN64
		wprintf_s(L"SimpleManager 64 bit\nCopyright 2001-2018, MetaQuotes Software Corp.\n\n");
#else
		wprintf_s(L"SimpleManager 32 bit\nCopyright 2001-2018, MetaQuotes Software Corp.\n\n");
#endif
		//--- get parameters
		//--- check and request parameters
		//--- check parameters
		if (wServer[0] == L'\0' || uLoginId == 0 || wPassword[0] == L'\0')
		{
			Usage();
			return(-1);
		}

		if (!m_apiFactory)
		{
			wprintf_s(L"Initial manager api factory failed !\n");
			return(-1);
		}
		//--- loading manager API
		if ((res = m_apiFactory->Initialize(L".\\")) != MT_RET_OK)
		{
			wprintf_s(L"Loading manager API failed (%u)\n", res);
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		//--- check Manager API version
		if ((res = m_apiFactory->Version(version)) != MT_RET_OK)
		{
			wprintf_s(L"Getting API version failed (%u)\n", res);
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		wprintf_s(L"Manager API version %u has been loaded\n", version);
		if (version < MTManagerAPIVersion)
		{
			wprintf_s(L"Wrong Manager API version %u, version %u required\n", version, MTManagerAPIVersion);
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		MTAPISTR server_name;
		//--- creating manager interface
		if ((res = m_apiFactory->CreateAdmin(MTManagerAPIVersion, L".\\AdminData\\", &m_adminApi)) != MT_RET_OK)
		{
			wprintf_s(L"Creating administrator interface failed (%u)\n", res);
			m_managerApi->Release();
			m_managerApi = NULL;
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		//1001为Administrators,没有下单权限
		//uLoginId = 1000;
		//wPassword = L"dsowdsfwe12";
		//--- connect
		res = m_adminApi->Connect(wServer, uLoginId, wPassword, L"", 0, MT5_TIMEOUT_CONNECT);
		if (res != MT_RET_OK)
		{
			wprintf_s(L"Connection failed (%u)\n", res);
			m_managerApi->Release();
			m_managerApi = NULL;
			m_adminApi->Release();
			m_adminApi = NULL;
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		//--- get server name and start network rescan

		if (m_adminApi->NetworkServer(server_name) == MT_RET_OK && m_adminApi->NetworkRescan(0, 10000) == MT_RET_OK)
		{
			wprintf_s(L"Reconnecting due better access point is available\n");
			//--- disconnect
			m_adminApi->Disconnect();
			//--- connect again to server by name
			res = m_adminApi->Connect(server_name, uLoginId, wPassword, L"", 0, MT5_TIMEOUT_CONNECT);
			if (res != MT_RET_OK)
			{
				wprintf_s(L"Connection failed (%u)\n", res);
				m_adminApi->Release();
				m_adminApi = NULL;
				delete m_apiFactory;
				m_apiFactory = NULL;
				return(-1);
			}
		}
		//--- creating manager interface
		if ((res = m_apiFactory->CreateManager(MTManagerAPIVersion, L".\\ManagerData\\", &m_managerApi)) != MT_RET_OK)
		{
			wprintf_s(L"Creating manager interface failed (%u)\n", res);
			m_adminApi->Release();
			m_adminApi = NULL;
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		
		//--- connect
		res = m_managerApi->Connect(wServer, uLoginId, wPassword, L"", IMTManagerAPI::PUMP_MODE_FULL, MT5_TIMEOUT_CONNECT);
		if (res != MT_RET_OK)
		{
			wprintf_s(L"Connection failed (%u)\n", res);
			m_managerApi->Release();
			m_managerApi = NULL;
			m_adminApi->Release();
			m_adminApi = NULL;
			delete m_apiFactory;
			m_apiFactory = NULL;
			return(-1);
		}
		//--- get server name and start network rescan
		if (m_managerApi->NetworkServer(server_name) == MT_RET_OK && m_managerApi->NetworkRescan(0, MT5_TIMEOUT_RESCAN) == MT_RET_OK)
		{
			wprintf_s(L"Reconnecting due better access point is available\n");
			//--- disconnect
			m_managerApi->Disconnect();
			//--- connect again to server by name
			res = m_managerApi->Connect(server_name, uLoginId, wPassword, L"", 0, MT5_TIMEOUT_CONNECT);
			if (res != MT_RET_OK)
			{
				wprintf_s(L"Connection failed (%u)\n", res);
				m_managerApi->Release();
				m_managerApi = NULL;
				m_adminApi->Release();
				m_adminApi = NULL;
				delete m_apiFactory;
				m_apiFactory = NULL;
				return(-1);
			}
		}
		
		//--- start publish
		if (m_real_tick_thread && m_redis_publisher)
		{
			m_real_tick_thread->Start(real_tick_recv, this, 0);
		}
		//--- subscribe order events
		if ((retcode = m_managerApi->OrderSubscribe(this)) != MT_RET_OK)
		{
			m_managerApi->LoggerOut(MTLogOK, L"Order subscribe failed [%d]", retcode);
		}
		//--- subscribe symbol events
		if ((retcode = m_managerApi->SymbolSubscribe(this)) != MT_RET_OK)
		{
			m_managerApi->LoggerOut(MTLogOK, L"Symbol subscribe failed [%d]", retcode);
		}
		//--- Subscribe symbol selected events
		LPWSTR lpSymbolArray[] = { L"AUDJPY",L"XAGUSD",L"XAUUSD", };
		INT nSymbolArray = sizeof(lpSymbolArray) / sizeof(*lpSymbolArray);
		if ((retcode = m_managerApi->SelectedAddBatch(lpSymbolArray, nSymbolArray)) != MT_RET_OK)
		//if ((retcode = m_managerApi->SelectedAddAll()) != MT_RET_OK)
		{
			m_managerApi->LoggerOut(MTLogOK, L"Subsctibe symbol selected add all failed [%d]", retcode);
		}
		//--- subscribe plugin on tick events
		if ((retcode = m_managerApi->TickSubscribe(this)) != MT_RET_OK)
		{
			m_managerApi->LoggerOut(MTLogOK, L"Tick subscribe failed [%d]", retcode);
		}

		return(MT_RET_OK);
	}
	//+------------------------------------------------------------------+
	//| CMTTickInstance stop notification function                       |
	//+------------------------------------------------------------------+
	MTAPIRES Stop(void)
	{
		if (m_adminApi)
		{
			//--- disconnect
			m_adminApi->Disconnect();
			//--- release
			m_adminApi->Release();
			//--- clear API 
			m_adminApi = NULL;
		}
		//--- unsubscribe from all events
		if (m_managerApi)
		{
			//--- unsubscribe symbol
			m_managerApi->SymbolUnsubscribe(this);
			//--- unsubscribe tick
			m_managerApi->TickUnsubscribe(this);
			//--- unsubscribe order
			m_managerApi->OrderUnsubscribe(this);
			//--- disconnect
			m_managerApi->Disconnect();
			//--- release
			m_managerApi->Release();
			//--- clear API 
			m_managerApi = NULL;
		}
		if (m_apiFactory)
		{
			//--- shutdown
			m_apiFactory->Shutdown();
			delete m_apiFactory;
			m_apiFactory = NULL;
		}

		//--- ok
		if (m_real_tick_thread && m_redis_publisher)
		{
			m_redis_publisher->stop();
			m_real_tick_thread->Shutdown(WAIT_TIMEOUT);
			m_real_tick_thread->Terminate();
			delete m_redis_publisher;
			m_redis_publisher = NULL;
		}

		return(MT_RET_OK);
	}
private:
	CMTManagerAPIFactory        *m_apiFactory;
	CMTManagerAPIFactory        *m_apiAdminFactory;
	IMTManagerAPI				*m_managerApi;
	IMTAdminAPI					*m_adminApi;
public:
	//+------------------------------------------------------------------+
	//| Process "symbol update" notification                             | 
	//+------------------------------------------------------------------+
	void OnSymbolUpdate(const IMTConSymbol* config)
	{
		//--- check
		if (!config || !m_managerApi) return;
		//--- just print
		m_managerApi->LoggerOut(MTLogOK, L"Symbol %s has been updated", config->Symbol());
	}

public:
	virtual void      OnOrderAdd(const IMTOrder*    /*order*/);// {  }
	virtual void      OnOrderUpdate(const IMTOrder* /*order*/);// {  }
	virtual void      OnOrderDelete(const IMTOrder* /*order*/);// {  }
	virtual void      OnOrderClean(const UINT64 /*login*/);// {  }
	virtual void      OnOrderSync(void) {  }
public:
	//--- tick events
	virtual void      OnTick(LPCWSTR /*symbol*/, const MTTickShort& /*tick*/);// { }
	virtual void      OnTickStat(const MTTickStat& /*tick*/);// { }
															 //--- tick hooks
	virtual MTAPIRES  HookTick(const int /*feeder*/, MTTick& /*tick*/) { return(MT_RET_OK); }
	virtual MTAPIRES  HookTickStat(const int /*feeder*/, MTTickStat& /*tstat*/) { return(MT_RET_OK); }
	//--- extended tick events
	virtual void      OnTick(const int /*feeder*/, const MTTick& /*tick*/) { }
	virtual void      OnTickStat(const int /*feeder*/, const MTTickStat& /*tstat*/) { }

private:
	CMTThread *		 m_real_tick_thread;
	CRedisPublisher * m_redis_publisher;
	boost::asio::io_service m_ioService;

	// 行情队列锁
	boost::mutex		*m_p_deque_lock;
	// 行情队列
	std::deque<std::string> *m_p_deque_list;
public:
	IMTAdminAPI * getAdminApi() { return m_adminApi; }
	IMTManagerAPI * getManagerApi() { return m_managerApi; }
	CRedisPublisher * getRedisPublisher() { return m_redis_publisher; }
	boost::asio::io_service * getIOServeice() { return &m_ioService; }

	// 设置行情队列及队列锁
	void setDequeParams(boost::mutex *p_deque_lock, std::deque<std::string> *p_deque_list) { m_p_deque_lock = p_deque_lock; m_p_deque_list = p_deque_list; }
	// 行情队列锁
	boost::mutex		*getDequeLock() { return m_p_deque_lock; }
	// 行情队列
	std::deque<std::string> *getDequeList() { return m_p_deque_list; }
};
//+------------------------------------------------------------------+

// 添加Json子项
__inline static std::string json_start()
{
	return "{";
}
__inline static void json_end(std::string & str)
{
	str.append("}");
}

__inline static void json_append(std::string & str, std::string key, bool val, bool bAddComma = true)
{
	str.append("\"" + key + "\":").append(val ? "true" : "false");
	if (bAddComma)
	{
		str.append(",");
	}
}
__inline static void json_append(std::string & str, std::string key, uint64_t val, bool bAddComma = true)
{
	str.append("\"" + key + "\":").append(std::to_string(val));
	if (bAddComma)
	{
		str.append(",");
	}
}
__inline static void json_append(std::string & str, std::string key, unsigned int val, bool bAddComma = true)
{
	str.append("\"" + key + "\":").append(std::to_string(val));
	if (bAddComma)
	{
		str.append(",");
	}
}
__inline static void json_append(std::string & str, std::string key, int val, bool bAddComma = true)
{
	str.append("\"" + key + "\":").append(std::to_string(val));
	if (bAddComma)
	{
		str.append(",");
	}
}
__inline static void json_append(std::string & str, std::string key, double val, bool bAddComma = true)
{
	str.append("\"" + key + "\":").append(std::to_string(val));
	if (bAddComma)
	{
		str.append(",");
	}
}
__inline static void json_append(std::string & str, std::string key, std::string val, bool bAddComma = true)
{
	str.append("\"" + key + "\":\"").append(val).append("\"");
	if (bAddComma)
	{
		str.append(",");
	}
}
//初始化{"result":true,"msg":"success","data":{}}
__inline static std::string json_data_make(bool result, std::string data, std::string msg = "")
{
	std::string str = json_start();
	json_append(str, "result", result);
	json_append(str, "msg", msg);
	json_append(str, "data", data);
	json_end(str);
	return str;
}
