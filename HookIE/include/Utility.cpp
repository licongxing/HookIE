#include "StdAfx.h"
#include "Utility.h"
#include <TlHelp32.h>
#include <string> // std::locale��Ҫ

// ��־��ӡ
#include "LogPolicy.h"
ConsolePolicy* g_pConsoleLog;
FilePolicy* g_pFileLog;
ThreadSafePolicy* g_pThreadSafeLog;

// GetModuleFileNameEx ��Ҫ���漸��
#ifndef PSAPI_VERSION
#define PSAPI_VERSION 1
#endif

#include <Psapi.h>  
#pragma comment (lib,"Psapi.lib")  

bool CUtility::m_bInitLog = false;

CUtility::CUtility(void)
{
}


CUtility::~CUtility(void)
{
}

void CUtility::InitLog(CString logFilePrefix,CString logFilePath)
{
	if(m_bInitLog == true)
		return;

	m_bInitLog = true;
	// ��־��ʼ��
	std::locale::global(std::locale(""));

	g_pFileLog = new FilePolicy(NULL);
	g_pThreadSafeLog = new ThreadSafePolicy(g_pFileLog);

	g_pFileLog->setprefix(logFilePrefix);
	g_pFileLog->setpath(logFilePath);
	g_pThreadSafeLog->setloglevel(LOGSYSTEM);

#ifdef TEXT_LOG
	g_pThreadSafeLog->setloglevel(LOGDEBUG);
#endif
}

void CUtility::TextLog(CString strKey,CString strValue)
{
	CString logInfo;
	CString strkey = L"[ ";
	strkey.Append(strKey);
	strkey.Append(L" ] ");
	logInfo.Append(strkey);
	logInfo.Append(strValue);
	g_pThreadSafeLog->Log(LOGINFO,logInfo);
}

BOOL CUtility::IsWindows64()
{
	typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)::GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
	BOOL bIsWow64 = FALSE;
	if (fnIsWow64Process)
		if (!fnIsWow64Process(::GetCurrentProcess(), &bIsWow64))
			bIsWow64 = FALSE;
	return bIsWow64;
}

CString CUtility::GetIEPath()
{
	TCHAR szPath[MAX_PATH];
	TCHAR *strLastSlash = NULL;
	GetSystemDirectoryW(szPath, sizeof(szPath) );
	szPath[MAX_PATH - 1] = 0;
	strLastSlash = wcschr( szPath, L'\\' );
	*strLastSlash = 0;
	if ( IsWindows64() )
	{
		wcscat_s( szPath,L"\\program files (x86)\\internet explorer\\iexplore.exe" );
	}
	else
	{
		wcscat_s( szPath,L"\\program files\\internet explorer\\iexplore.exe" );
	}
	return CString(szPath);
}

CString CUtility::GetModulePath(HMODULE hModule)
{
	TCHAR buf[MAX_PATH] = {'\0'};
	CString strDir, strTemp;

	::GetModuleFileName( hModule, buf, MAX_PATH);
	strTemp = buf;
	strDir = strTemp.Left( strTemp.ReverseFind('\\') + 1 );
	return strDir;
}

void CUtility::GetProcessHandle(CString strExePath,std::list<HANDLE>& handleList)
{
	CString exeName ;
	int index= strExePath.ReverseFind('\\');
	exeName = strExePath.Right(strExePath.GetLength()-index-1);

	HANDLE snapHandele = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,NULL);
	if( INVALID_HANDLE_VALUE == snapHandele)
	{
		return;
	}
	PROCESSENTRY32 entry = {0};
	entry.dwSize = sizeof(entry);// ���ȱ��븳ֵ
	BOOL bRet = Process32First(snapHandele,&entry);
	CString  exeTempName;
	while (bRet) 
	{
		exeTempName = (entry.szExeFile);
		if( exeTempName.CompareNoCase(exeName) ==0 )
		{
			HANDLE procHandle=OpenProcess(PROCESS_ALL_ACCESS,FALSE,entry.th32ProcessID);  
			TCHAR exePath[MAX_PATH] = {0};
			if(procHandle)
			{
				if( GetModuleFileNameEx(procHandle,NULL,exePath,MAX_PATH) )
				{
					// ȫ·����ȡ��
					if(CString(exePath).CompareNoCase(strExePath) == 0)
					{
						// ���̾���ҵ�
						handleList.push_back(procHandle);
					}
					else
					{
						CloseHandle(procHandle);
					}
				}
				else
				{
					CloseHandle(procHandle);
				}
				
			}
		}
		bRet = Process32Next(snapHandele,&entry);
	}
	CloseHandle(snapHandele);
	return;
}

void CUtility::InjectDllToExe(CString strDllPath,CString strExePath)
{
	std::list<HANDLE> handleList;
	GetProcessHandle(strExePath,handleList);
	HANDLE targetProc = NULL;

	// ��ȡ����ÿ��EXE���̾��������DLLע��
	for(std::list<HANDLE>::iterator it = handleList.begin(); it != handleList.end(); it++)
	{
		targetProc = *it;
		bool ret = InjectDllToProc(strDllPath, targetProc);
		CloseHandle(targetProc);
		if(ret == false)
		{
			//AfxGetApp()->GetMainWnd()->MessagekBox(_T("InjectDllToProc failed"));
#ifdef TEXT_LOG
			CString temp;
			temp.Format(_T("handle:%d false"),targetProc);
			CUtility::TextLog(_T("CUtility::InjectDllToExe"),temp);
#endif
		}
		
	}
	return;
}

