#ifndef _LOGPOLICY_
#define _LOGPOLICY_
#include <windows.h>
#include <stdio.h>
#include <TCHAR.h>
#include "ThreadModel.h"

#ifdef _MT
#include "Process.h"
#endif

#define MAXBUFSIZE	800 * 1024
#define MAXFILENAME 256

typedef enum LOGLEVEL
{
	LOGSYSTEM =0,
	LOGERROR,
	LOGWARNING,
	LOGINFO,
	LOGDEBUG
}LOGLEVEL;

static unsigned short CONSOLECOLORS[]=
{
	FOREGROUND_GREEN ,FOREGROUND_RED,FOREGROUND_RED,
	FOREGROUND_GREEN,FOREGROUND_GREEN
};

static TCHAR * LOGMSG[]={_T("LOGSYSTEM"),_T("LOGERROR"),_T("LOGWARNING"),
_T("LOGINFO"),_T("LOGDEBUG")};

class LogPolicy
{
protected:
	SYSTEMTIME m_time;
	TCHAR* m_cbuf;
	LOGLEVEL m_eLogLevel;
	LogPolicy * m_pNextPolicy;
	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)=0;
	void PassOn(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(m_pNextPolicy){
			m_pNextPolicy->WriteLog(level,t,buf);
		}
	}
	void clean()
	{
		if(m_pNextPolicy){
			delete m_pNextPolicy;
			m_pNextPolicy=NULL;
		}
	}
public:
	LogPolicy(LogPolicy * pNextPolicy){
		m_pNextPolicy=pNextPolicy;
		m_eLogLevel=LOGSYSTEM;
		m_cbuf = new TCHAR[MAXBUFSIZE];
	};
	
	virtual ~LogPolicy(){
		clean();
		delete m_cbuf;
	};

	void setloglevel(LOGLEVEL level){
		m_eLogLevel=level;
		if(m_pNextPolicy)
			m_pNextPolicy->setloglevel(level);
	}

	virtual void Log(LOGLEVEL level,const TCHAR * fmt,...)
	{
		if(level <= m_eLogLevel){
			GetLocalTime(&m_time);
			va_list valist;
			va_start(valist,fmt);
			_vsntprintf_s(m_cbuf,MAXBUFSIZE,_TRUNCATE,fmt,valist);
			va_end(valist);
			WriteLog(level,m_time,m_cbuf);
		}
	}	
};

class FilePolicy:public LogPolicy
{
protected:
	TCHAR m_cPrefix[20];
	TCHAR m_cPath[MAXFILENAME + 1];
	TCHAR m_cFileName[MAXFILENAME];
	int m_jnlsize;
	SYSTEMTIME m_time;
	
	virtual void Rotate(const SYSTEMTIME t)
	{
		//rotate file
		TCHAR filename[MAXFILENAME];
		_sntprintf_s(filename,MAXFILENAME,_TRUNCATE,TEXT("%s\\%s%04d%02d%02d.log"),
			m_cPath,m_cPrefix,t.wYear,t.wMonth,t.wDay);
		if(_tcscmp(filename,m_cFileName)!=0)
		{
			_tcsncpy_s(m_cFileName,MAXFILENAME,filename,MAXFILENAME-1);
		}
		else
		{
			long size = 0;

			FILE* hFile = NULL;
			errno_t err = _tfopen_s(&hFile,m_cFileName,TEXT("r"));
			if(hFile)
			{
				size = ftell(hFile);
				fclose(hFile);
				hFile = NULL;
			}

			if(size/(1024*1024)>=m_jnlsize)
			{
				TCHAR backup[MAXFILENAME];
				_sntprintf_s(backup,MAXFILENAME,_TRUNCATE,TEXT("%s\\backup_%04d%02d%02d%02d%02d%02d.log"),
					m_cPath,t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);

				CopyFile(m_cFileName,backup,FALSE);

				_tfopen_s(&hFile,m_cFileName,TEXT("w"));
				if(hFile)
				{
					fclose(hFile);
					hFile = NULL;
				}
			}
		}
	};

	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(level <= m_eLogLevel){
			Rotate(t);
			
			FILE* hFile = NULL;

			errno_t err = _tfopen_s(&hFile,m_cFileName,TEXT("a+"));
			if(err != 0)
			{
				if(CreateDirectory(m_cPath,NULL)){
					_tfopen_s(&hFile,m_cFileName,TEXT("a+"));
				}
			}

			if(hFile)
			{
				//write time and level first
				_ftprintf(hFile,TEXT("%04d-%02d-%02d %02d:%02d:%02d:%03d %s %s\r\n"),
					t.wYear,t.wMonth,t.wDay,
					t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,LOGMSG[level],buf);
				fflush(hFile);
				fclose(hFile);
				hFile = NULL;
			}

			PassOn(level,t,buf);
		}
	};

public:
	FilePolicy(LogPolicy * pLogPolicy):LogPolicy(pLogPolicy)
	{
		setprefix(TEXT(""));
		setpath(TEXT("C:\\log\\"));
		setsize(5);
	}

	FilePolicy(const TCHAR * Prefix,LogPolicy * pLogPolicy):LogPolicy(pLogPolicy)
	{
		setprefix(Prefix);
		setpath(TEXT("C:\\log\\"));
		setsize(5);
	}
	
	virtual ~FilePolicy()
	{
		clean();	
	};
	
	void setprefix(const TCHAR * Prefix)
	{
		ZeroMemory(m_cPrefix,20*sizeof(TCHAR));
		_tcsncpy_s(m_cPrefix,20,Prefix,19);
	}

	void setpath(const TCHAR * Path)
	{
		ZeroMemory(m_cPath,(MAXFILENAME + 1)*sizeof(TCHAR));
		_tcsncpy_s(m_cPath,MAXFILENAME+1,Path,MAXFILENAME);
	}

	void setsize(int jnlSize)
	{
		m_jnlsize = jnlSize;
	}
};

