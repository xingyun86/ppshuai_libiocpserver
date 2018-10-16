// SampleTextServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "MyIOCP.h"
#include "MacroDefine.h"
//#include "DealerSink.h"

void ConvertTimeZone()
{
	INT64 nStartTime = 1523451135;
	SYSTEMTIME stRemote = { 0 };
	SYSTEMTIME stLocal = { 0 };
	SMTTime::TimeToST(nStartTime, stRemote);
	nStartTime = SMTTime::STToTime(stRemote);
	TIME_ZONE_INFORMATION tzi = { 0 };
	GetTimeZoneInformation(&tzi);
	SystemTimeToTzSpecificLocalTime(&tzi, &stRemote, &stLocal);
	TzSpecificLocalTimeToSystemTime(&tzi, &stLocal, &stRemote);
}
// UNIX时间转换为本地时间
INT64 RemoteToLocale(INT64 nRemoteTime)
{
	TIME_ZONE_INFORMATION tzi = { 0 };
	GetTimeZoneInformation(&tzi);
	return nRemoteTime - tzi.Bias * MINUTES_IN_HOUR;
}
// 本地时间转换为UNIX时间
INT64 LocaleToRemote(INT64 nLocaleTime)
{
	TIME_ZONE_INFORMATION tzi = { 0 };
	GetTimeZoneInformation(&tzi);
	return nLocaleTime + tzi.Bias * MINUTES_IN_HOUR;
}
// 转换ChartBar时间为本地时间
void MTChartBarToLocale(MTChartBar * pMTChartBar, INT nMTChartBarNumber)
{
	for (INT nIndex = 0; nIndex < nMTChartBarNumber; nIndex++)
	{
		pMTChartBar[nIndex].datetime = RemoteToLocale(pMTChartBar[nIndex].datetime);
	}
}
//
INT64 MakeLocalTime(WORD wYear, WORD wMonth, WORD wDay, WORD wHour = 0, WORD wMinute = 0, WORD wSecond = 0)
{
	SYSTEMTIME stLocale = { 0 };
	TIME_ZONE_INFORMATION tzi = { 0 };
	stLocale.wYear = wYear;
	stLocale.wMonth = wMonth;
	stLocale.wDay = wDay;
	stLocale.wHour = wHour;
	stLocale.wMinute = wMinute;
	stLocale.wSecond = wSecond;
	return SMTTime::STToTime(stLocale);
}
void ConvertToPeriodData(MTChartBar * pMTChartBar, INT nMTChartBarNumber, UINT64 nStart, UINT64 nEnd, int nPeriod)
{
	switch (nPeriod)
	{
	case 1://1分钟
	case 5://5分钟
	case 15://15分钟
	case 30://30分钟
	case 60://1小时
	case 240://4小时
	case 1440://一天
	case 10080://一周
	case 30 * 1440://一月
	case 12 * 30 * 1440://一年
	{

	}
	break;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	// 行情队列锁
	boost::mutex deque_lock;
	// 行情队列
	std::deque<std::string> deque_list;

	WCHAR * wServer = L"47.52.199.85:443";

	//1001为Administrators,下单权限
	UINT64 uLoginId = 1000;
	WCHAR * wPassword = L"dsowdsfwe12";
	//1001为Administrators,没有下单权限
	//UINT64 uLoginId = 1001;
	//WCHAR * wPassword = L"ylipfh3s";

	CMTTickInstance tickInstance;
	
	tickInstance.setDequeParams(&deque_lock, &deque_list);

	//初始化建立MT5服务器连接会话
	if (tickInstance.Start(wServer, uLoginId, wPassword) != MT_RET_OK)
	{
		std::cout << "InitConn to mt5 server failed!" << std::endl;
		return 0;
	}

	{
		MTAPIRES	mtapires;
		int	n;
		MTChartBar	*bars = new MTChartBar;
		UINT	bar_total;
		LPWSTR lpSymbols[] = {
			L"XAUUSD",
		};

		for (int i = 0; i < sizeof(lpSymbols) / sizeof(*lpSymbols); i++)
		{
			INT64 nStartTime = LocaleToRemote(MakeLocalTime(2018, 8, 1));
			INT64 nEndTime = LocaleToRemote(MakeLocalTime(2018, 10, 27));
			INT64 nLocal = RemoteToLocale(1533052800);
			mtapires = tickInstance.getAdminApi()->ChartRequest(lpSymbols[i], nStartTime, nEndTime, bars, bar_total);
			n = bar_total;
			if (mtapires == MT_RET_OK)
			{
				FILE * pFile = fopen("2018-8-1_10-27.data", "wb");
				if (pFile)
				{
					fwrite(bars, bar_total * sizeof(*bars), 1, pFile);
					fclose(pFile);
				}
				MTChartBarToLocale(bars, bar_total);
				for (int j = 0; j < n; j++)
				{
					wprintf_s(L"Symbol charts: %s open %f, high %f, low %f, close %f, datetime %u, bars_total %u \n", lpSymbols[i], bars[j].open, bars[j].high, bars[j].low, bars[j].close, bars[j].datetime, bar_total);
				}
			}
			else
			{
				wprintf_s(L"Error requesting chart (%d)\n", mtapires);
			}
		}
	}
	std::map<std::string, std::string> odata = { { "account","" },{ "method","" },{ "symbol","" },{ "price","" },{ "volume","" }, };
	std::string str = "{\"account\":\"sdf\",\"method\":\"0\",\"symbol\":\"sadf\",\"price\":\"dsf\",\"volume\":\"0.1\"}";
	ParseJsonData(odata, str);
	double dVol = std::stod(odata.at("volume"));
	std::cout << "====Server started====" << std::endl;


	MyIOCP myiocp(&deque_lock, &deque_list);
	myiocp.setTickInstance(&tickInstance);
	// 启动服务
	myiocp.Startup(20000,4,10000);

	while (true)
	{
		::Sleep(1000);
	}
	//断开MT5服务器连接
	tickInstance.Stop();

	getchar();
	
	myiocp.Shutdown();

	return 0;
}

