/*
 *
 *  HairTunes.cpp
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

#include "StdAfx.h"
#include "HairTunes.h"

#include "stdint_win.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "ao/ao.h"
#include "RAOPDefs.h"
#include "alac.h"

#define FRAME_BYTES		(m_nFrameSize<<2)
#define OUTFRAME_BYTES	((m_nFrameSize+3)<<2)
#define CONTROL_A		(1e-4)
#define CONTROL_B		(1e-1)

static LONG			c_nStartFill			= START_FILL;
string				c_strlibao_deviceid;	// ao_options expects "char*"

static USHORT		c_nUniquePortCounter	= 6000;
static CMyMutex		c_mtxUniquePortCounter;

static inline void biquad_init(biquad_t *bq, double a[], double b[]) 
{
	bq->hist[0] = bq->hist[1] = 0.0l;

	memcpy(bq->a, a, 2*sizeof(double));
	memcpy(bq->b, b, 3*sizeof(double));
}

static inline void biquad_lpf(biquad_t *bq, double freq, double Q, double sampling_rate, double frame_size)
{
	double w0		= 2.0l * M_PI * freq * frame_size / sampling_rate;
	double alpha	= sin(w0) / (2.0l * Q);
	double a_0		= 1.0l + alpha;
	
	double b[3];
	double a[2];

	b[0] = (1.0l - cos(w0)) / (2.0l * a_0);
	b[1] = (1.0l - cos(w0)) / a_0;
	b[2] = b[0];
	a[0] = -2.0l * cos(w0) / a_0;
	a[1] = (1 - alpha) / a_0;

	biquad_init(bq, a, b);
}

static inline double biquad_filt(biquad_t *bq, double in) 
{
	double	w	= in - bq->a[0]*bq->hist[0] - bq->a[1]*bq->hist[1];
	double	out = bq->b[1]*bq->hist[0] + bq->b[2]*bq->hist[1] + bq->b[0]*w;
    
	bq->hist[1] = bq->hist[0];
	bq->hist[0] = w;

	return out;
}

static inline USHORT GetUniquePortNumber()
{
	c_mtxUniquePortCounter.Lock();

	if (c_nUniquePortCounter > 12000)
		c_nUniquePortCounter = 6000;

	USHORT nResult = c_nUniquePortCounter++;

	c_mtxUniquePortCounter.Unlock();

	return nResult;
}

CHairTunes::CHairTunes(void) : CMyThread(TRUE)
{
	m_lcg_prev			= 12345; 
	m_rand_a			= 0;
	m_rand_b			= 0;

	m_nFixVolume		= 0x10000;
	m_libao_dev			= NULL;
	m_ao_opts			= NULL;
	m_decoder_info		= NULL;
	m_bRedirKeepAlive	= false;
	m_hRedirProcess		= NULL;
	m_libao_hfile		= INVALID_HANDLE_VALUE;
	m_bFlush			= false;
	m_bPause			= false;

	m_bf_playback_rate	= 1.0l;
	m_bf_est_drift		= 0.0l; 
	m_bf_est_err		= 0.0l;
	m_bf_last_err		= 0.0l;

	memset(&m_bf_drift_lpf, 0, sizeof(biquad_t));
	memset(&m_bf_err_lpf, 0, sizeof(biquad_t));
	memset(&m_bf_err_deriv_lpf, 0, sizeof(biquad_t));

	m_bLiveOutput		= true;
}

CHairTunes::~CHairTunes(void)
{
	Stop();
}

void CHairTunes::TerminateRedirection(HANDLE& hProcess)
{
	if (hProcess != NULL)
	{
		::TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		hProcess = NULL;
	}
}

bool CHairTunes::StartRedirection(LPCTSTR	strSoundRedirection
								, PHANDLE	phRedirProcess
								, PHANDLE	phRedirData)
{
	bool bResult = false;

	if (strSoundRedirection != NULL && phRedirProcess != NULL && phRedirData != NULL)
	{
		*phRedirProcess = Exec(strSoundRedirection, 0, true, phRedirData);
		
		if (*phRedirProcess != NULL)
		{
			bResult = true;
		}
		else
		{
			*phRedirData = INVALID_HANDLE_VALUE;
		}
	}
	return bResult;
}

int CHairTunes::GetStartFill()
{
	return (int)c_nStartFill;
}

void CHairTunes::SetStartFill(int nValue)
{
	InterlockedExchange(&c_nStartFill, nValue);
}

LPCSTR CHairTunes::GetSoundId()
{
	return !c_strlibao_deviceid.empty() ? c_strlibao_deviceid.c_str() : NULL;
}

void CHairTunes::SetSoundId(LPCSTR strValue)
{
	if (strValue != NULL)
		c_strlibao_deviceid = strValue;
	else
		c_strlibao_deviceid.clear();
}

short CHairTunes::LcgRand() 
{ 
	m_lcg_prev = m_lcg_prev * 69069 + 3; 
	return m_lcg_prev & 0xffff; 
} 

short CHairTunes::DitheredVol(short sample)
{
	long out;

	out = (long)sample * m_nFixVolume;
    
	if (m_nFixVolume < 0x10000) 
	{
		m_rand_b = m_rand_a;
		m_rand_a = LcgRand();
		out += m_rand_a;
		out -= m_rand_b;
	}
	return out >> 16;
}

int CHairTunes::StuffBuffer(const short* inptr, short* outptr, int nFrameSize, double bf_playback_rate) 
{
	int		i;
	int		stuffsamp	= nFrameSize;
	int		stuff		= 0;

	if (nFrameSize > 1)
	{
		double	p_stuff		= 1.0l - pow(1.0l - fabs(bf_playback_rate - 1.0l), nFrameSize);

		if (rand() < (p_stuff * RAND_MAX)) 
		{
			stuff		= bf_playback_rate > 1.0l ? -1 : 1;
			stuffsamp	= rand() % (nFrameSize - 1);
		}
	}
	for (i = 0; i < stuffsamp; i++) 
	{   
		// the whole frame, if no stuffing
		*outptr++ = DitheredVol(*inptr++);
		*outptr++ = DitheredVol(*inptr++);
	}
	if (stuff) 
	{
		if (stuff == 1) 
		{
			// interpolate one sample
			*outptr++ = DitheredVol(((long)inptr[-2] + (long)inptr[0]) >> 1);
			*outptr++ = DitheredVol(((long)inptr[-1] + (long)inptr[1]) >> 1);
		} 
		else if (stuff == -1) 
		{
			inptr++;
			inptr++;
		}
		for (i = stuffsamp; i < nFrameSize + stuff; i++) 
		{
			*outptr++ = DitheredVol(*inptr++);
			*outptr++ = DitheredVol(*inptr++);
		}
	}
	return nFrameSize + stuff;
}

int CHairTunes::AlacDecode(unsigned char* pDest, const unsigned char* pBuf, int len)
{
	unsigned char	buf[RAOP_PACKET_MAX_SIZE];
    unsigned char*	packet = buf;

	if (len > RAOP_PACKET_MAX_SIZE)
		packet = new unsigned char[len];

    unsigned char	iv[16];
    
	memcpy(iv, m_AesIv, sizeof(iv));
    
	int aeslen = len & ~0xf;

	AES_cbc_encrypt(pBuf, packet, aeslen, &m_AES, iv, AES_DECRYPT);  
	memcpy(packet+aeslen, pBuf+aeslen, len-aeslen);  

	int outsize;

    decode_frame(m_decoder_info, packet, pDest, &outsize);

    ATLASSERT(outsize <= FRAME_BYTES);

	if (packet != buf)
		delete [] packet;
	return outsize;
}

void CHairTunes::BF_EstReset() 
{
    biquad_lpf(&m_bf_drift_lpf    , 1.0l / 180.0l, 0.3l , m_nSamplingRate, m_nFrameSize);
    biquad_lpf(&m_bf_err_lpf      , 1.0l / 10.0l , 0.25l, m_nSamplingRate, m_nFrameSize);
    biquad_lpf(&m_bf_err_deriv_lpf, 1.0l / 2.0l  , 0.2l , m_nSamplingRate, m_nFrameSize);
    
	m_fill_count			= 0;
    m_desired_fill			= 0.0l;
    m_bf_playback_rate		= 1.0l;
    m_bf_est_err			= 0.0l;
	m_bf_last_err			= 0.0l;
}

void CHairTunes::BF_EstUpdate(int fill) 
{
	if (m_fill_count < 1000)
	{
        m_desired_fill += ((double)fill) / 1000.0l;
        m_fill_count++;
        return;
    }
    double buf_delta = fill - m_desired_fill;
    
	m_bf_est_err = biquad_filt(&m_bf_err_lpf, buf_delta);
    
	double err_deriv = biquad_filt(&m_bf_err_deriv_lpf, m_bf_est_err - m_bf_last_err);

	double adj_error = m_bf_est_err*CONTROL_A;

    m_bf_est_drift = biquad_filt(&m_bf_drift_lpf, CONTROL_B*(adj_error + err_deriv) + m_bf_est_drift);
    m_bf_playback_rate = 1.0l + adj_error + m_bf_est_drift;

    m_bf_last_err = m_bf_est_err;
}

bool CHairTunes::Start(const shared_ptr<CRaopContext>& pContext
						, LPCTSTR strSoundRedirection	/*= NULL*/
						, bool bRedirKeepAlive			/*= false*/
						, HANDLE hRedirectionProcess	/*= NULL*/
						, HANDLE hRedirectionData		/*= INVALID_HANDLE_VALUE*/
						, PHANDLE phRedirProcess		/*= NULL*/	
						, PHANDLE phRedirData			/*= NULL*/
						)
{
	ATLASSERT(pContext != NULL);
	m_pRaopContext = pContext;

	if (strSoundRedirection != NULL)
	{
		m_bRedirKeepAlive	= bRedirKeepAlive;
		m_hRedirProcess		= hRedirectionProcess;
		m_libao_hfile		= hRedirectionData;

		USES_CONVERSION;

		m_libao_file = "-";

		if (m_hRedirProcess == NULL)
		{
			m_hRedirProcess = Exec(strSoundRedirection, 0, true, &m_libao_hfile);
		}
		if (m_hRedirProcess == NULL)
		{
			Log("Sound Redirection Exec failed - got to be file output!\n");

			WTL::CString str(strSoundRedirection);

			str.MakeLower();

			m_libao_file = T2CA(str);

			if (m_libao_file.length() < 4 || m_libao_file.rfind(".au") == string::npos)
				m_libao_file += string(".au");

			FILE* fh = fopen(m_libao_file.c_str(), "wb");

			if (fh == NULL)
			{
				Log("Creating of output file %s failed with error code: %lu!\n", m_libao_file.c_str(), GetLastError());
				m_libao_file.clear();
			}
			else
				fclose(fh);
		}
		else if (m_bRedirKeepAlive && m_libao_hfile != INVALID_HANDLE_VALUE && phRedirData != NULL)
		{
			// duplicate the pipe handle
			if (!DuplicateHandle(GetCurrentProcess(), 
								m_libao_hfile, 
								GetCurrentProcess(),
								phRedirData, 
								0,
								FALSE,
								DUPLICATE_SAME_ACCESS))
			{
				Log("Sound Redirection DuplicateHandle failed - Code: 0x%lx\n", (ULONG)GetLastError());
				*phRedirData		= INVALID_HANDLE_VALUE;
				m_bRedirKeepAlive	= false;

				if (phRedirProcess != NULL)
					*phRedirProcess = NULL;
			}
			else if (phRedirProcess != NULL)
				*phRedirProcess = m_hRedirProcess;
		}
	}
	m_nControlport	= atoi(pContext->m_strCport.c_str());
	m_nTimingport	= atoi(pContext->m_strTport.c_str());

	m_AesIv.Reallocate( pContext->m_sizeAesiv);
	memcpy(m_AesIv, pContext->m_Aesiv,  pContext->m_sizeAesiv);

	m_AesKey.Reallocate(pContext->m_sizeRsaaeskey);
	memcpy(m_AesKey, pContext->m_Rsaaeskey, pContext->m_sizeRsaaeskey);

    AES_set_decrypt_key(m_AesKey, pContext->m_sizeRsaaeskey*8, &m_AES);

	m_mapFmtp.clear();

	char fmtpstr[512];
	strcpy(fmtpstr, pContext->m_strFmtp.c_str());

	char*	arg = strtok(fmtpstr, " \t");
	int		i	= 0;

	while (arg != NULL)
	{
		m_mapFmtp[i++] = atoi(arg);
		arg = strtok(NULL, " \t");
	}
	ATLASSERT(m_decoder_info == NULL);

    alac_file* alac;

    m_nFrameSize	= m_mapFmtp[1]; 
    m_nSamplingRate = m_mapFmtp[11];

    int sample_size = m_mapFmtp[3];

    if (sample_size != SAMPLE_SIZE)
	{
		Log("only 16-bit samples supported!");
		return false;
	}
    alac = create_alac(sample_size, NUM_CHANNELS);

    if (!alac)
        return false;

    alac->setinfo_max_samples_per_frame = m_nFrameSize;
    alac->setinfo_7a					= m_mapFmtp[2];
    alac->setinfo_sample_size			= sample_size;
    alac->setinfo_rice_historymult		= m_mapFmtp[4];
    alac->setinfo_rice_initialhistory	= m_mapFmtp[5];
    alac->setinfo_rice_kmodifier		= m_mapFmtp[6];
    alac->setinfo_7f					= m_mapFmtp[7];
    alac->setinfo_80					= m_mapFmtp[8];
    alac->setinfo_82					= m_mapFmtp[9];
    alac->setinfo_86					= m_mapFmtp[10];
    alac->setinfo_8a_rate				= m_mapFmtp[11];

    m_decoder_info = alac;

    allocate_buffers(alac);	

	m_bFlush = false;
	m_bPause = false;
	ResetEvent(m_hEvent);
	BF_EstReset();

	m_OutBuf.Reallocate(OUTFRAME_BYTES);

	if (!IsRunning())              
	{
		if (!CMyThread::Start())
		{
			Stop();
			return false;
		}
	}
	if (!CRtpEndpoint::CreateEndpoint("Control", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Control))
	{
		Stop();
		return false;
	}
	m_threadResend.m_nControlport		= m_nControlport;
	m_threadResend.m_RtpClient_Control	= m_RtpClient_Control;

#ifdef _HAIRTUNES_TCP
	if (!CRtpEndpoint::CreateEndpoint("Sender.Data", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Data, &m_RtpClient_Tcp_Data))
#else
	if (!CRtpEndpoint::CreateEndpoint("Sender.Data", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Data))
#endif
	{
		Stop();
		return false;
	}
	if (!CRtpEndpoint::CreateEndpoint("Sender.Timing", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Timing))
	{
		Stop();
		return false;
	}
#ifdef _WITH_PROXY
	if (!CRtpEndpoint::CreateEndpoint("Receiver.Control.Proxy", this,(PSOCKADDR_IN)& pContext->m_Peer, m_RtpClient_Proxy_Control))
	{
		Stop();
		return false;
	}
	if (!CRtpEndpoint::CreateEndpoint("Receiver.Timing.Proxy", this, (PSOCKADDR_IN)&pContext->m_Peer, m_RtpClient_Proxy_Timing))
	{
		Stop();
		return false;
	}
#endif

	return true;
}

