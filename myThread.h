/*
 *
 *  myThread.h 
 *
 *  Copyright (C) Frank Friemel - April 2011
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

class CMyThread
{
public:
	CMyThread(BOOL bManualResetEvent = FALSE, PCTSTR strEventName = NULL)
	{
		m_hThread		= NULL;
		m_bSelfDelete	= false;
		m_dwTimeout		= INFINITE;
		m_hStopEvent	= ::CreateEvent(NULL, TRUE, FALSE, NULL);
		m_hEvent		= ::CreateEvent(NULL, bManualResetEvent, FALSE, strEventName);

		ATLASSERT(m_hStopEvent != NULL);
		ATLASSERT(m_hEvent != NULL);
	}
	virtual ~CMyThread()
	{
		if (IsRunning())
			Stop();
		if (m_hThread != NULL)
			CloseHandle(m_hThread);
		if (m_hEvent != NULL)
			CloseHandle(m_hEvent);
		if (m_hStopEvent != NULL)
			CloseHandle(m_hStopEvent);
	}
public:
	BOOL Start()
	{
		ATLASSERT(m_hThread == NULL);
		ResetEvent(m_hStopEvent);

		unsigned usThreadAddr;
		m_hThread = (HANDLE) _beginthreadex(NULL, 0, _Run, this, 0, &usThreadAddr);

		return m_hThread != NULL;
	}
	void Stop()
	{
		if (m_hThread != NULL)
		{
			::SetEvent(m_hStopEvent);

			WaitForSingleObject(m_hThread, INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	inline BOOL IsRunning() const
	{
		return m_hThread != NULL && WaitForSingleObject(m_hThread, 0) != WAIT_OBJECT_0;
	}
	inline BOOL IsStopping() const
	{
		return WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0;
	}
protected:
	virtual BOOL OnStart()
	{
		return TRUE;
	}
	virtual void OnStop()
	{
	}
	virtual void OnTimeout()
	{
	}
	virtual void OnEvent()
	{
	}
private:
	static unsigned int __stdcall _Run(LPVOID pParam)
	{
		CMyThread* pThis = (CMyThread*)pParam;
		return pThis->Run();
	}
	unsigned int Run()
	{
		if (OnStart())
		{
			HANDLE h[2];

			h[0] = m_hStopEvent;
			h[1] = m_hEvent;

			bool bRun = true;

			while (bRun)
			{
				switch(WaitForMultipleObjects(2, h, FALSE, m_dwTimeout))
				{
					case WAIT_OBJECT_0:
					{
						OnStop();
						bRun = false;
					}
					break;

					case WAIT_TIMEOUT:
					{
						OnTimeout();
					}
					break;

					default:
					{
						OnEvent();
					}
					break;
				}
			}
		}
		if (m_bSelfDelete)
		{
			CloseHandle(m_hThread);
			m_hThread = NULL;
			delete this;
		}
		_endthreadex(0);
		return 0;
	}
public:
	HANDLE	m_hThread;
	HANDLE	m_hStopEvent;
	HANDLE	m_hEvent;
	bool	m_bSelfDelete;
	DWORD	m_dwTimeout;
};

