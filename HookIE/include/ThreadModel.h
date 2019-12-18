// ThreadModel.h: interface for the CThreadModel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREADMODEL_H__AF8C47AB_E6E8_44E8_A859_30D85D25AB36__INCLUDED_)
#define AFX_THREADMODEL_H__AF8C47AB_E6E8_44E8_A859_30D85D25AB36__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

class CThreadModel  
{
public:
	CThreadModel(){};
	virtual ~CThreadModel(){};
	virtual void lock()=0;
	virtual void unlock()=0;
};

class CSingleThreadModel:public CThreadModel
{
public:
	CSingleThreadModel(){};
	virtual ~CSingleThreadModel(){};
	virtual void lock(){};
	virtual void unlock(){};

};

class CMultiThreadModel:public CThreadModel
{
private:
	CRITICAL_SECTION m_cs;
public:
	CMultiThreadModel()
	{
		InitializeCriticalSection(&m_cs);
	};
	virtual ~CMultiThreadModel()
	{
		DeleteCriticalSection(&m_cs);
	};
	virtual void lock(){EnterCriticalSection(&m_cs);};
	virtual void unlock(){LeaveCriticalSection(&m_cs);};
};

#endif // !defined(AFX_THREADMODEL_H__AF8C47AB_E6E8_44E8_A859_30D85D25AB36__INCLUDED_)