USHORT CHairTunes::GetServerPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Data != NULL)
	{
		nResult = ntohs(m_RtpClient_Data->m_nPort);
	}
	return nResult;
}

USHORT CHairTunes::GetControlPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Control != NULL)
	{
		nResult = ntohs(m_RtpClient_Control->m_nPort);
	}
	return nResult;
}

USHORT CHairTunes::GetTimingPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Timing != NULL)
	{
		nResult = ntohs(m_RtpClient_Timing->m_nPort);
	}
	return nResult;
}

#ifdef _WITH_PROXY
USHORT CHairTunes::GetProxyControlPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Proxy_Control != NULL)
	{
		nResult = ntohs(m_RtpClient_Proxy_Control->m_nPort);
	}
	return nResult;
}

USHORT CHairTunes::GetProxyTimingPort()
{
	USHORT nResult = 0;

	if (m_RtpClient_Proxy_Timing != NULL)
	{
		nResult = ntohs(m_RtpClient_Proxy_Timing->m_nPort);
	}
	return nResult;
}
#endif

void CHairTunes::Stop()
{
#ifdef _WITH_PROXY
	if (m_RtpClient_Proxy_Control != NULL)
		m_RtpClient_Proxy_Control->Cancel();
	if (m_RtpClient_Proxy_Timing != NULL)
		m_RtpClient_Proxy_Timing->Cancel();
#endif

	if (m_RtpClient_Timing != NULL)
		m_RtpClient_Timing->Cancel();
	if (m_RtpClient_Control != NULL)
		m_RtpClient_Control->Cancel();
#ifdef _HAIRTUNES_TCP
	if (m_RtpClient_Tcp_Data != NULL)
		m_RtpClient_Tcp_Data->Cancel();
#endif
	if (m_RtpClient_Data != NULL)
		m_RtpClient_Data->Cancel();

#ifdef _WITH_PROXY
	m_RtpClient_Proxy_Control.reset();
	m_RtpClient_Proxy_Timing.reset();
#endif

#ifdef _HAIRTUNES_TCP
	m_RtpClient_Tcp_Data.reset();
#endif
	m_RtpClient_Data.reset();
	m_RtpClient_Control.reset();
	m_RtpClient_Timing.reset();

	CMyThread::Stop();

	if (m_decoder_info != NULL)
	{
		destroy_alac(m_decoder_info);
		m_decoder_info = NULL;
	}
	m_mtxQueue.Lock();
	m_Queue.clear();
	ResetEvent(m_hEvent);
	m_mtxQueue.Unlock();

	if (m_hRedirProcess != NULL && !m_bRedirKeepAlive)
	{
		TerminateRedirection(m_hRedirProcess);
	}
}

