#include <stdafx.h>
#include "MsgHook.h"
#include <Windows.h>

HMODULE g_hModule ;
HHOOK g_hook = NULL;

LRESULT CALLBACK HookProc(
	int code, // hook code
	WPARAM wParam, // removal option
	LPARAM lParam // message
	)
{
	if(g_hook)
	{
		return CallNextHookEx(g_hook,code,wParam,lParam);
	}
	return 1;
}

void SetMsgHookOn()
{
	OutputDebugString(_T("SetMsgHookOn enter--------"));
	g_hook = SetWindowsHookEx(WH_GETMESSAGE,HookProc,g_hModule,0);
	if(g_hook == NULL )
	{
		OutputDebugString(_T("SetWindowsHookEx false"));
	}
}

void SetMsgHookOff()
{
	OutputDebugString(_T("SetMsgHookOff enter++++++++"));
	BOOL ret = FALSE;
	if(g_hook)
	{
		ret = UnhookWindowsHookEx(g_hook);
	}
	if(ret == FALSE)
	{
		OutputDebugString(_T("UnhookWindowsHookEx false++++++++"));
	}
}

// 下的全局钩子被系统强势注入到进程中后通知你的目标程序
void NotifyYourApp(BOOL bTatch)
{
	HWND targetWnd = FindWindow(MAIN_APP_CLASS,MAIN_APP_TITLE);
	if(targetWnd == NULL)
	{
		OutputDebugString(_T("target exe not found"));
		// 目标程序都没启动，捕获没用处，取消全局钩子
		SetMsgHookOff();
		return;
	}

	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR exePath[MAX_PATH] = {0};
	int ret = GetModuleFileName(hModule,exePath,MAX_PATH);
	CString strPath = exePath;
	int lastFlag = strPath.ReverseFind('\\');
	CString exeName = strPath.Right(strPath.GetLength() - lastFlag - 1);
	if(exeName.CompareNoCase(_T("iexplore.exe")) == 0)
	{
		OutputDebugString(_T("windows hook proc inject iexplore.exe"));
		// 当前注入的进程为IE浏览器,通知目标程序
		HANDLE ieHandle = GetCurrentProcess();
		PostMessage(targetWnd,WM_IE_OPEN,bTatch,(LPARAM)ieHandle);
	}
}

// DLL 入口
BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	g_hModule = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			NotifyYourApp(TRUE);
		}
		break;
	case DLL_PROCESS_DETACH:
		{
			// 什么情况 钩子DLL会被卸载？程序退出？
			NotifyYourApp(FALSE);
		}
		break;
	}
	return TRUE;
}