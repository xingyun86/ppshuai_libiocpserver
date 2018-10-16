// SampleTextServer.cpp : �������̨Ӧ�ó������ڵ㡣
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
// UNIXʱ��ת��Ϊ����ʱ��
INT64 RemoteToLocale(INT64 nRemoteTime)
{
	TIME_ZONE_INFORMATION tzi = { 0 };
	GetTimeZoneInformation(&tzi);
	return nRemoteTime - tzi.Bias * MINUTES_IN_HOUR;
}
// ����ʱ��ת��ΪUNIXʱ��
INT64 LocaleToRemote(INT64 nLocaleTime)
{
	TIME_ZONE_INFORMATION tzi = { 0 };
	GetTimeZoneInformation(&tzi);
	return nLocaleTime + tzi.Bias * MINUTES_IN_HOUR;
}
// ת��ChartBarʱ��Ϊ����ʱ��
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
	case 1://1����
	case 5://5����
	case 15://15����
	case 30://30����
	case 60://1Сʱ
	case 240://4Сʱ
	case 1440://һ��
	case 10080://һ��
	case 30 * 1440://һ��
	case 12 * 30 * 1440://һ��
	{

	}
	break;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ���������
	boost::mutex deque_lock;
	// �������
	std::deque<std::string> deque_list;

	WCHAR * wServer = L"47.52.199.85:443";

	//1001ΪAdministrators,�µ�Ȩ��
	UINT64 uLoginId = 1000;
	WCHAR * wPassword = L"dsowdsfwe12";
	//1001ΪAdministrators,û���µ�Ȩ��
	//UINT64 uLoginId = 1001;
	//WCHAR * wPassword = L"ylipfh3s";

	CMTTickInstance tickInstance;
	
	tickInstance.setDequeParams(&deque_lock, &deque_list);

	//��ʼ������MT5���������ӻỰ
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
	// ��������
	myiocp.Startup(20000,4,10000);

	while (true)
	{
		::Sleep(1000);
	}
	//�Ͽ�MT5����������
	tickInstance.Stop();

	getchar();
	
	myiocp.Shutdown();

	return 0;
}

