/*
 *
 *  RaopContext.h
 *
 *  Copyright (C) Frank Friemel - Feb 2013
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

#ifdef _WITH_PROXY
#include "Networking.h"
#endif

class CHairTunes;
class CRaopContext;

class IRaopContextCallback
{
	friend class CRaopContext;

public:
	virtual ~IRaopContextCallback()
	{
	}
protected:
	virtual void OnContextChanged(CRaopContext* pContext, PCWSTR strHint)
	{
	}
};

class CRaopContext : public CMyMutex
{
public:
	CRaopContext()
	{
		m_bIsStreaming		= false;
		m_bV4				= true;
		_m_bmInfo			= NULL;
		m_nBitmapBytes		= 0;
		m_rtpStart			= 0;
		m_rtpEnd			= 0;

		Announce();
	}
	CRaopContext(bool bV4, const char* strPeerIP, const char* strLocalIP)
	{
		m_bIsStreaming		= false;
		m_bV4				= bV4;
		_m_bmInfo			= NULL;
		m_nBitmapBytes		= 0;
		m_rtpStart			= 0;
		m_rtpEnd			= 0;
		
		if (strPeerIP != NULL)
			m_strPeerIP		= strPeerIP;

		if (strLocalIP != NULL)
			m_strLocalIP	= strLocalIP;

		Announce();
	}
	~CRaopContext()
	{
		if (m_pDecoder != NULL)
			m_pDecoder.reset();

		if (_m_bmInfo != NULL)
			delete _m_bmInfo;
	}
	void Announce()
	{
		Lock();

		m_lfFreq			= 44100.0l;
		
		_m_nTimeStamp		= ::time(NULL);
		_m_nTimeTotal		= 0;
		_m_nTimeCurrentPos	= 0;

		_m_nDurHours		= 0;
		_m_nDurMins			= 0;
		_m_nDurSecs			= 0;

		if (_m_bmInfo != NULL)
		{
			delete _m_bmInfo;
			_m_bmInfo		= NULL;
			m_nBitmapBytes	= 0;
		}
		Unlock();

		Fire(L"Announce");
	}
	void ResetSongInfos()
	{
		Lock();

		_m_str_daap_songalbum.Empty();
		_m_str_daap_songartist.Empty();
		_m_str_daap_trackname.Empty();

		Unlock();
	}
	time_t GetTimeStamp()
	{
		Lock();
		time_t tResult = _m_nTimeStamp;
		Unlock();

		return tResult;
	}
	void PutTimeStamp(time_t tValue)
	{
		Lock();
		_m_nTimeStamp	= tValue;
		Unlock();
	}
	time_t GetTimeTotal()
	{
		Lock();
		time_t tResult = _m_nTimeTotal;
		Unlock();

		return tResult;
	}
	void PutTimeTotal(time_t tValue)
	{
		Lock();
		_m_nTimeTotal	= tValue;
		Unlock();
	}
	time_t GetTimeCurrentPos()
	{
		Lock();
		time_t tResult = _m_nTimeCurrentPos;
		Unlock();

		return tResult;
	}
	void PutTimeCurrentPos(time_t tValue)
	{
		Lock();
		_m_nTimeCurrentPos	= tValue;
		Unlock();
	}
	long GetDurHours()
	{
		Lock();
		long nResult = _m_nDurHours;
		Unlock();

		return nResult;
	}
	void PutDurHours(long nValue)
	{
		Lock();
		_m_nDurHours	= nValue;
		Unlock();
	}
	long GetDurMins()
	{
		Lock();
		long nResult = _m_nDurMins;
		Unlock();

		return nResult;
	}
	void PutDurMins(long nValue)
	{
		Lock();
		_m_nDurMins	= nValue;
		Unlock();
	}
	long GetDurSecs()
	{
		Lock();
		long nResult = _m_nDurSecs;
		Unlock();

		return nResult;
	}
	void PutDurSecs(long nValue)
	{
		Lock();
		_m_nDurSecs	= nValue;
		Unlock();
	}
	WTL::CString GetSongalbum()
	{
		Lock();
		WTL::CString strResult = _m_str_daap_songalbum;
		Unlock();

		return strResult;
	}
	void PutSongalbum(WTL::CString strValue)
	{
		Lock();
		_m_str_daap_songalbum = strValue;
		Unlock();

		Fire(L"Trackalbum");
	}
	WTL::CString GetSongartist()
	{
		Lock();
		WTL::CString strResult = _m_str_daap_songartist;
		Unlock();

		return strResult;
	}
	void PutSongartist(WTL::CString strValue)
	{
		Lock();
		_m_str_daap_songartist = strValue;
		Unlock();

		Fire(L"Trackartist");
	}
	WTL::CString GetSongtrack()
	{
		Lock();
		WTL::CString strResult = _m_str_daap_trackname;
		Unlock();

		return strResult;
	}
	void PutSongtrack(WTL::CString strValue)
	{
		Lock();
		_m_str_daap_trackname = strValue;
		Unlock();

		Fire(L"Trackname");
	}
	Bitmap* GetBitmap()
	{
		// you need to lock yourself
		return _m_bmInfo;
	}
	void PutBitmap(Bitmap* bmValue)
	{
		ATLASSERT(_m_bmInfo == NULL);

		Lock();
		_m_bmInfo		= bmValue;

		if (_m_bmInfo == NULL)
			m_nBitmapBytes	= 0;
		Unlock();

		Fire(L"Bitmap");
	}
	bool GetHICON(HICON* phIcon)
	{
		bool bResult = false;

		Lock();

		if (_m_bmInfo != NULL)
			bResult = _m_bmInfo->GetHICON(phIcon) == Gdiplus::Ok ? true : false;
		Unlock();

		return bResult;
	}
	void DeleteImage()
	{
		Lock();

		if (_m_bmInfo != NULL)
		{
			delete _m_bmInfo;
			_m_bmInfo = NULL;
		}
		m_nBitmapBytes	= 0; 

		Unlock();
	}
	void RegisterCallback(const shared_ptr<IRaopContextCallback>& pCallback)
	{
		ATLASSERT(pCallback != NULL);

		Lock();
		m_mapCallbacks[pCallback.get()] = pCallback;
		Unlock();
	}
	void UnregisterCallback(const shared_ptr<IRaopContextCallback>& pCallback)
	{
		if (pCallback != NULL)
		{
			Lock();
			m_mapCallbacks.erase(pCallback.get());
			Unlock();
		}
	}
	void Fire(PCWSTR strHint)
	{
		Lock();

		if (!m_mapCallbacks.empty())
		{
			list < shared_ptr<IRaopContextCallback> > listStrongCallbacks;

			for (auto i = m_mapCallbacks.begin(); i != m_mapCallbacks.end(); )
			{
				shared_ptr<IRaopContextCallback> pStrongCallback = i->second.lock();

				if (pStrongCallback != NULL)
				{
					listStrongCallbacks.push_back(pStrongCallback);
					++i;
				}
				else
					m_mapCallbacks.erase(i++);
			}
			Unlock();

			for (auto i = listStrongCallbacks.begin(); i != listStrongCallbacks.end(); ++i)
				(*i)->OnContextChanged(this, strHint);
		}
		else
			Unlock();
	}
	void Duplicate(shared_ptr<CRaopContext>& pRaopContext)
	{
		ATLASSERT(!pRaopContext);
		pRaopContext = make_shared<CRaopContext>();

		if (pRaopContext)
		{
			Lock();

			pRaopContext->m_bV4			= m_bV4;
			pRaopContext->m_lfFreq		= m_lfFreq;

			// if (m_pDecoder)
			//	pRaopContext->m_pDecoder	= m_pDecoder;
			
			Unlock();
		}
	}
public:
#ifdef _WITH_PROXY
	CSocketBase				m_Proxy;
#endif

	CHttp					m_CurrentHttpRequest;
	CTempBuffer<BYTE>		m_Aesiv;
	long					m_sizeAesiv;
	CTempBuffer<BYTE>		m_Rsaaeskey;
	long					m_sizeRsaaeskey;
	string					m_strFmtp;
	string					m_strCport;
	string					m_strTport;
	bool					m_bIsStreaming;
	string					m_strNonce;
	bool					m_bV4;
	ci_string				m_strPeerIP;
	ci_string				m_strLocalIP;
	SOCKADDR_IN6			m_Peer;
	list<string>			m_listFmtp;
	double					m_lfFreq;
	shared_ptr<CHairTunes>	m_pDecoder;
	CTempBuffer<BYTE>		m_binBitmap;
	long					m_nBitmapBytes;
	unsigned long long		m_rtpStart;
	unsigned long long		m_rtpCur;
	unsigned long long		m_rtpEnd;
	WTL::CString			m_strUserAgent;
	WTL::CString			m_strPeerHostName;

	__declspec(property(get=GetTimeStamp,put=PutTimeStamp))				time_t			m_nTimeStamp;
	__declspec(property(get=GetTimeTotal,put=PutTimeTotal))				time_t			m_nTimeTotal;
	__declspec(property(get=GetTimeCurrentPos,put=PutTimeCurrentPos))	time_t			m_nTimeCurrentPos;
	__declspec(property(get=GetDurHours,put=PutDurHours))				long			m_nDurHours;
	__declspec(property(get=GetDurMins,put=PutDurMins))					long			m_nDurMins;
	__declspec(property(get=GetDurSecs,put=PutDurSecs))					long			m_nDurSecs;
	__declspec(property(get=GetSongalbum,put=PutSongalbum))				WTL::CString	m_str_daap_songalbum;
	__declspec(property(get=GetSongartist,put=PutSongartist))			WTL::CString	m_str_daap_songartist;
	__declspec(property(get=GetSongtrack,put=PutSongtrack))				WTL::CString	m_str_daap_trackname;
	__declspec(property(get=GetBitmap,put=PutBitmap))					Bitmap*			m_bmInfo;

protected:
	volatile time_t							_m_nTimeStamp;			// in Secs
	volatile time_t							_m_nTimeTotal;			// in Secs
	volatile time_t							_m_nTimeCurrentPos;		// in Secs
	volatile long							_m_nDurHours;
	volatile long							_m_nDurMins;
	volatile long							_m_nDurSecs;
	Bitmap*	volatile						_m_bmInfo;
	WTL::CString							_m_str_daap_songalbum;
	WTL::CString							_m_str_daap_songartist;
	WTL::CString							_m_str_daap_trackname;
	map<IRaopContextCallback*,  weak_ptr< IRaopContextCallback> >	m_mapCallbacks;
};
