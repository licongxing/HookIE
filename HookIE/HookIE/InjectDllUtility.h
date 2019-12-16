#pragma once
#include <list>

class CInjectDllUtility
{
public:
	CInjectDllUtility(void);
	~CInjectDllUtility(void);

	// 判断是否为64位操作系统
	static BOOL IsWindows64();

	static CString GetIEPath();

	// 获取进程句柄所在的路径
	static CString GetModulePath(HMODULE hModule = NULL);

	// 获取指定EXE路径的进程句柄
	static void GetProcessHandle(CString strExePath,std::list<HANDLE> &handleList);
	
	// 指定DLL注入到指定EXE进程
	static void InjectDllToExe(CString strDllPath,CString strExePath);

	// 指定DLL注入到指定进程句柄
	static bool InjectDllToProc(CString strDllPath, HANDLE targetProc);

	// 指定DLL从指定EXE进程卸载
	static void UninstallDllToExe(CString strDllPath,CString strExePath);

	// 指定DLL从指定进程句柄卸载
	static bool  UninstallDllToProc(CString strDllPath, HANDLE targetProc);

};

