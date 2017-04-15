
#include "stdafx.h"
#include "RemoteAudioOutputProtocolServer.h"
#include <atlapp.h>
#include "myMutex.h"
#include "MainDlg.h"
#include "HardwareAddressRetriever.h"
#include "AirplayRsaKey.h"

_SetThreadExecutionState pSetThreadExecutionState = NULL;

RemoteAudioOutputProtocolServer::RemoteAudioOutputProtocolServer(
   CMyMutex& mutexConnection,
   SocketConnectionMap& connectionMap,
   IAirplayRsaKey& airplayRsaKey,
   IHardwareAddressRetriever& hardwareAddressRetriever,
   CMyMutex& rsaMutex,
   CMainDlg& mainDialog)
   : m_bV4(false)
   , m_mutexConnection(mutexConnection)
   , m_connectionMap(connectionMap)
   , m_airplayRsaKey(airplayRsaKey)
   , m_hardwareAddressRetriever(hardwareAddressRetriever)
   , mtxRSA(rsaMutex)
   , dlgMain(mainDialog)
{

}

RemoteAudioOutputProtocolServer::~RemoteAudioOutputProtocolServer()
{

}

bool RemoteAudioOutputProtocolServer::digest_ok(shared_ptr<CRaopContext>& pRaopContext)
{
   if (!pRaopContext->m_strNonce.empty())
   {
      string& strAuth = pRaopContext->m_CurrentHttpRequest.m_mapHeader["Authorization"];

      if (strAuth.find("Digest ") == 0)
      {
         USES_CONVERSION;

         map<ci_string, string> mapAuth;
         ParseRegEx(strAuth.c_str() + 7, "{\\a+}=\"{[^\"]+}\"", mapAuth, FALSE);

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

void RemoteAudioOutputProtocolServer::on_string(void* ctx, const char *code, const char *name, const char *buf, size_t len)
{
   CRaopContext* pRaopContext = (CRaopContext*)ctx;
   ATLASSERT(pRaopContext != NULL);

   if (stricmp(name, "daap.songalbum") == 0)
   {
      WTL::CString strValue;

      LPTSTR p = strValue.GetBuffer(len * 2);

      if (p != NULL)
      {
         int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len * 2);
         strValue.ReleaseBuffer((n >= 0) ? n : -1);
      }
      pRaopContext->Lock();
      pRaopContext->m_str_daap_songalbum = strValue;
      pRaopContext->Unlock();
   }
   else if (stricmp(name, "daap.songartist") == 0)
   {
      WTL::CString strValue;

      LPTSTR p = strValue.GetBuffer(len * 2);

      if (p != NULL)
      {
         int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len * 2);
         strValue.ReleaseBuffer((n >= 0) ? n : -1);
      }
      pRaopContext->Lock();
      pRaopContext->m_str_daap_songartist = strValue;
      pRaopContext->Unlock();
   }
   else if (stricmp(name, "dmap.itemname") == 0)
   {
      WTL::CString strValue;

      LPTSTR p = strValue.GetBuffer(len * 2);

      if (p != NULL)
      {
         int n = ::MultiByteToWideChar(CP_UTF8, 0, buf, len, p, len * 2);
         strValue.ReleaseBuffer((n >= 0) ? n : -1);
      }
      pRaopContext->Lock();
      pRaopContext->m_str_daap_trackname = strValue;
      pRaopContext->Unlock();
   }
}

bool RemoteAudioOutputProtocolServer::OnAccept(SOCKET sd)
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

      m_mutexConnection.Lock();

      m_connectionMap.erase(sd);

      m_connectionMap[sd] = pRaopContext;

      GetPeerIP(sd, m_bV4, (struct sockaddr_in*)&m_connectionMap[sd]->m_Peer);

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
      m_mutexConnection.Unlock();

      return true;
   }
   return false;
}