class DebugOutPolicy:public LogPolicy
{
public:
	DebugOutPolicy(LogPolicy * pNextPolicy):LogPolicy(pNextPolicy){
		
	}
	virtual ~DebugOutPolicy()
	{
	}
	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(level <= m_eLogLevel){
			TCHAR temp[2048];
			_sntprintf_s(temp,2048,_TRUNCATE,TEXT("%04d-%02d-%02d %02d:%02d:%02d:%03d %s %s\r\n"),
				t.wYear,t.wMonth,t.wDay,
				t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,LOGMSG[level],buf);
			OutputDebugString(temp);
		}
		PassOn(level,t,buf);
	}

};

#ifdef _MT
#define WM_LOGMSG WM_USER + 1
class BufferedPolicy:public LogPolicy
{
private:
	HANDLE m_hBackThread;
	unsigned int m_nThreadID;
	static  unsigned int __stdcall run(void * pParam){
		BufferedPolicy * pPolicy=(BufferedPolicy *)pParam;
		MSG msg;
		while(GetMessage(&msg,NULL,0,0L)){
			if(msg.message==WM_LOGMSG){
				SYSTEMTIME * t=(SYSTEMTIME *)msg.lParam;
				pPolicy->PassOn((LOGLEVEL)msg.wParam,*t,(TCHAR *)(msg.lParam+sizeof(SYSTEMTIME)));
				delete[] (unsigned char*)msg.lParam;
			}
		}
		return 0;
	}		
public:
	BufferedPolicy(LogPolicy * pNextPolicy):LogPolicy(pNextPolicy){
		m_hBackThread=(HANDLE)_beginthreadex(NULL,0,run,this,0,&m_nThreadID);
	}
	virtual ~BufferedPolicy()
	{
		if(m_hBackThread){
			PostThreadMessage(m_nThreadID,WM_QUIT,0,0l);
			WaitForSingleObject(m_hBackThread,INFINITE);
			CloseHandle(m_hBackThread);
		}
		clean();
	}
private:
	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(m_hBackThread && (level <= m_eLogLevel)){
			TCHAR * p=new TCHAR[sizeof(SYSTEMTIME)+_tcslen(buf)+sizeof(TCHAR)];
			if(p){
				memcpy(p,&t,sizeof(SYSTEMTIME));
				_tcsncpy_s(p+sizeof(SYSTEMTIME),sizeof(SYSTEMTIME)+_tcslen(buf)+sizeof(TCHAR),buf,_tcslen(buf)+sizeof(TCHAR));
				while(!PostThreadMessage(m_nThreadID,WM_LOGMSG,level,(LPARAM)p))
				{
					Sleep(10);
				}
			}
		}
	}
};

class ThreadSafePolicy:public LogPolicy,public CMultiThreadModel
{
public:
	ThreadSafePolicy(LogPolicy * pLogPolicy):LogPolicy(pLogPolicy)
	{
		
	}

	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(level <= m_eLogLevel){
			PassOn(level,t,buf);
		}
	}

	virtual void Log(LOGLEVEL level,const TCHAR * fmt,...)
	{
		if(level <= m_eLogLevel){
			lock();
			GetLocalTime(&m_time);
			va_list valist;
			va_start(valist,fmt);
			_vsntprintf_s(m_cbuf,MAXBUFSIZE,_TRUNCATE,fmt,valist);
			va_end(valist);
			WriteLog(level,m_time,m_cbuf);
			unlock();
		}
	}
};

#endif

class ConsolePolicy:public LogPolicy
{
private:
	BOOL m_bOwner;
	HANDLE m_hConsole;
public:
	ConsolePolicy(LogPolicy * pLogPolicy):LogPolicy(pLogPolicy){
		m_bOwner=AllocConsole();
		m_hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
		
	};
	~ConsolePolicy(){
		if(m_bOwner){
			FreeConsole();
		}
		clean();
	}
	
private:
	virtual void WriteLog(LOGLEVEL level,const SYSTEMTIME & t,const TCHAR * buf)
	{
		if(level <= m_eLogLevel){
			if(m_hConsole){
				TCHAR ttext[40];
				_sntprintf_s(ttext,40,_TRUNCATE,TEXT("%04d-%02d-%02d %02d:%02d:%02d:%03d %s "),
					t.wYear,t.wMonth,t.wDay,
					t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,LOGMSG[level]);

				SetConsoleTextAttribute(m_hConsole,CONSOLECOLORS[level]);

				unsigned long written;
				
				WriteConsole(m_hConsole,ttext,_tcslen(ttext),&written,NULL);
				
				WriteConsole(m_hConsole,buf,_tcslen(buf),&written,NULL);
				
				WriteConsole(m_hConsole,TEXT("\n"),_tcslen(TEXT("\n")),&written,NULL);
				
				FlushFileBuffers(m_hConsole);
			}
			PassOn(level,t,buf);
		}
	}
};


#endif /*_LOGPOLICY_*/
