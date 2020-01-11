#include "stdafx.h"
#include <Windows.h>
#include <Wininet.h>
#pragma comment(lib,"wininet.lib")

#include "InlineHook7.h"

CInlineHook7 g_inlineHookObj7;

typedef HINTERNET ( WINAPI *InternetConnectWFunc)(
	HINTERNET     hInternet,
	LPCWSTR       lpszServerName,
	INTERNET_PORT nServerPort,
	LPCWSTR       lpszUserName,
	LPCWSTR       lpszPassword,
	DWORD         dwService,
	DWORD         dwFlags,
	DWORD_PTR     dwContext
	);

HINTERNET WINAPI MyInternetConnectW(
	HINTERNET     hInternet,
	LPCWSTR       lpszServerName,
	INTERNET_PORT nServerPort,
	LPCWSTR       lpszUserName,
	LPCWSTR       lpszPassword,
	DWORD         dwService,
	DWORD         dwFlags,
	DWORD_PTR     dwContext
	)
{
	g_inlineHookObj7.UnHook(); // 必须先卸载钩子 再才可以再次调用被Hook的函数，不然会进入死循环
	HINTERNET ret = NULL;
	CString temp;
	temp.Format(_T("拦截到url:%s"),lpszServerName);
	CString url = lpszServerName;
	if(url.Find(_T("baidu.com")) >= 0)
	{
		url = _T("www.sogou.com");
	}

	OutputDebugString(temp);
	// 这里要用已经存在的库，不要再在自己的库中链接WININET.dll库去使用
	//ret = ::InternetConnectW(hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword,  dwService,  dwFlags,  dwContext);

	HMODULE hModule = GetModuleHandle(_T("WININET.dll"));
	if(hModule == NULL)
	{
		OutputDebugString(_T("GetModuleHandle false"));
		goto end;
	}
	InternetConnectWFunc func = (InternetConnectWFunc)GetProcAddress(hModule,"InternetConnectW");
	if(func == NULL)
	{
		OutputDebugString(_T("GetProcAddress false"));
		goto end;
	}
	ret = func(hInternet, url, nServerPort, lpszUserName, lpszPassword,  dwService,  dwFlags,  dwContext);
	
end:
	g_inlineHookObj7.ReHook();
	return ret;
}


VOID InlineHookIE()
{
	// 测试消息发送
	//HWND targetWnd = FindWindow(_T("#32770"),_T("HookIE"));
	//if(targetWnd)
	//{

	//	BOOL ret = PostMessage(targetWnd,WM_USER + 99,TRUE,NULL);
	//	if(ret == FALSE)
	//	{
	//		DWORD errCode = GetLastError();
	//		CString errMsg = CUtility::GetErrorMsg(errCode);
	//		CString temp;
	//		temp.Format(_T("PostMessage failed!!!errCode:%d,errMsg:%s"),errCode,errMsg);
	//		OutputDebugString(temp);
	//	}
	//}
	

	OutputDebugString(_T("InlineHookIE into"));
	g_inlineHookObj7.Hook("WININET.dll","InternetConnectW",(FARPROC)MyInternetConnectW);
	return;
}

VOID UnInlineHookIE()
{
	OutputDebugString(_T("UnInlineHookIE into"));
	g_inlineHookObj7.UnHook();
	return;
}

// DLL 入口
BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			InlineHookIE();
		}
		break;
	case DLL_PROCESS_DETACH:
		UnInlineHookIE();
		break;
	}
	return TRUE;
}