void RemoteAudioOutputProtocolServer::OnRequest(SOCKET sd)
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

   m_mutexConnection.Lock();

   auto pRaopContext = m_connectionMap[sd];

   m_mutexConnection.Unlock();

   CHttp&	request = pRaopContext->m_CurrentHttpRequest;

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
               request.m_strUrl = request.m_strUrl.substr(0, nHostIdx)
                  + pRaopContext->m_strLocalIP
                  + request.m_strUrl.substr(nHostIdx + pRaopContext->m_strPeerIP.length());
            }
         }
         if (stricmp(request.m_strMethod.c_str(), "SETUP") != 0
            && stricmp(request.m_strMethod.c_str(), "ANNOUNCE") != 0
            )
         {
            string strProxyRequest = request.GetAsString(false);

            pRaopContext->m_Proxy.Send(strProxyRequest.c_str(), strProxyRequest.length());

            if (pRaopContext->m_Proxy.WaitForIncomingData(5000))
            {
               int		nProxyReceived = pRaopContext->m_Proxy.recv(buf, 0x100000);
               string	strProxyResponse = string(buf, nProxyReceived);
               Log("+++++++ Proxy +++++++\n%s------- Proxy -------\n", strProxyResponse.c_str());
            }
         }
      }
#endif
      CHttp response;

      response.Create(request.m_strProtocol.c_str());

      response.m_mapHeader["CSeq"] = request.m_mapHeader["CSeq"];
      response.m_mapHeader["Audio-Jack-Status"] = "connected; type=analog";
      response.m_mapHeader["Server"] = "AirTunes/105.1";

      if (request.m_mapHeader.find("Apple-Challenge") != request.m_mapHeader.end())
      {
         string strChallenge = request.m_mapHeader["Apple-Challenge"];

         CTempBuffer<BYTE>	pChallenge;
         long				sizeChallenge = 0;

         if ((sizeChallenge = DecodeBase64(strChallenge, pChallenge)))
         {
            CTempBuffer<BYTE>	pIP;
            int					sizeIP = GetLocalIP(sd, m_bV4, pIP);

            BYTE pAppleResponse[64];
            long sizeAppleResponse = 0;

            memset(pAppleResponse, 0, sizeof(pAppleResponse));

            memcpy(pAppleResponse + sizeAppleResponse, pChallenge, sizeChallenge);
            sizeAppleResponse += sizeChallenge;
            ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));
            memcpy(pAppleResponse + sizeAppleResponse, pIP, sizeIP);
            sizeAppleResponse += sizeIP;
            ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));

            memcpy(
               pAppleResponse + sizeAppleResponse, 
               m_hardwareAddressRetriever.hardwareAddress(), 
               m_hardwareAddressRetriever.sizeInBytes());
            sizeAppleResponse += m_hardwareAddressRetriever.sizeInBytes();
            ATLASSERT(sizeAppleResponse <= sizeof(pAppleResponse));

            sizeAppleResponse = max(32, sizeAppleResponse);

            LPBYTE pSignature = new BYTE[2048];
            size_t sizeSignature = 0;

            mtxRSA.Lock();
            sizeSignature = RSA_private_encrypt(
               sizeAppleResponse, 
               pAppleResponse, 
               pSignature, 
               m_airplayRsaKey.rsaKey(), 
               RSA_PKCS1_PADDING);
            mtxRSA.Unlock();

            ATLASSERT(sizeSignature > 0);

            string strAppleResponse;
            EncodeBase64(pSignature, sizeSignature, strAppleResponse, false);

            TrimRight(strAppleResponse, "=\r\n");

            response.m_mapHeader["Apple-Response"] = strAppleResponse;

            delete[] pSignature;
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

            response.m_mapHeader["WWW-Authenticate"] = string("Digest realm=\"")
               + string(T2CA(dlgMain.m_strApName))
               + string("\", nonce=\"")
               + pRaopContext->m_strNonce
               + string("\"");
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

         pRaopContext->m_strFmtp = mapKeyValue["fmtp"];
         pRaopContext->m_sizeAesiv = DecodeBase64(mapKeyValue["aesiv"], pRaopContext->m_Aesiv);
         pRaopContext->m_sizeRsaaeskey = DecodeBase64(mapKeyValue["rsaaeskey"], pRaopContext->m_Rsaaeskey);

         pRaopContext->m_strUserAgent = A2CW_CP(request.m_mapHeader["User-Agent"].c_str(), CP_UTF8);
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
         pRaopContext->m_sizeRsaaeskey = RSA_private_decrypt(
            pRaopContext->m_sizeRsaaeskey, 
            pRaopContext->m_Rsaaeskey, 
            pRaopContext->m_Rsaaeskey, 
            m_airplayRsaKey.rsaKey(), 
            RSA_PKCS1_OAEP_PADDING);
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
               int		nProxyReceived = pRaopContext->m_Proxy.recv(buf, 0x100000);
               string	strProxyResponse = string(buf, nProxyReceived);
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
               } while (dlgMain.GetMMState() != CMainDlg::pause && --nTry > 0);
            }
         }
         m_mutexConnection.Lock();

         for (auto i = m_connectionMap.begin(); i != m_connectionMap.end(); ++i)
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

         if (!dlgMain.m_strSoundRedirection.IsEmpty() && dlgMain.m_bSoundRedirection)
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

            long nServerPort = pRaopContext->m_pDecoder->GetServerPort();
            long nControlPort = pRaopContext->m_pDecoder->GetControlPort();
            long nTimingPort = pRaopContext->m_pDecoder->GetTimingPort();

            sprintf_s(strTransport, 256, "RTP/AVP/UDP;unicast;mode=record;server_port=%ld;control_port=%ld;timing_port=%ld", nServerPort, nControlPort, nTimingPort);

            response.m_mapHeader["Transport"] = strTransport;

