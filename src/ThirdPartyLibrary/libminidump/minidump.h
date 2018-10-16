#include <MacroDefine.h>
#include <dbghelp.h>
#include <tchar.h>

// based on dbghelp.h
typedef BOOL (WINAPI *PFN_MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

class MiniDumper
{
private:
	static LPCTSTR m_szAppName;

	static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
	{
		LONG retval = EXCEPTION_CONTINUE_SEARCH;
		HWND hParent = NULL;						// find a better value for your app

													// firstly see if dbghelp.dll is around and has the function we need
													// look next to the EXE first, as the one in System32 might be old 
													// (e.g. Windows 2000)
		HMODULE hDll = NULL;
		_TCHAR szDbgHelpPath[_MAX_PATH];

		if (GetModuleFileName(NULL, szDbgHelpPath, _MAX_PATH))
		{
			_TCHAR *pSlash = _tcsrchr(szDbgHelpPath, _T('\\'));
			if (pSlash)
			{
				_tcscpy(pSlash + 1, _T("DBGHELP.DLL"));
				hDll = ::LoadLibrary(szDbgHelpPath);
			}
		}

		if (hDll == NULL)
		{
			// load any version we can
			hDll = ::LoadLibrary(_T("DBGHELP.DLL"));
		}

		LPCTSTR szResult = NULL;

		if (hDll)
		{
			PFN_MINIDUMPWRITEDUMP pDump = (PFN_MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
			if (pDump)
			{
				_TCHAR tzModPath[MAX_PATH] = { 0 };
				_TCHAR szDumpPath[_MAX_PATH] = { 0 };
				_TCHAR szScratch[_MAX_PATH] = { 0 };
				SYSTEMTIME systemtime = { 0 };
				::GetModuleFileName(GetModuleHandle(NULL), tzModPath, sizeof(tzModPath) / sizeof(*tzModPath));
				*(_tcsrchr(tzModPath, _T('\\')) + sizeof(char)) = _T('\0');
				if (!*tzModPath)
				{
					// work out a good place for the dump file
					if (!::GetTempPath(_MAX_PATH, szDumpPath))
					{
						_tcscpy(szDumpPath, _T(".\\"));
					}
				}

				::GetLocalTime(&systemtime);
				memset(szDumpPath, 0, sizeof(szDumpPath));
				_stprintf(szDumpPath,
					_T("%s%s-%04d%02d%02d%02d%02d%02d.dmp"),
					tzModPath, m_szAppName,
					systemtime.wYear, systemtime.wMonth, systemtime.wDay,
					systemtime.wHour, systemtime.wMinute, systemtime.wSecond);
				//_tcscat(szDumpPath, m_szAppName);
				//_tcscat(szDumpPath, _T(".dmp"));

				// ask the user if they want to save a dump file
				//if (::MessageBox( NULL, _T("Something bad happened in your program, would you like to save a diagnostic file?"), m_szAppName, MB_YESNO )==IDYES)
				{
					// create the file
					HANDLE hFile = ::CreateFile(szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL, NULL);

					if (hFile != INVALID_HANDLE_VALUE)
					{
						_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

						ExInfo.ThreadId = ::GetCurrentThreadId();
						ExInfo.ExceptionPointers = pExceptionInfo;
						ExInfo.ClientPointers = NULL;

						// write the dump
						BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
						if (bOK)
						{
							_stprintf(szScratch, _T("Saved dump file to '%s'"), szDumpPath);
							szResult = szScratch;
							retval = EXCEPTION_EXECUTE_HANDLER;
						}
						else
						{
							_stprintf(szScratch, _T("Failed to save dump file to '%s' (error %d)"), szDumpPath, GetLastError());
							szResult = szScratch;
						}
						::CloseHandle(hFile);
					}
					else
					{
						_stprintf(szScratch, _T("Failed to create dump file '%s' (error %d)"), szDumpPath, GetLastError());
						szResult = szScratch;
					}
				}
			}
			else
			{
				szResult = _T("DBGHELP.DLL too old");
			}
		}
		else
		{
			szResult = _T("DBGHELP.DLL not found");
		}

		//if (szResult)
		//{
		//	::MessageBox(NULL, szResult, m_szAppName, MB_OK);
		//}
		return retval;
	}

public:
	MiniDumper(LPCTSTR szAppName)
	{
		// if this assert fires then you have two instances of MiniDumper
		// which is not allowed
		assert(m_szAppName == NULL);

		m_szAppName = szAppName ? _tcsdup(szAppName) : _T("Application");

		::SetUnhandledExceptionFilter(TopLevelFilter);
	}
};
LPCTSTR MiniDumper::m_szAppName;
