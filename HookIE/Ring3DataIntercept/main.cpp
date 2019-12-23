#include "stdafx.h"
#include "windows.h"
#include "winnt.h"

PVOID pNtDeviceIoControl  = NULL ; 



//InternetOpenUrlFunc dwInternetOpenUrlAddr = 0;
DWORD* pNtDeviceIoControlAddrOrgin = 0;


//

#define AFD_RECV 0x12017

#define AFD_SEND 0x1201f

typedef struct AFD_WSABUF{
	UINT  len ;
	PCHAR  buf ;
}AFD_WSABUF , *PAFD_WSABUF;

typedef struct AFD_INFO {
	PAFD_WSABUF  BufferArray ; 
	ULONG  BufferCount ; 
	ULONG  AfdFlags ;
	ULONG  TdiFlags ;
} AFD_INFO,  *PAFD_INFO;
typedef LONG NTSTATUS;

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

const CHAR GetXX[] = "GET ";
const CHAR PostXX[] = "POST ";
const CHAR HttpXX[] = "HTTP";
//////////////////////////////////////////////////////////////////////////
//
// LookupSendPacket
// 检查Send包
// 目前实现了过滤HTTP请求（GET AND POST）
//
//////////////////////////////////////////////////////////////////////////

BOOL LookupSendPacket(PVOID Buffer , ULONG Len)
{
	if (Len < 5)
	{
		return FALSE ; 
	}

	//外层已有异常捕获
	if (memcmp(Buffer , GetXX , 4) == 0 
		||
		memcmp(Buffer , PostXX , 5) == 0 )
	{
		return TRUE ; 
	}
	return FALSE ; 
}        
//////////////////////////////////////////////////////////////////////////
//
// LookupRecvPacket
//
// 检查Recv包
// 在这里可以实现Recv包查字典功能
// 目前实现了过滤HTTP返回数据包的功能
//
//
///////////////////////////////////////////////////////////////////////////
BOOL LookupRecvPacket(PVOID Buffer , ULONG Len)
{
	if (Len < 4)
	{
		return FALSE ; 
	}

	if (memcmp(Buffer , HttpXX , 4) == 0 )
	{
		return TRUE ; 
	}

	return FALSE ; 
}
//hook函数

//////////////////////////////////////////////////////////////////////////
//
// NtDeviceIoControlFile的HOOK函数 
// ws2_32.dll的send , recv最终会调用到mswsock.dll内的数据发送函数
// mswsock.dll会调用NtDeviceIoControlFile向TDI Client驱动发送Send Recv指令
// 我们在这里做拦截，可以过滤所有的TCP 收发包（UDP之类亦可，不过要更改指令）
//
//////////////////////////////////////////////////////////////////////////