#ifdef _WITH_PROXY
            if (pRaopContext->m_Proxy.IsValid())
            {
               nControlPort = pRaopContext->m_pDecoder->GetProxyControlPort();
               nTimingPort = pRaopContext->m_pDecoder->GetProxyTimingPort();
               sprintf_s(strTransport, 256, "RTP/AVP/UDP;unicast;mode=record;timing_port=%ld;control_port=%ld", nTimingPort, nControlPort);

               request.m_mapHeader["Transport"] = strTransport;

               string strProxyRequest = request.GetAsString(false);

               pRaopContext->m_Proxy.Send(strProxyRequest.c_str(), strProxyRequest.length());

               if (pRaopContext->m_Proxy.WaitForIncomingData(5000))
               {
                  int		nProxyReceived = pRaopContext->m_Proxy.recv(buf, 0x100000);
                  string	strProxyResponse = string(buf, nProxyReceived);
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
               pRaopContext->m_pDecoder->m_Proxy_timingport = -1;
               pRaopContext->m_pDecoder->m_Proxy_serverport = -1;
            }
#endif
         }
         else
         {
            pRaopContext->m_bIsStreaming = false;
            response.SetStatus(503, "Service Unavailable");
         }
         m_mutexConnection.Unlock();
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

               if (stricmp(ct->second.c_str() + 6, "none") == 0)
               {
                  // do nothing
               }
               else if (stricmp(ct->second.c_str() + 6, "jpeg") == 0 || stricmp(ct->second.c_str() + 6, "png") == 0)
               {
                  pRaopContext->m_nBitmapBytes = 0;

                  long nBodyLength = (long)request.m_strBody.length();

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

                              double start = atof(tsi->c_str());
                              double curr = atof((++tsi)->c_str());
                              double end = atof((++tsi)->c_str());

                              if (end >= start)
                              {
                                 // obviously set wrongly by some Apps (e.g. Soundcloud) 
                                 if (start > curr)
                                 {
                                    double _curr = curr;
                                    curr = start;
                                    start = _curr;
                                 }
                                 time_t nTotalSecs = (time_t)((end - start) / pRaopContext->m_lfFreq);

                                 pRaopContext->Lock();

                                 pRaopContext->m_rtpStart = MAKEULONGLONG((ULONG)start, 0);
                                 pRaopContext->m_rtpCur = MAKEULONGLONG((ULONG)curr, 0);
                                 pRaopContext->m_rtpEnd = MAKEULONGLONG((ULONG)end, 0);
                                 pRaopContext->m_nTimeStamp = ::time(NULL);
                                 pRaopContext->m_nTimeTotal = nTotalSecs;

                                 if (curr > start)
                                    pRaopContext->m_nTimeCurrentPos = (time_t)((curr - start) / pRaopContext->m_lfFreq);
                                 else
                                    pRaopContext->m_nTimeCurrentPos = 0;

                                 pRaopContext->m_nDurHours = (long)(nTotalSecs / 3600);
                                 nTotalSecs -= ((time_t)pRaopContext->m_nDurHours * 3600);

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

void RemoteAudioOutputProtocolServer::OnDisconnect(SOCKET sd)
{
   string strIP;

   GetPeerIP(sd, m_bV4, strIP);

   Log("Client %s disconnected\n", strIP.c_str());
   m_mutexConnection.Lock();

   auto i = m_connectionMap.find(sd);

   if (i != m_connectionMap.end())
   {
      auto pRaopContext = i->second;

      m_connectionMap.erase(i);
      m_mutexConnection.Unlock();

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
      m_mutexConnection.Unlock();
}

