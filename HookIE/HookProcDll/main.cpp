#include "stdafx.h"
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
	OutputDebugString(_T("===== SetMsgHookOn enter===== \n"));
	g_hook = SetWindowsHookEx(WH_GETMESSAGE,HookProc,g_hModule,0);
	if(g_hook == NULL )
	{
		DWORD errCode = GetLastError();
		CString errMsg = CUtility::GetErrorMsg(errCode);
		CString temp;
		temp.Format(_T("SetWindowsHookEx false!!!errCode:%d,errMsg:%s\n"),errCode,errMsg);
		OutputDebugString(temp);
	}
}



void SetMsgHookOff()
{
	OutputDebugString(_T("===== SetMsgHookOff enter =====\n"));
	CString temp;
	temp.Format(_T("Cur Exe:%s\n"),CUtility::GetCurExeName());
	OutputDebugString(temp);
	BOOL ret = FALSE;
	if(g_hook)
	{
		ret = UnhookWindowsHookEx(g_hook);
	}
	if(ret == FALSE)
	{
		DWORD errCode = GetLastError();
		CString errMsg = CUtility::GetErrorMsg(errCode);
		CString temp;
		temp.Format(_T("UnhookWindowsHookEx false!!!errCode:%d,errMsg:%s\n"),errCode,errMsg);
		OutputDebugString(temp);
	}
}

// 下的全局钩子被系统强势注入到进程中后通知你的目标程序
void NotifyYourApp(BOOL bTatch)
{
	HWND targetWnd = FindWindow(_T("#32770"),MAIN_APP_TITLE);
	//HWND targetWnd = FindWindow(_T("Notepad"),NULL);
	if(targetWnd == NULL)
	{
		OutputDebugString(_T("target exe not found"));
		// 目标程序都没启动，捕获没用处，取消全局钩子
		SetMsgHookOff();
		return;
	}

	CString exeName = CUtility::GetCurExeName();
	if(exeName.CompareNoCase(_T("iexplore.exe")) == 0)
	{
		if(bTatch)
		{
			OutputDebugString(_T("HookProcDll.dll Attach iexplore.exe"));
		}
		else
		{
			OutputDebugString(_T("HookProcDll.dll Detach iexplore.exe"));
		}
		
		// 当前注入的进程为IE浏览器,通知目标程序,将进程ID发给目标程序
		DWORD processId = GetProcessId(GetCurrentProcess());
		BOOL ret = PostMessage(targetWnd,WM_IE_OPEN,bTatch,(LPARAM)processId);
		if(ret == FALSE)
		{
			DWORD errCode = GetLastError();
			CString errMsg = CUtility::GetErrorMsg(errCode);
			CString temp;
			temp.Format(_T("PostMessage failed!!!errCode:%d,errMsg:%s"),errCode,errMsg);
			OutputDebugString(temp);
		}
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
			// 什么情况 钩子DLL会被卸载？当Windows全局钩子被卸载后 系统会卸载该DLL，程序主动退出也会卸载该DLL
			NotifyYourApp(FALSE);
		}
		break;
	}
	return TRUE;
}