NTSTATUS __stdcall NewNtDeviceIoControlFile(
	HANDLE FileHandle,
	HANDLE Event OPTIONAL,
	PVOID ApcRoutine OPTIONAL,
	PVOID ApcContext OPTIONAL,
	PVOID IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer OPTIONAL,
	ULONG InputBufferLength,
	PVOID OutputBuffer OPTIONAL,
	ULONG OutputBufferLength
	)
{
	//OutputDebugString(L"NewNtDeviceIoControlFile into \n");
	//先调用原始函数

	LONG stat ; 
	__asm
	{
		    push        OutputBufferLength
			push        OutputBuffer
			push        InputBufferLength
			push        InputBuffer 
			push        IoControlCode
			push        IoStatusBlock 
			push        ApcContext
			push        ApcRoutine
			push        Event
			push        FileHandle
			call        pNtDeviceIoControl
			mov                stat ,eax
	}

	//如果原始函数失败了（例如RECV无数据）

	if (!NT_SUCCESS(stat))
	{
		return stat ; 
	}
	//OutputDebugString(L"NewNtDeviceIoControlFile into aaaaaa\n");
	//检查是否为TCP收发指令

	if (IoControlCode != AFD_SEND && IoControlCode != AFD_RECV)
	{
		return stat ; 
	}
	//OutputDebugString(L"NewNtDeviceIoControlFile into bbbbbb\n");
	//访问AFD INFO结构，获得SEND或RECV的BUFFER信息
	//这里可能是有问题的BUFFER，因此我们要加TRY EXCEPT
	//

	__try
	{
		//从InputBuffer得到Buffer和Len

		PAFD_INFO AfdInfo = (PAFD_INFO)InputBuffer ; 
		PVOID Buffer = AfdInfo->BufferArray->buf ; 
		ULONG Len = AfdInfo->BufferArray->len;
		//char temp[11] = {0};
		//memcpy_s(temp,10,Buffer,10);
		//char temp2[100] = {0};
		//sprintf(temp2,"%x.%x.%x.%x.%x",temp[0],temp[1],temp[2],temp[3],temp[4]);
		//OutputDebugStringA(temp2);
		if (IoControlCode == AFD_SEND)
		{
			//OutputDebugString(L"AFD_SEND-----!\n");
			
			if (LookupSendPacket(Buffer , Len))
			{
				//输出包内容
				//这里输出调试信息，可以用DbgView查看，如果有UI可以做成SendMessage形式~
				OutputDebugString(L"SendPacket!\n");                
				OutputDebugStringA((char*)Buffer);
			}
		}
		else
		{
			//OutputDebugString(L"AFD_RECV-----!\n");
			if (LookupRecvPacket(Buffer , Len))
			{
				OutputDebugString(L"RecvPacket!\n");
				OutputDebugStringA((char*)Buffer);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		OutputDebugString(L"EXCEPTION_EXECUTE_HANDLER -----!\n");
		return stat ; 
	}

	return stat ; 



}

//////////////////////////////////////////////////////////////////////////
//
//  Hook mswsock.dll导出表的Ntdll!NtDeviceIoControlFile
//  并过滤其对TDI Cilent的请求来过滤封包
//  稳定，隐蔽，RING3下最底层的包过滤~
//
//////////////////////////////////////////////////////////////////////////
void SuperHookDeviceIoControl()
{
	//得到ws2_32.dll的模块基址
	HMODULE hMod = LoadLibrary(L"mswsock.dll");
	if (hMod == 0 )
	{
		OutputDebugString(L"LoadLibrary false !!! \n");
		return ;
	}

	//得到DOS头

	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hMod ; 

	//如果DOS头无效
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		OutputDebugString(L"PIMAGE_DOS_HEADER invalid !!! \n");
		return ; 
	}

	//得到NT头

	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((ULONG)hMod + pDosHeader->e_lfanew);

	//如果NT头无效
	if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		OutputDebugString(L"PIMAGE_NT_HEADERS invalid !!! \n");
		return ; 
	}

	//检查输入表数据目录是否存在
	if (pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress == 0 ||
		pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size == 0 )
	{
		OutputDebugString(L"IMAGE_DIRECTORY_ENTRY_IMPORT invalid !!! \n");
		return ; 
	}
	//得到输入表描述指针

	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG)hMod + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	PIMAGE_THUNK_DATA ThunkData ; 

	//检查每个输入项
	while(ImportDescriptor->FirstThunk)
	{
		//检查输入表项是否为ntdll.dll

		char* dllname = (char*)((ULONG)hMod + ImportDescriptor->Name);
		CString cstrName = dllname;
		OutputDebugString(cstrName);
		//如果不是，则跳到下一个处理

		if (stricmp(dllname , "ntdll.dll") !=0)
		{
			ImportDescriptor ++ ; 
			continue;
		}

		OutputDebugString(L"ntdll.dll find \n");
		ThunkData = (PIMAGE_THUNK_DATA)((ULONG)hMod + ImportDescriptor->OriginalFirstThunk);

		int no = 1;
		while(ThunkData->u1.Function)
		{
			//检查函数是否为NtDeviceIoControlFile

			char* functionname = (char*)((ULONG)hMod + ThunkData->u1.AddressOfData + 2);
			if (stricmp(functionname , "NtDeviceIoControlFile") == 0 )
			{
				OutputDebugString(L"NtDeviceIoControlFile find \n");
				//
				//如果是，那么记录原始函数地址
				//HOOK我们的函数地址
				//
				ULONG myaddr = (ULONG)NewNtDeviceIoControlFile;
				ULONG btw ; 
				PDWORD lpAddr = (DWORD *)((ULONG)hMod + (DWORD)ImportDescriptor->FirstThunk) +(no-1);
				pNtDeviceIoControlAddrOrgin = lpAddr; // 存放函数地址的地址
				pNtDeviceIoControl = (PVOID)(*(ULONG*)lpAddr) ; // 原来函数的地址

				DWORD oldProtect = 0;
				BOOL ret  = FALSE;
				// 修改页保护为可读可写
				ret = VirtualProtect(lpAddr,sizeof(ULONG),PAGE_READWRITE,&oldProtect);
				if(ret == FALSE)
				{
					DWORD errorCode = GetLastError();
					CString errorMsg = CUtility::GetErrorMsg(errorCode);
					CString msg ;
					msg.Format(_T("VirtualProtect false !!! errorCode:%d,errorMsg:%s \n"),errorCode,errorMsg);
					OutputDebugString(msg);
					return;
				}
				ret = WriteProcessMemory(GetCurrentProcess() , lpAddr , &myaddr , sizeof(ULONG), &btw );
				if(ret == FALSE)
				{
					DWORD errorCode = GetLastError();
					CString errorMsg = CUtility::GetErrorMsg(errorCode);
					CString msg ;
					msg.Format(_T("WriteProcessMemory false !!! errorCode:%d,errorMsg:%s \n"),errorCode,errorMsg);
					OutputDebugString(msg);
					return;
				}
				else
				{
					OutputDebugString(L"WriteProcessMemory true \n");
				}
				// 页保护属性改回去
				VirtualProtect(lpAddr,sizeof(ULONG),oldProtect,NULL);
				return ; 

			}

			no++;
			ThunkData ++;
		}
		ImportDescriptor ++;
	}
	return ; 
}

