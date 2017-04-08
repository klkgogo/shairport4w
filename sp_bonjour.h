/*
 *
 *  sp_bonjour.h
 *
 *  Copyright (C) Frank Friemel - Mar 2013
 *
 *  This file is part of Shairport4w.
 *
 *  Shairport4w is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  Shairport4w is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#pragma once

#include "myThread.h"
#include "Bonjour/dns_sd.h"

extern bool InitBonjour();
extern void DeInitBonjour();

class CRegisterServiceEvent;

class CServiceResolveCallback
{
public:
	virtual bool OnServiceResolved(CRegisterServiceEvent* pServiceEvent, const char* strHost, USHORT nPort) = 0;
};

class CDnsSD_Thread : protected CMyThread
{
public:
	CDnsSD_Thread();
	~CDnsSD_Thread();

public:
	void ProcessResult(DNSServiceRef sdref, DWORD dwTimeout);
	void Stop();

protected:
	virtual BOOL OnStart();
	virtual void OnStop();
	virtual void OnEvent();
	virtual void OnTimeout();

public:
	bool			m_bTimedout;
protected:
	DNSServiceRef	m_sdref;
	CSocketBase		m_socket;
};

class CDnsSD_Register : public CDnsSD_Thread
{
public:
	CDnsSD_Register()
	{
		m_nErrorCode	= 0;
		m_hErrorEvent	= ::CreateEventA(NULL, FALSE, FALSE, NULL);
		ATLASSERT(m_hErrorEvent);
	}
	~CDnsSD_Register()
	{
		if (m_hErrorEvent)
			CloseHandle(m_hErrorEvent);
	}
public:
	bool Start(BYTE* addr, LPCSTR strApName, BOOL bNoMetaInfo, BOOL bPassword, USHORT nPort);
public:
	HANDLE					m_hErrorEvent;
	DNSServiceErrorType		m_nErrorCode;
};

class CDnsSD_BrowseForService : public CDnsSD_Thread
{
public:
	virtual void OnDNSServiceBrowseReply(
									DNSServiceRef                       sdRef,
									DNSServiceFlags                     flags,
									uint32_t                            interfaceIndex,
									DNSServiceErrorType                 errorCode,
									const char                          *serviceName,
									const char                          *regtype,
									const char                          *replyDomain
									);
	bool Start(const char* strRegType, HWND hWnd, UINT nMsg);

protected:
	UINT	m_nMsg;
	HWND	m_hWnd;
};

class CDnsSD_ServiceQueryRecord : public CDnsSD_Thread
{
public:
	bool Start(CRegisterServiceEvent* pRSEvent, uint32_t interfaceIndex, const char* fullname, uint16_t	rrtype, uint16_t rrclass);
};

class CRegisterServiceEvent : public CMyMutex
{
public:
	CRegisterServiceEvent()
	{
		m_bRegister			= false;
		m_pCallback			= NULL;
		m_nInterfaceIndex	= 0;
		m_nPort				= 0;
		m_bCallbackResult	= false;
	}
	CRegisterServiceEvent(const CRegisterServiceEvent& i)
	{
		m_pCallback			= NULL;
		operator=(i);
	}
	bool Resolve(CServiceResolveCallback* pCallback, DWORD dwTimeout = 2500);
	bool QueryHostAddress();

	CRegisterServiceEvent& operator=(const CRegisterServiceEvent& i)
	{
		m_bRegister				= i.m_bRegister;
		m_strService			= i.m_strService;
		m_strRegType			= i.m_strRegType;
		m_strReplyDomain		= i.m_strReplyDomain;
		m_nInterfaceIndex		= i.m_nInterfaceIndex;
		m_strHost				= i.m_strHost;
		m_nPort					= i.m_nPort;
		m_strFullname			= i.m_strFullname;

		return *this;
	}
	inline bool IsValid()
	{
		return m_bRegister && !m_strService.empty();
	}
	bool operator==(const CRegisterServiceEvent& i)
	{
		return m_strService == i.m_strService ? true : false;
	}
public:
	bool						m_bRegister;
	ci_string					m_strService;
	ci_string					m_strRegType;
	ci_string					m_strReplyDomain;
	ULONG						m_nInterfaceIndex;
	ci_string					m_strHost;
	USHORT						m_nPort; // net order
	ci_string					m_strTXT;
	ci_string					m_strFullname;
public:
	// transient members
	CServiceResolveCallback*	m_pCallback;	
	bool						m_bCallbackResult;
	CDnsSD_ServiceQueryRecord	m_threadQueryRecord;
	struct sockaddr_in			m_sa4;
};
