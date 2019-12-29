#pragma once

#include <Windows.h>

class CInlineHook7
{
public:
	CInlineHook7(void);
	~CInlineHook7(void);

	bool Hook(LPSTR strModuleName,LPSTR strHookFnName,FARPROC strHookCallFnName);

	bool UnHook();
	bool ReHook();
private:
	
	FARPROC m_pFnOrign; // 要Hook的函数地址
	BYTE m_bOld[7]; // 要Hook的函数 前7个字节
	BYTE m_bNew[7]; // 要Hook的函数 修改后的7个字节
};