void CHairTunes::Flush()
{
	m_mtxQueue.Lock();
	
	while(!m_Queue.empty())
	{
		m_bFlush = true;
		m_bPause = false;
		SetEvent(m_hEvent);

		m_mtxQueue.Unlock();
		Sleep(10);
		m_mtxQueue.Lock();
	}
	m_bFlush = false;
	m_mtxQueue.Unlock();
}

void CHairTunes::SetPause(bool bValue)
{
	if (bValue != m_bPause)
	{
		m_mtxQueue.Lock();

		m_bPause = bValue;
		
		if (m_bPause)
			ResetEvent(m_hEvent);
		else
			SetEvent(m_hEvent);

		m_mtxQueue.Unlock();
	}
}

void CHairTunes::SetVolume(double lfValue)
{
	double lfVolume		= pow(10.0l, 0.05l * lfValue);
	long   nFixVolume	= (long)(65536.0l * lfVolume);

	InterlockedExchange(&m_nFixVolume, nFixVolume);
}

void CHairTunes::OnStop()
{
	if (m_ao_opts != NULL)
	{
		ao_free_options(m_ao_opts);
		m_ao_opts = NULL;
	}
	if (m_libao_dev != NULL)
	{
		ao_close(m_libao_dev);
		m_libao_dev = NULL;

		// note: m_libao_hfile will be closed by ao_close
		m_libao_hfile = INVALID_HANDLE_VALUE;
	}
	m_threadResend.Stop();
}

