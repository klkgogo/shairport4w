/*
 *
 *  Shairport4w.cpp
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

#include "stdafx.h"

#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "Bonjour/dns_sd.h"

#include "DmapParser.h"

typedef EXECUTION_STATE (WINAPI *_SetThreadExecutionState)(EXECUTION_STATE esFlags);
_SetThreadExecutionState	pSetThreadExecutionState = NULL;

static	map<SOCKET, shared_ptr<CRaopContext> >	c_mapConnection;
static 	CMyMutex								c_mtxConnection;
		CPlugInManager							g_PlugInManager;

CAppModule		_Module;
CMutex			mtxAppSessionInstance;
CMutex			mtxAppGlobalInstance;
CMainDlg		dlgMain;
BYTE			hw_addr[6];
CMyMutex		mtxRSA;
RSA*			pRSA			= NULL;
BOOL&			bLogToFile(dlgMain.m_bLogToFile);
string			strPrivateKey	=	"MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt"
									"wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U"
									"wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf"
									"/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/"
									"UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW"
									"BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa"
									"LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5"
									"NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm"
									"lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz"
									"aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu"
									"a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM"
									"oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z"
									"oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+"
									"k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL"
									"AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA"
									"cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf"
									"54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov"
									"17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc"
									"1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI"
									"LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ"
									"2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=";

static bool digest_ok(shared_ptr<CRaopContext>& pRaopContext)
{
	if (!pRaopContext->m_strNonce.empty())
	{
		string& strAuth = pRaopContext->m_CurrentHttpRequest.m_mapHeader["Authorization"];

		if (strAuth.find("Digest ") == 0)
		{
			USES_CONVERSION;

			map<ci_string, string> mapAuth;
			ParseRegEx(strAuth.c_str()+7, "{\\a+}=\"{[^\"]+}\"", mapAuth, FALSE);

			string str1 = mapAuth["username"] + string(":") + mapAuth["realm"] + string(":") + string(T2CA(dlgMain.m_strPassword));
			string str2 = pRaopContext->m_CurrentHttpRequest.m_strMethod + string(":") + mapAuth["uri"];

			string strDigest1 = MD5_Hex((const BYTE*)str1.c_str(), str1.length(), true);
			string strDigest2 = MD5_Hex((const BYTE*)str2.c_str(), str2.length(), true);

			string strDigest = strDigest1 + string(":") + mapAuth["nonce"] + string(":") + strDigest2;

			strDigest = MD5_Hex((const BYTE*)strDigest.c_str(), strDigest.length(), true);

			Log("Password Digest: \"%s\" == \"%s\"\n", strDigest.c_str(), mapAuth["response"].c_str());

			return strDigest == mapAuth["response"];
		}
	}
	return false;
}

class CRaopServer : public CNetworkServer, protected CDmapParser
{
protected:
	// Dmap Parser Callbacks
	virtual void on_string(void* ctx, const char *code, const char *name, const char *buf, size_t len)  
	{
		CRaopContext* pRaopContext = (CRaopContext*)ctx;
		ATLASSERT(pRaopContext != NULL);
		
		if (stricmp(name, "daap.songalbum") == 0)
		{
			WTL::CString strValue;

			LPTSTR p = strValue.GetBuffer(len*2);

			if (p != NULL)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len*2);
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->Lock();
			pRaopContext->m_str_daap_songalbum = strValue;
			pRaopContext->Unlock();
		}
		else if (stricmp(name, "daap.songartist") == 0)
		{
			WTL::CString strValue;

			LPTSTR p = strValue.GetBuffer(len*2);

			if (p != NULL)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len*2);
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->Lock();
			pRaopContext->m_str_daap_songartist = strValue;
			pRaopContext->Unlock();
		}
		else if (stricmp(name, "dmap.itemname") == 0)
		{
			WTL::CString strValue;

			LPTSTR p = strValue.GetBuffer(len*2);

			if (p != NULL)
			{
				int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len*2);
				strValue.ReleaseBuffer((n >= 0) ? n : -1);
			}
			pRaopContext->Lock();
			pRaopContext->m_str_daap_trackname = strValue;
			pRaopContext->Unlock();
		}
	}
	// Network Callbacks
	virtual bool OnAccept(SOCKET sd)
	{
		string strPeerIP;
		string strLocalIP;

		GetLocalIP(sd, m_bV4, strLocalIP);
		GetPeerIP(sd, m_bV4, strPeerIP);
		Log("Accepted new client %s\n", strPeerIP.c_str());

		shared_ptr<CRaopContext> pRaopContext = make_shared<CRaopContext>(m_bV4, strPeerIP.c_str(), strLocalIP.c_str());

		if (pRaopContext != NULL)
		{
			pRaopContext->m_pDecoder = make_shared<CHairTunes>();
			ATLASSERT(pRaopContext->m_pDecoder != NULL);			

			c_mtxConnection.Lock();

			c_mapConnection.erase(sd);

			c_mapConnection[sd] = pRaopContext;

			GetPeerIP(sd, m_bV4, (struct sockaddr_in*)&c_mapConnection[sd]->m_Peer);

#ifdef _WITH_PROXY
			int nProxy = dlgMain.m_ctlProxy.GetCurSel();

			if (nProxy != CB_ERR)
			{
				CRegisterServiceEvent* pItemData = (CRegisterServiceEvent*)dlgMain.m_ctlProxy.GetItemData(nProxy);

				int nAF = m_bV4 ? AF_INET : AF_INET6;

				if (!c_mapConnection[sd]->m_Proxy.Connect(pItemData->m_strHost.c_str(), pItemData->m_nPort, &nAF, (SOCKADDR_IN *)&c_mapConnection[sd]->m_pDecoder->m_rtp_proxy_server, &c_mapConnection[sd]->m_pDecoder->m_rtp_proxy_server_size))
				{
					Log("Connect to Proxy %s : %lu failed\n", pItemData->m_strHost.c_str(), (ULONG)ntohs(pItemData->m_nPort));
					c_mapConnection[sd]->m_Proxy.Close();
				}
			}
#endif
			c_mtxConnection.Unlock();

			return true;
		}
		return false;
	}
	virtual void OnRequest(SOCKET sd)
	{
		CTempBuffer<char> buf(0x100000);

		memset(buf, 0, 0x100000);

		CSocketBase sock;

		sock.Attach(sd);

		int nReceived = sock.recv(buf, 0x100000);

		if (nReceived < 0)
		{
			sock.Detach();
			return;
		}
		ATLASSERT(nReceived > 0);

		c_mtxConnection.Lock();

		auto pRaopContext = c_mapConnection[sd];

		c_mtxConnection.Unlock();

		CHttp&	request	= pRaopContext->m_CurrentHttpRequest;

		request.m_strBuf += string(buf, nReceived);

		if (request.Parse())
		{
			if (request.m_mapHeader.find("content-length") != request.m_mapHeader.end())
			{
				if (atol(request.m_mapHeader["content-length"].c_str()) > (long)request.m_strBody.length())
				{
					// need more
					sock.Detach();
					return;
				}
			}

			Log("+++++++ Http Request +++++++\n%s------- Http Request -------\n", request.m_strBuf.c_str());

#ifdef _WITH_PROXY
			if (pRaopContext->m_Proxy.IsValid())
			{
				// patch Url to this host
				if (pRaopContext->m_strPeerIP != pRaopContext->m_strLocalIP)
				{
					int nHostIdx = request.m_strUrl.find(pRaopContext->m_strPeerIP);

					if (nHostIdx > 0)
					{
						request.m_strUrl =		request.m_strUrl.substr(0, nHostIdx) 
											+	pRaopContext->m_strLocalIP 
											+	request.m_strUrl.substr(nHostIdx+pRaopContext->m_strPeerIP.length());
					}
				}
				if (	stricmp(request.m_strMethod.c_str(), "SETUP") != 0
					&&	stricmp(request.m_strMethod.c_str(), "ANNOUNCE") != 0
					)
				{
					string strProxyRequest = request.GetAsString(false);
				
					pRaopContext->m_Proxy.Send(strProxyRequest.c_str(), strProxyRequest.length());

					if (pRaopContext->m_Proxy.WaitForIncomingData(5000))
					{
						int		nProxyReceived		= pRaopContext->m_Proxy.recv(buf, 0x100000);
						string	strProxyResponse	= string(buf, nProxyReceived);
						Log("+++++++ Proxy +++++++\n%s------- Proxy -------\n", strProxyResponse.c_str());
					}
				}
			}
#endif
			CHttp response;

			response.Create(request.m_strProtocol.c_str());

			response.m_mapHeader["CSeq"]				= request.m_mapHeader["CSeq"];
			response.m_mapHeader["Audio-Jack-Status"]	= "connected; type=analog";
			response.m_mapHeader["Server"]				= "AirTunes/105.1";

			if (request.m_mapHeader.find("Apple-Challenge") != request.m_mapHeader.end())
			{
				string strChallenge = request.m_mapHeader["Apple-Challenge"];
				
				CTempBuffer<BYTE>	pChallenge;
				long				sizeChallenge	= 0;

				if ((sizeChallenge = DecodeBase64(strChallenge, pChallenge)))
				{
					CTempBuffer<BYTE>	pIP;
					int					sizeIP = GetLocalIP(sd, m_bV4, pIP);

					BYTE pAppleResponse[64];
					long sizeAppleResponse = 0;

					memset(pAppleResponse, 0, sizeof(pAppleResponse));

					memcpy(pAppleResponse+sizeAppleResponse, pChallenge, sizeChallenge);
					sizeAppleResponse += sizeChallenge;
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));
					memcpy(pAppleResponse+sizeAppleResponse, pIP, sizeIP);
					sizeAppleResponse += sizeIP;
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));
					memcpy(pAppleResponse+sizeAppleResponse, hw_addr, sizeof(hw_addr));
					sizeAppleResponse += sizeof(hw_addr);
					ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));

					sizeAppleResponse = max(32, sizeAppleResponse);

					LPBYTE pSignature		= new BYTE[2048];
					size_t sizeSignature	= 0;

					mtxRSA.Lock();
					sizeSignature = RSA_private_encrypt(sizeAppleResponse, pAppleResponse, pSignature, pRSA, RSA_PKCS1_PADDING);
					mtxRSA.Unlock();

					ATLASSERT(sizeSignature > 0);

					string strAppleResponse;
					EncodeBase64(pSignature, sizeSignature, strAppleResponse, false);

					TrimRight(strAppleResponse, "=\r\n");

					response.m_mapHeader["Apple-Response"] = strAppleResponse;

					delete [] pSignature;
				}
			}
			if (dlgMain.m_strPassword.GetLength() > 0)
			{
				if (!digest_ok(pRaopContext))
				{
					USES_CONVERSION;

					BYTE nonce[20];

					for (long i = 0; i < 20; ++i)
					{
						nonce[i] = (256 * rand()) / RAND_MAX;
					}
					pRaopContext->m_strNonce = MD5_Hex(nonce, 20, true);

					response.m_mapHeader["WWW-Authenticate"] =		string("Digest realm=\"") 
																+	string(T2CA(dlgMain.m_strApName))
																+	string("\", nonce=\"")
																+	pRaopContext->m_strNonce
																+	string("\"");
					response.SetStatus(401, "Unauthorized");
					request.m_strMethod = "DENIED";
				}
		    }
			if (stricmp(request.m_strMethod.c_str(), "OPTIONS") == 0)
			{
				response.m_mapHeader["Public"] = "ANNOUNCE, SETUP, RECORD, PAUSE, FLUSH, TEARDOWN, OPTIONS, GET_PARAMETER, SET_PARAMETER, POST, GET";
			}
			else if (stricmp(request.m_strMethod.c_str(), "ANNOUNCE") == 0)
			{
				USES_CONVERSION;

				map<ci_string, string>	mapKeyValue;
				ParseRegEx(request.m_strBody.c_str(), "a={[^:]+}:{[^\r^\n]+}", mapKeyValue, FALSE);

				pRaopContext->Lock();

				pRaopContext->Announce();

				pRaopContext->m_strFmtp			= mapKeyValue["fmtp"];
				pRaopContext->m_sizeAesiv		= DecodeBase64(mapKeyValue["aesiv"], pRaopContext->m_Aesiv);
				pRaopContext->m_sizeRsaaeskey	= DecodeBase64(mapKeyValue["rsaaeskey"], pRaopContext->m_Rsaaeskey);

				pRaopContext->m_strUserAgent	= A2CW_CP(request.m_mapHeader["User-Agent"].c_str(), CP_UTF8);
				int nPar = pRaopContext->m_strUserAgent.Find(L'(');

				if (nPar > 0)
					pRaopContext->m_strUserAgent = pRaopContext->m_strUserAgent.Left(nPar);
				pRaopContext->m_strUserAgent.TrimRight(L"\r\n \t");

				ParseRegEx(pRaopContext->m_strFmtp.c_str(), "{\\z}", pRaopContext->m_listFmtp, FALSE);
				
				if (pRaopContext->m_listFmtp.size() >= 12)
				{
					auto iFmtp = pRaopContext->m_listFmtp.begin();
					advance(iFmtp, 11);

					pRaopContext->m_lfFreq = atof(iFmtp->c_str());
				}
				pRaopContext->Unlock();

				mtxRSA.Lock();
				pRaopContext->m_sizeRsaaeskey = RSA_private_decrypt(pRaopContext->m_sizeRsaaeskey, pRaopContext->m_Rsaaeskey, pRaopContext->m_Rsaaeskey, pRSA, RSA_PKCS1_OAEP_PADDING);
				mtxRSA.Unlock();

#ifdef _WITH_PROXY
				if (pRaopContext->m_Proxy.IsValid())
				{
					// patch IP in body to this host
					CAtlStringA strBody = request.m_strBody.c_str();

					strBody.Replace(pRaopContext->m_strPeerIP.c_str(), pRaopContext->m_strLocalIP.c_str());

					request.m_strBody = (LPCSTR)strBody;

					string strProxyRequest = request.GetAsString(false);
				
					pRaopContext->m_Proxy.Send(strProxyRequest.c_str(), strProxyRequest.length());

					if (pRaopContext->m_Proxy.WaitForIncomingData(5000))
					{
						int		nProxyReceived		= pRaopContext->m_Proxy.recv(buf, 0x100000);
						string	strProxyResponse	= string(buf, nProxyReceived);
						Log("+++++++ Proxy +++++++\n%s------- Proxy -------\n", strProxyResponse.c_str());
					}
				}
#endif	
			}
			else if (stricmp(request.m_strMethod.c_str(), "SETUP") == 0)
			{
				string strTransport = request.m_mapHeader["transport"];

				ParseRegEx(strTransport.c_str(), "control_port={\\z}", pRaopContext->m_strCport, FALSE);
				ParseRegEx(strTransport.c_str(), "timing_port={\\z}", pRaopContext->m_strTport, FALSE);
				
				response.m_mapHeader["Session"] = "DEADBEEF";

				// kick currently playing stream
				if (dlgMain.GetMMState() != CMainDlg::pause)
				{
					// give playing client a chance to disconnect itself
					if (dlgMain.OnPause())
					{
						long nTry = 12;

						do
						{ 
							Sleep(12);
						}
						while(dlgMain.GetMMState() != CMainDlg::pause && --nTry > 0);
					}
				}
				c_mtxConnection.Lock();

				for (auto i = c_mapConnection.begin(); i != c_mapConnection.end(); ++i)
				{
					if (sd != i->first && i->second->m_bIsStreaming)
					{
						i->second->m_pDecoder->Stop();

						if (pSetThreadExecutionState != NULL)
							pSetThreadExecutionState(ES_CONTINUOUS);
						i->second->m_bIsStreaming = false;
						break;
					}
				}
				pRaopContext->m_bIsStreaming = true;

				if (request.m_mapHeader.find("DACP-ID") != request.m_mapHeader.end())
				{
					dlgMain.SetDacpID(ci_string("iTunes_Ctrl_") + ci_string(request.m_mapHeader["DACP-ID"].c_str()), request.m_mapHeader["Active-Remote"].c_str());
				}
				dlgMain.SendMessage(WM_RAOP_CTX, 0, (LPARAM)&pRaopContext);

				WTL::CString strNoSoundRedirection;
				ATLVERIFY(strNoSoundRedirection.LoadString(IDS_NO_REDIRECTION));

				bool bSoundRedirection = false;

				if (!dlgMain.m_strSoundRedirection.IsEmpty() &&	dlgMain.m_bSoundRedirection)
				{
					Log("Sound Redirection is: %ls\n", T2CW(dlgMain.m_strSoundRedirection));
					bSoundRedirection = true;
				}
				if (pRaopContext->m_pDecoder->Start(pRaopContext
													, bSoundRedirection ? (LPCTSTR)dlgMain.m_strSoundRedirection : NULL
													, dlgMain.m_bRedirKeepAlive
													, dlgMain.m_hRedirProcess
													, dlgMain.m_hRedirData
													, &dlgMain.m_hRedirProcess
													, &dlgMain.m_hRedirData 
													)
					)
				{
					if (pSetThreadExecutionState != NULL)
						pSetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

					char strTransport[256];

					long nServerPort	= pRaopContext->m_pDecoder->GetServerPort();
					long nControlPort	= pRaopContext->m_pDecoder->GetControlPort();
					long nTimingPort	= pRaopContext->m_pDecoder->GetTimingPort();

					sprintf_s(strTransport, 256, "RTP/AVP/UDP;unicast;mode=record;server_port=%ld;control_port=%ld;timing_port=%ld", nServerPort, nControlPort, nTimingPort);

					response.m_mapHeader["Transport"] = strTransport;

#ifdef _WITH_PROXY
					if (pRaopContext->m_Proxy.IsValid())
					{
						nControlPort	= pRaopContext->m_pDecoder->GetProxyControlPort();
						nTimingPort		= pRaopContext->m_pDecoder->GetProxyTimingPort();
						sprintf_s(strTransport, 256, "RTP/AVP/UDP;unicast;mode=record;timing_port=%ld;control_port=%ld", nTimingPort, nControlPort);

						request.m_mapHeader["Transport"] = strTransport;

						string strProxyRequest = request.GetAsString(false);
				
						pRaopContext->m_Proxy.Send(strProxyRequest.c_str(), strProxyRequest.length());

						if (pRaopContext->m_Proxy.WaitForIncomingData(5000))
						{
							int		nProxyReceived		= pRaopContext->m_Proxy.recv(buf, 0x100000);
							string	strProxyResponse	= string(buf, nProxyReceived);
							Log("+++++++ Proxy +++++++\n%s------- Proxy -------\n", strProxyResponse.c_str());
								
							string strTok;
							ParseRegEx(strProxyResponse.c_str(), "control_port={\\z}", strTok, FALSE);
							pRaopContext->m_pDecoder->m_Proxy_controlport = htons((USHORT)atol(strTok.c_str()));
							ParseRegEx(strProxyResponse.c_str(), "timing_port={\\z}", strTok, FALSE);
							pRaopContext->m_pDecoder->m_Proxy_timingport = htons((USHORT)atol(strTok.c_str()));
							ParseRegEx(strProxyResponse.c_str(), "server_port={\\z}", strTok, FALSE);
							pRaopContext->m_pDecoder->m_Proxy_serverport = htons((USHORT)atol(strTok.c_str()));
						}
					}
					else
					{
						pRaopContext->m_pDecoder->m_Proxy_controlport = -1;
						pRaopContext->m_pDecoder->m_Proxy_timingport	= -1;
						pRaopContext->m_pDecoder->m_Proxy_serverport	= -1;
					}
#endif
				}
				else
				{
					pRaopContext->m_bIsStreaming = false;
					response.SetStatus(503, "Service Unavailable");
				}
				c_mtxConnection.Unlock();
			}
			else if (stricmp(request.m_strMethod.c_str(), "FLUSH") == 0)
			{
				response.m_mapHeader["RTP-Info"] = "rtptime=0";

				if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
					dlgMain.PostMessage(WM_PAUSE, 1);
				
				if (pRaopContext->m_bIsStreaming)
					pRaopContext->m_pDecoder->Flush();
			}
			else if (stricmp(request.m_strMethod.c_str(), "TEARDOWN") == 0)
			{
				if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
					dlgMain.PostMessage(WM_STOP);

				if (pRaopContext->m_bIsStreaming)
				{
					pRaopContext->m_pDecoder->Stop();
					pRaopContext->m_bIsStreaming = false;

					if (pSetThreadExecutionState != NULL)
						pSetThreadExecutionState(ES_CONTINUOUS);
				}
				pRaopContext->Fire(L"Teardown");

				response.m_mapHeader["Connection"] = "close";
			}
			else if (stricmp(request.m_strMethod.c_str(), "RECORD") == 0)
			{
				dlgMain.PostMessage(WM_RESUME);
				response.m_mapHeader["Audio-Latency"] = "6174";
			}
			else if (stricmp(request.m_strMethod.c_str(), "SET_PARAMETER") == 0)
			{
				auto ct = request.m_mapHeader.find("content-type");

				if (ct != request.m_mapHeader.end())
				{
					if (stricmp(ct->second.c_str(), "application/x-dmap-tagged") == 0)
					{
						if (!request.m_strBody.empty())
						{
							pRaopContext->Lock();
							pRaopContext->ResetSongInfos();
							pRaopContext->Unlock();

							dmap_parse(pRaopContext.get(), request.m_strBody.c_str(), request.m_strBody.length());

							if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
								dlgMain.PostMessage(WM_SONG_INFO);
						}
					}
					else if (ct->second.length() > 6 && strnicmp(ct->second.c_str(), "image/", 6) == 0)
					{
						pRaopContext->Lock();

						pRaopContext->DeleteImage();

						if (stricmp(ct->second.c_str()+6, "none") == 0)
						{
							// do nothing
						}
						else if (stricmp(ct->second.c_str()+6, "jpeg") == 0 || stricmp(ct->second.c_str()+6, "png") == 0)
						{
							pRaopContext->m_nBitmapBytes = 0;

							long nBodyLength = (long) request.m_strBody.length();

							if (nBodyLength)
							{
								if (pRaopContext->m_binBitmap.Reallocate(nBodyLength))
								{
									memcpy(pRaopContext->m_binBitmap, request.m_strBody.c_str(), nBodyLength);
									pRaopContext->m_nBitmapBytes = nBodyLength;

									pRaopContext->m_bmInfo = BitmapFromBlob(pRaopContext->m_binBitmap, pRaopContext->m_nBitmapBytes);
								}
							}
						}
						else
						{
							Log("Unknown Imagetype: %s\n", ct->second.c_str());
						}
						pRaopContext->Unlock();

						if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
						{
							dlgMain.PostMessage(WM_INFO_BMP);
							dlgMain.PostMessage(WM_SONG_INFO);
						}
					}
					else if (stricmp(ct->second.c_str(), "text/parameters") == 0)
					{
						if (!request.m_strBody.empty())
						{
							map<ci_string, string>	mapKeyValue;
							ParseRegEx(request.m_strBody.c_str(), "{[^:^\r^\n]+}:{[^\r^\n]+}", mapKeyValue, FALSE);

							if (!mapKeyValue.empty())
							{
								auto i = mapKeyValue.find("volume");

								if (i != mapKeyValue.end())
								{
									pRaopContext->m_pDecoder->SetVolume(atof(i->second.c_str()));
								}
								else
								{
									i = mapKeyValue.find("progress");

									if (i != mapKeyValue.end())
									{
										list<string> listProgressValues;

										ParseRegEx(i->second.c_str(), "{\\z}/{\\z}/{\\z}", listProgressValues, FALSE);

										if (listProgressValues.size() == 3)
										{
											auto tsi = listProgressValues.begin();

											double start	= atof(tsi->c_str());
											double curr		= atof((++tsi)->c_str());
											double end		= atof((++tsi)->c_str());

											if (end >= start)
											{
												// obviously set wrongly by some Apps (e.g. Soundcloud) 
												if (start > curr)
												{
													double _curr = curr;
													curr	= start;
													start	= _curr;
												}
												time_t nTotalSecs = (time_t)((end-start) / pRaopContext->m_lfFreq);

												pRaopContext->Lock();

												pRaopContext->m_rtpStart		= MAKEULONGLONG((ULONG)start, 0);
												pRaopContext->m_rtpCur			= MAKEULONGLONG((ULONG)curr, 0);
												pRaopContext->m_rtpEnd			= MAKEULONGLONG((ULONG)end, 0);
												pRaopContext->m_nTimeStamp		= ::time(NULL);
												pRaopContext->m_nTimeTotal		= nTotalSecs;

												if (curr > start)
													pRaopContext->m_nTimeCurrentPos	= (time_t)((curr-start) / pRaopContext->m_lfFreq); 
												else
													pRaopContext->m_nTimeCurrentPos	= 0;

												pRaopContext->m_nDurHours = (long)(nTotalSecs / 3600);
												nTotalSecs -= ((time_t)pRaopContext->m_nDurHours*3600);

												pRaopContext->m_nDurMins = (long)(nTotalSecs / 60);
												pRaopContext->m_nDurSecs = (long)(nTotalSecs % 60);

												pRaopContext->Unlock();

												pRaopContext->Fire(L"Progress");

												if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
													dlgMain.PostMessage(WM_RESUME, 1);
											}
										}
										else
										{
											Log("Unexpected Value of Progress Parameter: %s\n", i->second.c_str());
										}
									}
									else
									{
										Log("Unknown textual parameter: %s\n", request.m_strBody.c_str());
									}
								}
							}
							else
							{
								Log("No Key Values in textual parameter: %s\n", request.m_strBody.c_str());
							}
						}
					}
					else
					{
						Log("Unknown Parameter Content Type: %s\n", ct->second.c_str());
					}
				}
			}
			string strResponse = response.GetAsString();

			strResponse += "\r\n";

#ifndef _WITH_PROXY

#endif
			Log("++++++++ Me +++++++++\n%s-------- Me ---------\n", strResponse.c_str());

			if (!sock.Send(strResponse.c_str(), strResponse.length()))
			{
				Log("Communication failed\n");
			}
			request.InitNew();
		}
		sock.Detach();
	}
	virtual void OnDisconnect(SOCKET sd)
	{
		string strIP;

		GetPeerIP(sd, m_bV4, strIP);

		Log("Client %s disconnected\n", strIP.c_str());
		c_mtxConnection.Lock();

		auto i = c_mapConnection.find(sd);

		if (i != c_mapConnection.end())
		{
			auto pRaopContext = i->second;

			c_mapConnection.erase(i);
			c_mtxConnection.Unlock();

			if (dlgMain.m_hWnd != NULL && dlgMain.IsCurrentContext(pRaopContext))
			{
				dlgMain.PostMessage(WM_RAOP_CTX, 0, NULL);
			}
			if (pRaopContext->m_bIsStreaming)
			{
				pRaopContext->m_pDecoder->Stop();

				if (pSetThreadExecutionState != NULL)
					pSetThreadExecutionState(ES_CONTINUOUS);
			}
			pRaopContext->Fire(L"Disconnect");
		}
		else
			c_mtxConnection.Unlock();
	}
protected:
	bool						m_bV4;
};

class CRaopServerV6 : public CRaopServer
{
public:
	BOOL Run(int nPort)
	{
		m_bV4 = false;

		SOCKADDR_IN6 in;

		memset(&in, 0, sizeof(in));

		in.sin6_family		= AF_INET6;
		in.sin6_port		= nPort;
		in.sin6_addr		= in6addr_any;

		return CNetworkServer::Run(in);
	}
};

class CRaopServerV4 : public CRaopServer
{
public:
	BOOL Run()
	{
		m_bV4 = true;

		int		nPort	= 5000;
		BOOL	bResult = FALSE;
		int		nTry	= 10;

		do 
		{
			bResult	= CNetworkServer::Run(htons(nPort++));
		}
		while(!bResult && nTry--);

		return bResult;
	}
};

static CRaopServerV6			raop_v6_server;
static CRaopServerV4			raop_v4_server;
static CDnsSD_Register			dns_sd_register_server;
static CDnsSD_BrowseForService	dns_sd_browse_dacp;

#ifdef _WITH_PROXY
static CDnsSD_BrowseForService	dns_sd_browse_raop;
#endif

bool is_streaming()
{
	bool bResult = false;

	c_mtxConnection.Lock();

	for (auto i = c_mapConnection.begin(); i != c_mapConnection.end(); ++i)
	{
		if (i->second->m_bIsStreaming)
		{
			bResult = true;
			break;
		}
	}
	c_mtxConnection.Unlock();

	return bResult;
}

bool start_serving()
{
	USES_CONVERSION;

	// try to get hw_addr from registry
	string				strHwAddr;
	CTempBuffer<BYTE>	HwAddr;

	if (	GetValueFromRegistry(HKEY_LOCAL_MACHINE, "HwAddr", strHwAddr)
		&&	DecodeBase64(strHwAddr, HwAddr) == 6
		)
	{
		memcpy(hw_addr, HwAddr, 6);
	}
	else
	{
		for (long i = 1; i < 6; ++i)
		{
			hw_addr[i] = (256 * rand()) / RAND_MAX;
		}
	}
	hw_addr[0] = 0;

	if (!raop_v4_server.Run())
	{
		Log("can't listen on IPv4\n");
		::MessageBoxA(NULL, "Can't start network service on IPv4", strConfigName.c_str(), MB_ICONERROR|MB_OK);
		return false;
	}
	if (!Log("listening at least on IPv4...port %lu\n", (ULONG)ntohs(raop_v4_server.m_nPort)))
	{
		::MessageBoxA(NULL, "Can't log to file. Please run elevated", strConfigName.c_str(), MB_ICONERROR|MB_OK);
	}
	
#ifndef _WITH_PROXY
	if (raop_v6_server.Run(raop_v4_server.m_nPort))
	{
		Log("listening on IPv6...port %lu\n", (ULONG)ntohs(raop_v6_server.m_nPort));
	}
	else
	{
		Log("can't listen on IPv6. Don't care!\n");
	}
#endif
	if (!InitBonjour())
	{
		raop_v4_server.Cancel();
		raop_v6_server.Cancel();

		WTL::CString strInstBonj;

		ATLVERIFY(strInstBonj.LoadString(IDS_INSTALL_BONJOUR));

		if (IDOK == ::MessageBox(NULL, strInstBonj, A2CT(strConfigName.c_str()), MB_ICONINFORMATION|MB_OKCANCEL))
		{
			ShellExecute(NULL, L"open", L"http://support.apple.com/kb/DL999", NULL, NULL, SW_SHOWNORMAL);
		}
		return false;
	}
	g_PlugInManager.m_strApName = (PCWSTR) dlgMain.m_strApName;

	bool bResult = dns_sd_register_server.Start(hw_addr, W2CA_CP(g_PlugInManager.GetApName(), CP_UTF8), dlgMain.m_bNoMetaInfo, dlgMain.m_strPassword.IsEmpty(), raop_v4_server.m_nPort);

	if (!bResult)
	{
		long nTry			= 10;
		LPCSTR strApName	= W2CA_CP(dlgMain.m_strApName, CP_UTF8);

		do
		{
			Log("Could not register Raop.Tcp Service on Port %lu with dnssd.dll\n", (ULONG)ntohs(raop_v4_server.m_nPort));

			Sleep(1000);
			bResult = dns_sd_register_server.Start(hw_addr, strApName, dlgMain.m_bNoMetaInfo, dlgMain.m_strPassword.IsEmpty(), raop_v4_server.m_nPort);
			nTry--;
		}
		while(!bResult && nTry > 0);

		if (!bResult)
			::MessageBoxA(NULL, "Could not register Raop.Tcp Service with dnssd.dll", strConfigName.c_str(), MB_ICONERROR);
	}
	if (bResult)
	{
		Log("Registered Raop.Tcp Service ok\n");

		if (!dlgMain.m_bNoMediaControl)
		{
			if (!dns_sd_browse_dacp.Start("_dacp._tcp", dlgMain.m_hWnd, WM_DACP_REG))
			{
				Log("Failed to start DACP browser!\n");
				ATLASSERT(FALSE);
			}
			else
			{
				Log("Succeeded to start DACP browser!\n");
#ifdef _WITH_PROXY
				dns_sd_browse_raop.Start("_raop._tcp", dlgMain.m_hWnd, WM_RAOP_REG);
#endif
			}
		}
		else
		{
			Log("Omitted to start DACP browser - no Media Controls desired\n");
		}
	}
	else
	{
		Log("Raop.Tcp Service failed to register - Dead End!\n");

		raop_v4_server.Cancel();
		raop_v6_server.Cancel();
	}
	return bResult;
}

void stop_serving()
{
	Log("Stopping Services .... ");

	raop_v6_server.Cancel();
	raop_v4_server.Cancel();

#ifdef _WITH_PROXY
	dns_sd_browse_raop.Stop();
#endif
	dns_sd_browse_dacp.Stop();
	dns_sd_register_server.Stop();

	Log("stopped ok!\n");
}

/////////////////////////////////////////////////////////////////////
// CPlugInManager::UpdateTrayIcon

void CPlugInManager::UpdateTrayIcon(HICON hIcon)
{
	dlgMain.UpdateTrayIcon(hIcon);
}

/////////////////////////////////////////////////////////////////////
// Run

int Run(LPTSTR lpstrCmdLine = NULL)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	dlgMain.ReadConfig();

	if (dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
	if (start_serving())
	{
		bool bRunElevated			= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("Elevation")) != NULL;
		bool bWithChangeApName		= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("WithChangeApName")) != NULL;
		bool bWithChangeExtended	= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("WithChangeExtended")) != NULL;

		dlgMain.ShowWindow(dlgMain.m_bStartMinimized && !bRunElevated ? dlgMain.m_bTray ? SW_HIDE : SW_SHOWMINIMIZED : SW_SHOWDEFAULT);

		if (bRunElevated)
			::SetForegroundWindow(dlgMain.m_hWnd);
		if (bWithChangeApName)
			dlgMain.PostMessage(WM_COMMAND, IDC_CHANGENAME);
		if (bWithChangeExtended)
			dlgMain.PostMessage(WM_COMMAND, IDC_EXTENDED_OPTIONS);
		int nRet = theLoop.Run();

		_Module.RemoveMessageLoop();

		stop_serving();


		return nRet;
	}
	else
		dlgMain.DestroyWindow();
	return 0;
}

/////////////////////////////////////////////////////////////////////
// _tWinMain

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	USES_CONVERSION;

	DetermineOS();

	HRESULT hRes = ::CoInitializeEx(NULL, bIsWinXP ? COINIT_APARTMENTTHREADED : COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES);	// add flags to support other controls

	setlocale(LC_ALL, "");

	LPCTSTR strConfig = NULL;

	int nRet = -1;

	if (lpstrCmdLine != NULL && (strConfig = _tcsstr(lpstrCmdLine, _T("/Config:"))) != NULL)
	{
		string	str;
		long	n = 8;

		while(*(strConfig+n) && *(strConfig+n) != _T(' '))
		{
			str += (char)*(strConfig+n);
			n++;
		}
		Trim(str, "\t\n\r\"");

		if (!str.empty())
		{
			bPrivateConfig	= true;
			strConfigName	= str;
			dlgMain.m_strApName = A2CT(strConfigName.c_str());
		}
	}
	CTempBuffer<BYTE> pKey;
	HMODULE hKernel32 = NULL;

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR			gdiplusToken				= 0;

	gdiplusStartupInput.GdiplusVersion				= 1;
	gdiplusStartupInput.DebugEventCallback			= NULL;
	gdiplusStartupInput.SuppressBackgroundThread	= FALSE;
	gdiplusStartupInput.SuppressExternalCodecs		= FALSE; 
		
	ATLVERIFY(GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Gdiplus::Ok);

	if (mtxAppSessionInstance.Create(NULL, FALSE, WTL::CString(_T("__")) + WTL::CString(A2CT(strConfigName.c_str())) + WTL::CString(_T("SessionInstanceIsRunning"))))
	{
		while (WaitForSingleObject(mtxAppSessionInstance, 0) != WAIT_OBJECT_0)
		{
			if (lpstrCmdLine == NULL || _tcsstr(lpstrCmdLine, _T("Elevation")) == NULL)
			{
				HANDLE hShowEvent = ::OpenEvent(EVENT_MODIFY_STATE, FALSE, EVENT_NAME_SHOW_WINDOW);

				if (hShowEvent != NULL)
				{
					SetEvent(hShowEvent);
					CloseHandle(hShowEvent);
					hShowEvent = NULL;
				}
				else
				{
					WTL::CString strMsg;
					ATLVERIFY(strMsg.LoadString(IDS_ERR_MULTIPLE_INSTANCES));
					::MessageBoxW(NULL, strMsg, A2CW(strConfigName.c_str()), MB_ICONWARNING|MB_OK);
				}
				goto exit;		
			}
			Sleep(500);
		}
	}
	else
	{
		ATLASSERT(FALSE);
	}
	if (!mtxAppGlobalInstance.Create(NULL, FALSE, WTL::CString(_T("Global\\__")) + WTL::CString(A2CT(strConfigName.c_str())) + WTL::CString(_T("GlobalInstanceIsRunning"))) || WaitForSingleObject(mtxAppGlobalInstance, 0) != WAIT_OBJECT_0)
	{
		goto exit;
	}
	long sizeKey = DecodeBase64(strPrivateKey, pKey);
	ATLASSERT(pKey != NULL);

	const unsigned char* _pKey = pKey;

	d2i_RSAPrivateKey(&pRSA, &_pKey, sizeKey);
	ATLASSERT(pRSA != NULL);

	srand(GetTickCount());

	hKernel32 = ::LoadLibraryA("Kernel32.dll");

	if (hKernel32 != NULL)
	{
		pSetThreadExecutionState = (_SetThreadExecutionState) GetProcAddress(hKernel32, "SetThreadExecutionState");
	}
	nRet = Run(lpstrCmdLine);

	RSA_free(pRSA);

	c_mtxConnection.Lock();
	c_mapConnection.clear();
	c_mtxConnection.Unlock();
exit:
	GdiplusShutdown(gdiplusToken);

	if (hKernel32 != NULL)
	{
		FreeLibrary(hKernel32);
	}

	_Module.Term();
	DeInitBonjour();

	::CoUninitialize();

	return nRet;
}