bool CUtility::InjectDllToProc(CString strDllPath, HANDLE targetProc)
{
	if(targetProc == NULL)
	{
		return false;
	}
	/*
	ע��DLL��˼·���裺
	1. ��Ŀ�����������һ���ڴ�ռ�(ʹ��VirtualAllocEx����) ���DLL��·�����������ִ��LoadLibraryA
	2. ��DLL·��д�뵽Ŀ�����(ʹ��WriteProcessMemory����)
	3. ��ȡLoadLibraryA������ַ(ʹ��GetProcAddress)��������Ϊ�̵߳Ļص�����
	4. ��Ŀ����� �����̲߳�ִ��(ʹ��CreateRemoteThread)
	*/

	int dllLen = strDllPath.GetLength();
	// 1.Ŀ���������ռ�
	LPVOID pDLLPath = VirtualAllocEx(targetProc,NULL,dllLen,MEM_COMMIT,PAGE_READWRITE );
	if( pDLLPath == NULL )
	{
		return false;
	}
	SIZE_T wLen = 0;
	// 2.��DLL·��д��Ŀ������ڴ�ռ�
	int ret = WriteProcessMemory(targetProc,pDLLPath,strDllPath,dllLen,&wLen);
	if( ret == 0 )
	{
		return false;
	}
	// 3.��ȡLoadLibraryA������ַ
	FARPROC myLoadLibrary = GetProcAddress(GetModuleHandleA("kernel32.dll"),"LoadLibraryA");
	if( myLoadLibrary == NULL )
	{
		return false;
	}
	// 4.��Ŀ�����ִ��LoadLibrary ע��ָ�����߳�
	HANDLE tHandle = CreateRemoteThread(targetProc,NULL,NULL,
		(LPTHREAD_START_ROUTINE)myLoadLibrary,pDLLPath,NULL,NULL);
	if(tHandle == NULL)
	{
		return false;
	}
	WaitForSingleObject(tHandle,INFINITE);
	CloseHandle(tHandle);
	return true;
}

void CUtility::UninstallDllToExe(CString strDllPath,CString strExePath)
{
	std::list<HANDLE> handleList;
	GetProcessHandle(strExePath,handleList);
	HANDLE targetProc = NULL;

	// ��ȡ����ÿ��EXE���̾��������DLLж��
	for(std::list<HANDLE>::iterator it = handleList.begin(); it != handleList.end(); it++)
	{
		targetProc = *it;
		bool ret = UninstallDllToProc(strDllPath, targetProc);
		CloseHandle(targetProc);
		if(ret == false)
		{
			//AfxGetApp()->GetMainWnd()->MessageBox(_T("UninstallDllToProc failed"));
#ifdef TEXT_LOG
			CString temp;
			temp.Format(_T("handle:%d false"),targetProc);
			CUtility::TextLog(_T("CUtility::UninstallDllToExe"),temp);
#endif
			
		}
		
	}
	return;
}

bool CUtility::UninstallDllToProc(CString strDllPath, HANDLE targetProc)
{
    /*
    ж�ز����ע��DLL����ʵ�ʲ��.
    ע��DLL�� ��Ŀ�������ִ��LoadLibraryA
    ж��DLL�� ��Ŀ�������ִ��FreeLibrary��������ͬ����ж�ز���Ҫ��Ŀ�����������ռ䣬
    ��ΪFreeLibrary����ΪHMODULE ʵ���Ͼ���һ��ָ��ֵ���������Ѿ����ؾ��Ѿ����ڡ�
    */
	
    if( targetProc == NULL )
    {
        return false;
    }
	DWORD processID = GetProcessId(targetProc);

    // 1. ��ȡж��dll��ģ����
    HANDLE snapHandele = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE ,processID);
    if( INVALID_HANDLE_VALUE == snapHandele)
    {
        return false;
    }
    MODULEENTRY32 entry = {0};
    entry.dwSize = sizeof(entry);// ���ȱ��븳ֵ
    BOOL ret = Module32First(snapHandele,&entry);
    HMODULE dllHandle = NULL;
	CString tempDllPath;
    while (ret) {
        tempDllPath = entry.szModule;
        if(tempDllPath.CompareNoCase((strDllPath)))
        {
            dllHandle = entry.hModule;
            break;
        }
        ret = Module32Next(snapHandele,&entry);
    }

    CloseHandle(snapHandele);
    if( dllHandle == NULL )
    {
        return false;
    }

    // 2.��ȡFreeLibrary������ַ
    FARPROC myLoadLibrary = GetProcAddress(GetModuleHandleA("kernel32.dll"),"FreeLibrary");
    if( myLoadLibrary == NULL )
    {
        return false;
    }
    // 3.��Ŀ�����ִ��FreeLibrary ж��ָ�����߳�
    HANDLE tHandle = CreateRemoteThread(targetProc,NULL,NULL,
                       (LPTHREAD_START_ROUTINE)myLoadLibrary,dllHandle,NULL,NULL);
    if(tHandle == NULL)
    {
        return false;
    }
    WaitForSingleObject(tHandle,INFINITE);
    CloseHandle(tHandle);
	return true;
}
