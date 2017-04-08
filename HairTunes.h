/*
 *
 *  HairTunes.h
 *
 *  Copyright (C) Frank Friemel - May 2013
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

#include "RaopDefs.h"

#define BUFFER_FRAMES	512
#define START_FILL		282
#define	MIN_FILL		30
#define	MAX_FILL		500

class CRaopContext;
struct ao_device;
struct alac_file;
struct ao_option;

typedef struct 
{
	double hist[2];
	double a[2];
	double b[3];
} biquad_t;

class CHairTunes : protected CMyThread, protected IRtpRequestHandler
{
public:
	CHairTunes(void);
	~CHairTunes(void);

	static int GetStartFill();
	static void SetStartFill(int nValue);

	static LPCSTR GetSoundId();
	static void SetSoundId(LPCSTR strValue);

public:
	bool Start(const shared_ptr<CRaopContext>& pContext
				, LPCTSTR	strSoundRedirection		= NULL
				, bool		bRedirKeepAlive			= false
				, HANDLE	hRedirectionProcess		= NULL
				, HANDLE	hRedirectionData		= INVALID_HANDLE_VALUE
				, PHANDLE	phRedirProcess			= NULL
				, PHANDLE	phRedirData				= NULL
				);
	void Stop();

	void Flush();
	void SetPause(bool bValue);

	void SetVolume(double lfValue);

	static bool StartRedirection(LPCTSTR	strSoundRedirection
								, PHANDLE	phRedirProcess
								, PHANDLE	phRedirData);
	static void TerminateRedirection(HANDLE& hProcess);

	USHORT GetServerPort();			// host order
	USHORT GetControlPort();		// host order
	USHORT GetTimingPort();			// host order

	inline int StuffBuffer(const short* inptr, short* outptr, int nFrameSize, double bf_playback_rate);

#ifdef _WITH_PROXY
	USHORT GetProxyControlPort();	// host order
	USHORT GetProxyTimingPort();	// host order
#endif

protected:
	virtual BOOL OnStart();
	virtual void OnStop();
	virtual void OnEvent();

protected:
	HANDLE								m_hRedirProcess;
	bool								m_bRedirKeepAlive;

	long								m_nFixVolume;
	long								m_lcg_prev;
	short								m_rand_a;
	short								m_rand_b;

	ao_device*							m_libao_dev;
	bool								m_bLiveOutput;
	HANDLE								m_libao_hfile;
	string								m_libao_file;
	ao_option*							m_ao_opts;

	int									m_nControlport;
	int									m_nTimingport;

	CTempBuffer<BYTE>					m_AesKey;
	CTempBuffer<BYTE>					m_AesIv;

	CTempBuffer<BYTE>					m_OutBuf;

	AES_KEY								m_AES;

	map<int, int>						m_mapFmtp;
	int									m_nSamplingRate;
	int									m_nFrameSize;
	bool								m_bFlush;
	bool								m_bPause;

	alac_file*							m_decoder_info;

	shared_ptr<CRtpEndpoint>			m_RtpClient_Data;
#ifdef _HAIRTUNES_TCP
	shared_ptr<CRtpEndpoint>			m_RtpClient_Tcp_Data;
#endif
	shared_ptr<CRtpEndpoint>			m_RtpClient_Control;
	shared_ptr<CRtpEndpoint>			m_RtpClient_Timing;

#ifdef _WITH_PROXY
	shared_ptr<CRtpEndpoint>			m_RtpClient_Proxy_Control;
	shared_ptr<CRtpEndpoint>			m_RtpClient_Proxy_Timing;
public:
	int									m_Proxy_serverport;
	int									m_Proxy_controlport;
	int									m_Proxy_timingport;
	struct sockaddr_in6					m_rtp_proxy_server;
	ULONG								m_rtp_proxy_server_size;
#endif

protected:
	typedef list< shared_ptr<CRtpPacket> > _CQueue;
	CMyMutex							m_mtxQueue;
	_CQueue								m_Queue;

	double								m_bf_playback_rate;
	double								m_bf_est_drift;   
	biquad_t							m_bf_drift_lpf;
	double								m_bf_est_err;
	double								m_bf_last_err;
	biquad_t							m_bf_err_lpf;
	biquad_t							m_bf_err_deriv_lpf;

	double								m_desired_fill;
	int									m_fill_count;
	weak_ptr<CRaopContext>				m_pRaopContext;

protected:
	virtual void OnRequest(CRtpEndpoint* pRtpEndpoint, shared_ptr<CRtpPacket>& packet);

	inline void QueuePacket(shared_ptr<CRtpPacket>& p);
	inline int AlacDecode(unsigned char* pDest, const unsigned char* pBuf, int len);
	inline short DitheredVol(short sample);
	inline short LcgRand();

	void BF_EstReset();
	void BF_EstUpdate(int fill);

protected:
	class _CResendThread : public CMyThread
	{
	protected:
		virtual BOOL OnStart()
		{
			m_mtxCheckMap.Lock();
			m_mapCheck.clear();
			m_dwTimeout = INFINITE;
			m_mtxCheckMap.Unlock();

			return TRUE;
		}
		virtual void OnStop()
		{
			if (m_RtpClient_Control != NULL)
				m_RtpClient_Control.reset();
		}
		virtual void OnTimeout()
		{
			list<USHORT> listSeqNrToResend;

			m_mtxCheckMap.Lock();

			ATLASSERT(!m_mapCheck.empty() || m_dwTimeout == INFINITE);

			for (auto i = m_mapCheck.begin(); i != m_mapCheck.end(); )
			{
				if (i->second > 0)
				{
					i->second--;
					
					listSeqNrToResend.push_back(i->first);
					++i;
				}
				else
				{
					ATLASSERT(i->second == 0);
					m_mapCheck.erase(i++);
				}
			}
			if (m_mapCheck.empty())
				m_dwTimeout = INFINITE;

			m_mtxCheckMap.Unlock();

			for (auto i = listSeqNrToResend.begin(); i != listSeqNrToResend.end(); ++i)
			{
				if (!_RequestResend(*i, 1))
				{
					m_mtxCheckMap.Lock();

					m_mapCheck.erase(*i);

					if (m_mapCheck.empty())
						m_dwTimeout = INFINITE;

					m_mtxCheckMap.Unlock();
				}
			}
		}
	public:
		inline void Push(USHORT nSeq, short nCount)
		{
			ATLASSERT(nCount > 0);

			m_mtxCheckMap.Lock();

			USHORT nSeqDest = nSeq+nCount;

			for (USHORT n = nSeq; n < nSeqDest; ++n)
			{
				// retry up to 4 times
				m_mapCheck[n] = 4;
			}
			if (!m_mapCheck.empty())
			{
				// send resend request every 15 ms
				m_dwTimeout = 15;
				SetEvent(m_hEvent);
			}
			m_mtxCheckMap.Unlock();

			_RequestResend(nSeq, nCount);
		}
		inline bool Pull(USHORT nSeq)
		{
			bool bResult = false;

			m_mtxCheckMap.Lock();

			auto i = m_mapCheck.find(nSeq);

			if (i != m_mapCheck.end())
			{
				bResult = true;
				m_mapCheck.erase(i);
	
				if (m_mapCheck.empty())
					m_dwTimeout = INFINITE;
			}
			m_mtxCheckMap.Unlock();

			return bResult;
		}
		inline void Remove(USHORT nSeq)
		{
			if (m_dwTimeout != INFINITE)
			{
				m_mtxCheckMap.Lock();

				for (auto i = m_mapCheck.begin(); i != m_mapCheck.end(); )
				{
					short nSeqDiff = nSeq - i->first;

					ATLASSERT(nSeqDiff != 0);

					if (nSeqDiff >= 0)
						m_mapCheck.erase(i++);
					else
						++i;
				}
				if (m_mapCheck.empty())
					m_dwTimeout = INFINITE;
				m_mtxCheckMap.Unlock();
			}
		}
	protected:
		bool _RequestResend(USHORT nSeq, short nCount);

	public:
		shared_ptr<CRtpEndpoint>	m_RtpClient_Control;
		USHORT						m_nControlport;
		
	protected:
		CMyMutex					m_mtxCheckMap;
		map<USHORT, int>			m_mapCheck;
	};
protected:
	_CResendThread					m_threadResend;
};


