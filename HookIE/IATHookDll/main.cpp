// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <windows.h>
#include <Wininet.h>

//定义函数指针，和CreateFileW一致
typedef HINTERNET (* WINAPI InternetOpenUrlFunc)(
	_In_ HINTERNET hInternet,
	_In_ LPCTSTR   lpszUrl,
	_In_ LPCTSTR   lpszHeaders,
	_In_ DWORD     dwHeadersLength,
	_In_ DWORD     dwFlags,
	_In_ DWORD_PTR dwContext
	);


InternetOpenUrlFunc dwInternetOpenUrlAddr = 0;
DWORD* pInternetOpenUrlAddrOrgin = 0;

//HINTERNET InternetOpenUrl(
//	_In_ HINTERNET hInternet,
//	_In_ LPCTSTR   lpszUrl,
//	_In_ LPCTSTR   lpszHeaders,
//	_In_ DWORD     dwHeadersLength,
//	_In_ DWORD     dwFlags,
//	_In_ DWORD_PTR dwContext
//	);



//fake func replace the one above
HANDLE WINAPI myInternetOpenUrl(
		_In_ HINTERNET hInternet,
		_In_ LPCTSTR   lpszUrl,
		_In_ LPCTSTR   lpszHeaders,
		_In_ DWORD     dwHeadersLength,
		_In_ DWORD     dwFlags,
		_In_ DWORD_PTR dwContext
	)
{
	CString str;
	CString cstrUrl = lpszUrl;
	if (cstrUrl.Find(_T("baidu.com")) >= 0)
	{
		if (MessageBoxW(NULL,L"访问百度?",L"NOTICE",MB_YESNO)==IDYES)
		{
			return   dwInternetOpenUrlAddr(
				hInternet,
				lpszUrl,
				lpszHeaders,
				dwHeadersLength,
				dwFlags,
				dwContext);
		} 
		else
		{
			return INVALID_HANDLE_VALUE;
		}
	} 
	else
	{
		return dwInternetOpenUrlAddr(
			hInternet,
			lpszUrl,
			lpszHeaders,
			dwHeadersLength,
			dwFlags,
			dwContext);
	}
}

VOID HookIEIAT()
{
	// 进程基址
	HMODULE hModule = GetModuleHandleA(NULL); // 当前EXE句柄
	
	// 定位PE结构
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule; // DOS头
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule+pDosHeader->e_lfanew); // PE头起始位置

	// 保存映像基址和导入表RVA
	DWORD dwImageBase = pNTHeader->OptionalHeader.ImageBase;
	DWORD dwImpRva = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	// 导入表的VA
	PIMAGE_IMPORT_DESCRIPTOR pImgDes = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)dwImageBase+dwImpRva);
	PIMAGE_IMPORT_DESCRIPTOR pTempImgDes = pImgDes;

	// 进程中InternetOpenUrlW的地址
	HMODULE handle=LoadLibraryA("WININET.dll");
	DWORD dwFuncAddr = (DWORD)GetProcAddress(handle,"InternetOpenUrlW");

	BOOL bFound = FALSE;
	// 查找欲hook函数的模块名
	while (pTempImgDes->Name) // 结构体全0为结束标志
	{
		DWORD dwNameAddr = dwImageBase + pTempImgDes->Name;
		char szName[MAXBYTE]={0};
		strcpy(szName,(char*)dwNameAddr);
		CString cstrName = szName;
		if (cstrName.CompareNoCase(_T("WININET.dll")) == 0 )
		{
			bFound = TRUE;
			break;
		}
		pTempImgDes ++; // 继续下一个导入表，一个导入表结构和一个DLL对应
	}

	// 判断是否找到欲hook函数所在的函数名
	if (bFound==TRUE)
	{
		bFound= FALSE;
		char szAddr[10] = {0};
		// 逐个遍历该模块的IAT导入地址表
		PIMAGE_THUNK_DATA pThunk=(PIMAGE_THUNK_DATA)(pTempImgDes->FirstThunk + dwImageBase); 
		while ( pThunk->u1.Function ) // 结构体全0为结束标志
		{
			// 遍历出的函数地址
			DWORD* pAddr=(DWORD*)(&pThunk->u1.Function); 
			// 比较是否与欲hook函数的地址相同
			if (*pAddr == dwFuncAddr)         
			{
				bFound = TRUE;         
				pInternetOpenUrlAddrOrgin = pAddr;
				dwInternetOpenUrlAddr = (InternetOpenUrlFunc)*pAddr;    
				DWORD dwMyHookAddr = (DWORD) myInternetOpenUrl;
				// 修改此处地址为hook函数地址
				WriteProcessMemory(GetCurrentProcess(),(LPVOID)pAddr,&dwMyHookAddr,sizeof(DWORD),NULL); 
				break;
			}
			pThunk ++; // 继续下一个导入地址表结构体，一个函数和一个导入地址表结构体对应
		}
	}

	return;
}

VOID UnHookIEIAT()
{
	if(dwInternetOpenUrlAddr)
	{
		// 如果被hook了，被hook地方还原为原来地址
		WriteProcessMemory(GetCurrentProcess(),(LPVOID)pInternetOpenUrlAddrOrgin,&dwInternetOpenUrlAddr,sizeof(DWORD),NULL); 
	}
}
BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		HookIEIAT();
		break;
	case DLL_PROCESS_DETACH:
		UnHookIEIAT();
		break;
	}
	return TRUE;
}