BOOL CHairTunes::OnStart() 
{
	if (!m_threadResend.Start())
		return FALSE;

	if (m_libao_dev == NULL)
	{
		ao_initialize();

		int	driver = ao_driver_id(m_libao_file.empty() && m_libao_hfile == INVALID_HANDLE_VALUE ? "wmm" : m_libao_hfile != INVALID_HANDLE_VALUE ? "raw" : "au");
			
		if (driver < 0) 
		{
			Log("Could not find requested ao driver");
			return FALSE;
		}

		ao_sample_format fmt;
		memset(&fmt, 0, sizeof(fmt));
	
		fmt.bits		= SAMPLE_SIZE;
		fmt.rate		= m_nSamplingRate;
		fmt.channels	= NUM_CHANNELS;
		fmt.byte_format = AO_FMT_NATIVE;

		if (!c_strlibao_deviceid.empty()) 
		{
			ao_append_option(&m_ao_opts, "id", c_strlibao_deviceid.c_str());
		}

		if (m_libao_file.empty() && m_libao_hfile == INVALID_HANDLE_VALUE)
		{
			m_bLiveOutput	= true;
			m_libao_dev		= ao_open_live(driver, &fmt, m_ao_opts);

			if (m_libao_dev == NULL) 
			{
				Log("Could not open ao device");
				return FALSE;
			}
		}
		else
		{
			m_bLiveOutput = false;

			if (m_libao_hfile != INVALID_HANDLE_VALUE)
			{
				m_libao_dev = ao_open_file2(driver, m_libao_hfile, &fmt, m_ao_opts);

				if (m_libao_dev == NULL) 
				{
					Log("Could not open ao stdout device");
					return FALSE;
				}
			}
			else
			{
				m_libao_dev = ao_open_file(driver, m_libao_file.c_str(), TRUE, &fmt, m_ao_opts);
				
				if (m_libao_dev == NULL) 
				{
					Log("Could not open ao file device");
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

void CHairTunes::QueuePacket(shared_ptr<CRtpPacket>& p)
{
	if (!m_bFlush && p->getDataLen() >= 16)
	{
		USHORT nCurSeq = p->getSeqNo();

		m_mtxQueue.Lock();

		if (!m_Queue.empty())
		{
			shared_ptr<CRtpPacket>& pBack = m_Queue.back();

			// check for lacking packets
			short nSeqDiff = nCurSeq - pBack->getSeqNo();

			switch(nSeqDiff)
			{
				case 0:
				{
					ATLTRACE("packet %lu already queued\n", (long)nCurSeq);
					ATLASSERT(FALSE);
				}
				break;

				case 1:
				{
					// expected sequence
					m_Queue.push_back(p);
					p.reset();
				}
				break;

				default:
				{
					if (nSeqDiff > 1)
					{
						if (p->getPayloadType() != PAYLOAD_TYPE_RESEND_RESPONSE)
							m_threadResend.Push(pBack->getSeqNo()+1, nSeqDiff-1);
						m_Queue.push_back(p);
						p.reset();
					}
					else
					{
						ATLASSERT(nSeqDiff < 0);

						for (auto i = m_Queue.begin(); i != m_Queue.end(); ++i)
						{
							nSeqDiff = nCurSeq - i->get()->getSeqNo();

							if (nSeqDiff == 0)
							{
								//ATLTRACE("late packet %lu already queued\n", (long)nCurSeq);
								break;
							}
							if (nSeqDiff < 0)
							{
								if (i != m_Queue.begin())
								{
									m_Queue.insert(i, p);
									p.reset();
#ifdef _DEBUG
									USHORT s3 = i->get()->getSeqNo();
									USHORT s2 = (--i)->get()->getSeqNo();
									USHORT s1 = i == m_Queue.begin() ? i->get()->getSeqNo()-1 : (--i)->get()->getSeqNo();

									//ATLTRACE("inserted packet %lu > %lu > %lu\n", (long) s1, (long) s2, (long) s3);
#endif
								}
								else
								{
									//ATLTRACE("packet %lu is too late and won't be inserted at beginning of the queue\n", (long) nCurSeq);
								}
								break;
							}
							ATLASSERT(nSeqDiff > 0);
						}
					}
				}
				break;
			}
		}
		else
		{
			// expected sequence (initial packet)
			m_Queue.push_back(p);
			p.reset();
		}
		if (!m_bPause && (LONG)m_Queue.size() >= c_nStartFill)
		{
			SetEvent(m_hEvent);
		}
		m_mtxQueue.Unlock();
	}
}

void CHairTunes::OnRequest(CRtpEndpoint* pRtpEndpoint, shared_ptr<CRtpPacket>& packet)
{
	byte type = packet->getPayloadType();

#ifdef _WITH_PROXY
	if (pRtpEndpoint == m_RtpClient_Proxy_Control.get())
	{
		ATLTRACE("Forwarding %s to Control Port (Airport Sender)\n", CRtpPacket::GetPayloadName(type));
		m_RtpClient_Control->SendTo(packet->m_Data, packet->m_nLen, m_nControlport);
		return;
	}
	else if (pRtpEndpoint == m_RtpClient_Proxy_Timing.get())
	{
		ATLTRACE("Forwarding %s to Timing Port (Airport Sender)\n", CRtpPacket::GetPayloadName(type));

		switch(type)
		{
			case PAYLOAD_TYPE_TIMING_RESPONSE:
			case PAYLOAD_TYPE_TIMING_REQUEST:
			{
				ATLASSERT(packet->m_nLen == 32);
				ATLTRACE("ReferenceTime: %I64x, ReceivedTime: %I64x, SendTime: %I64x\n", packet->getReferenceTime(), packet->getReceivedTime(), packet->getSendTime());
			}
			break;
		}
		m_RtpClient_Timing->SendTo(packet->m_Data, packet->m_nLen, m_nTimingport);
		return;
	}
	shared_ptr<CRtpPacket> pCopy = make_shared<CRtpPacket>(packet->m_Data, packet->m_nLen);
#endif

	switch (type)
	{
		case PAYLOAD_TYPE_RESEND_RESPONSE:
		{
#ifdef _WITH_PROXY
			pCopy.reset();
#endif
			// shift data 4 bytes left
			memmove(packet->m_Data, packet->m_Data + RTP_BASE_HEADER_SIZE, packet->m_nLen - RTP_BASE_HEADER_SIZE);
			packet->m_nLen -= RTP_BASE_HEADER_SIZE;

			if (packet->getSeqNo() == 0 && packet->getDataLen() < 16)
			{
				// resync?
				Flush();
			}
			else if (m_threadResend.Pull(packet->getSeqNo()))
				QueuePacket(packet);
		}
		break;

		case PAYLOAD_TYPE_STREAM_DATA:
		{
			// ATLTRACE("PAYLOAD_TYPE_STREAM_DATA (Ext: %s) - SSRC: %d, TimeStamp: %d, ReferenceTime: %I64x, DataLen: %d\n", packet->getExtension() ? "yes" : "no", packet->getSSRC(), packet->getTimeStamp(), packet->getReferenceTime(), packet->getDataLen());
			QueuePacket(packet);
		}
		break;

		case PAYLOAD_TYPE_TIMING_RESPONSE:
		case PAYLOAD_TYPE_TIMING_REQUEST:
		{
			ATLASSERT(packet->m_nLen == 32);
			// ATLTRACE("%s - ReferenceTime: %I64x, ReceivedTime: %I64x, SendTime: %I64x\n", CRtpPacket::GetPayloadName(type), packet->getReferenceTime(), packet->getReceivedTime(), packet->getSendTime());
		}
		break;

		case PAYLOAD_TYPE_STREAM_SYNC:
		{
			// ATLTRACE("PAYLOAD_TYPE_STREAM_SYNC (Ext: %s) - SSRC: %d, TimeStamp: %d, ReferenceTime: %I64x, DataLen: %d\n", packet->getExtension() ? "yes" : "no", packet->getSSRC(), packet->getTimeStamp(), packet->getReferenceTime(), packet->getDataLen());
		}
		break;

		default:
		{
			// ATLTRACE("PayloadType: %s (0x%lx)\n", CRtpPacket::GetPayloadName(type), (ULONG)type);
			// todo: implement - DebugBreak();
		}
		break;
	}
	PutPacketToPool(packet);

#ifdef _WITH_PROXY
	if (pCopy != NULL)
	{
		CRtpEndpoint*	pSendFrom	= NULL;
		int				nProxyPort	= 0;

		if		(pRtpEndpoint == m_RtpClient_Data.get())
		{
			pSendFrom = m_RtpClient_Data.get();
			nProxyPort = m_Proxy_serverport;
		}
		else if (pRtpEndpoint == m_RtpClient_Control.get())
		{
			ATLTRACE("Forwarding to Control Port (Airport Receiver)\n");
			pSendFrom = m_RtpClient_Proxy_Control.get();
			nProxyPort = m_Proxy_controlport;
		}
		else if (pRtpEndpoint == m_RtpClient_Timing.get())
		{
			ATLTRACE("Forwarding to Timing Port (Airport Receiver)\n");
			pSendFrom = m_RtpClient_Proxy_Timing.get();
			nProxyPort = m_Proxy_timingport;
		}
		if (pSendFrom != NULL)
		{
			((SOCKADDR_IN*)&m_rtp_proxy_server)->sin_port = nProxyPort;
			((CNetworkServer*)pSendFrom)->SendTo(pCopy->m_Data, pCopy->m_nLen, (struct sockaddr *)&m_rtp_proxy_server, m_rtp_proxy_server_size);
		}
	}
#endif
}

void CHairTunes::OnEvent()
{
	shared_ptr<CRtpPacket>	packet;
	int						nBufFill = 0;

	m_mtxQueue.Lock();

	nBufFill = m_Queue.size();

	if (nBufFill == 0)
		ResetEvent(m_hEvent);
	else
	{
		packet = m_Queue.front();
		m_Queue.pop_front();

		// removing later packets from resend queue
		m_threadResend.Remove(packet->getSeqNo());
	}
	m_mtxQueue.Unlock();

	if (nBufFill == 0)
		BF_EstReset();
	else
		BF_EstUpdate(nBufFill);

	if (packet != NULL)
	{
		if (!m_bFlush)
		{
			ULONGLONG nTimeStamp = MAKEULONGLONG(packet->getTimeStamp(), 0);

			packet->m_nLen = AlacDecode(packet->m_Data, packet->getData(), packet->getDataLen());

			if (packet->m_nLen >= 4 && packet->m_nLen <= FRAME_BYTES)
			{
				if (!g_bMute || !m_bLiveOutput)
				{
					int nPlaySamples = StuffBuffer((const short*)(BYTE*)packet->m_Data, (short*)((BYTE*)m_OutBuf), packet->m_nLen >> NUM_CHANNELS, m_bf_playback_rate);

					ao_play(m_libao_dev, m_OutBuf, nPlaySamples << NUM_CHANNELS);
				}
			}
		}
		PutPacketToPool(packet);
	}
}

bool CHairTunes::_CResendThread::_RequestResend(USHORT nSeq, short nCount)
{
	//ATLTRACE("requesting resend of %d packets from seq %lu on remote port %lu\n", (long)nCount, (long)nSeq, (long)m_nControlport);

	// *not* a standard RTCP NACK
	unsigned char req[8] = { 0 };						

	// Apple 'resend'
	req[0]						= 0x80;
	req[1]						= PAYLOAD_TYPE_RESEND_REQUEST|0x80;	

	// our seqnum
	*(unsigned short *)(req+2)	= htons(1);		
	// missed seqnum
	*(unsigned short *)(req+4)	= htons(nSeq); 
	// count
	*(unsigned short *)(req+6)	= htons(nCount);  

	if (!m_RtpClient_Control->SendTo(req, sizeof(req), m_nControlport))
	{
		//ATLTRACE("requesting resend of %d packets from seq %lu on remote port %lu *failed* !!!\n", (long)nCount, (long)nSeq, (long)m_nControlport);
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////
// CPlugInManager

bool CPlugInManager::wave_open(int nSamplingRate, HANDLE hEventPlay)
{
	if (m_libao_dev == NULL)
	{
		ao_initialize();

		int	driver = ao_driver_id("wmm");
			
		if (driver < 0) 
		{
			Log("Could not find requested ao driver");
			return false;
		}

		ao_sample_format fmt;
		memset(&fmt, 0, sizeof(fmt));
	
		fmt.bits		= SAMPLE_SIZE;
		fmt.rate		= nSamplingRate;
		fmt.channels	= NUM_CHANNELS;
		fmt.byte_format = AO_FMT_NATIVE;
		fmt.hPlay		= hEventPlay;
		m_libao_dev		= ao_open_live(driver, &fmt, NULL);	
	}
	return m_libao_dev != NULL ? true : false;
}

// returns the remaining bytes to play
int CPlugInManager::wave_play(shared_ptr<CRaopContext> pRaopContext, const unsigned char* output_samples, unsigned int num_bytes, double bf_playback_rate)
{
	if (m_libao_dev != NULL)
	{
		/*if (pRaopContext != NULL)
		{
			if (pRaopContext->m_pDecoder == NULL)
			{
				pRaopContext->m_pDecoder = make_shared<CHairTunes>();
			}
			if (num_bytes > m_nBuf)
			{
				ATLVERIFY(m_OutBuf.Reallocate(num_bytes) != NULL);
				m_nBuf = num_bytes;
			}
			int nPlaySamples = pRaopContext->m_pDecoder->StuffBuffer((const short*)(BYTE*)output_samples, (short*)((BYTE*)m_OutBuf), num_bytes >> NUM_CHANNELS, bf_playback_rate);

			return ao_play(m_libao_dev, m_OutBuf, nPlaySamples << NUM_CHANNELS);
		}
		else*/
			return ao_play(m_libao_dev, output_samples, num_bytes);
	}
	return 0;
}

void CPlugInManager::wave_close()
{
	if (m_libao_dev != NULL)
	{
		ao_close(m_libao_dev);
		m_libao_dev = NULL;
	}
}