//////////////////////////////////////////////////////////////////////////
//
// CheckProcess 检查是否是需要挂钩的进程
//
//
//////////////////////////////////////////////////////////////////////////

BOOL CheckProcess()
{
	//在此加入你的进程过滤
	return TRUE ;
}

void UnkHookDeviceIoControl()
{
	OutputDebugString(_T("UnkHookDeviceIoControl into"));
	if(pNtDeviceIoControl)
	{
		DWORD oldProtect = 0;
		BOOL ret  = FALSE;
		// 修改页保护为可读可写
		ret = VirtualProtect(pNtDeviceIoControlAddrOrgin,sizeof(ULONG),PAGE_READWRITE,&oldProtect);
		if(ret == FALSE)
		{
			DWORD errorCode = GetLastError();
			CString errorMsg = CUtility::GetErrorMsg(errorCode);
			CString msg ;
			msg.Format(_T("VirtualProtect false !!! errorCode:%d,errorMsg:%s \n"),errorCode,errorMsg);
			OutputDebugString(msg);
			return;
		}
		// 如果被hook了，被hook地方还原为原来地址
		ret = WriteProcessMemory(GetCurrentProcess(),(LPVOID)pNtDeviceIoControlAddrOrgin,&pNtDeviceIoControl,sizeof(DWORD),NULL); 
		if(ret == FALSE)
		{
			DWORD errorCode = GetLastError();
			CString errorMsg = CUtility::GetErrorMsg(errorCode);
			CString msg ;
			msg.Format(_T("WriteProcessMemory false !!! errorCode:%d,errorMsg:%s \n"),errorCode,errorMsg);
			OutputDebugString(msg);
			return;
		}
		else
		{
			OutputDebugString(_T("WriteProcessMemory true"));
		}
		// 页保护属性改回去
		VirtualProtect(pNtDeviceIoControlAddrOrgin,sizeof(ULONG),oldProtect,NULL);
	}
}
BOOL APIENTRY DllMain( HANDLE hModule, 
	DWORD  ul_reason_for_call, 
	LPVOID lpReserved
	)
{
	//当加载DLL时，进行API HOOK
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			//检查是否是要过滤的进程
			if (CheckProcess() == FALSE)
			{        
				//如果不是，返回FALSE,将自身从进程中卸除
				return FALSE ; 
			}

			//HOOK API
			SuperHookDeviceIoControl();
		}
		break;
	case DLL_PROCESS_DETACH:
		UnkHookDeviceIoControl();
		break;
	}
	return TRUE;
}	