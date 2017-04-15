#pragma once

#include "RemoteAudioOutputProtocolServer.h"

class CRaopServerV4 : public RemoteAudioOutputProtocolServer
{
public:
   CRaopServerV4(
      CMyMutex& mutexConnection,
      SocketConnectionMap& connectionMap,
      IAirplayRsaKey& airplayRsaKey,
      IHardwareAddressRetriever& hardwareAddressRetriever,
      CMyMutex& rsaMutex,
      CMainDlg& mainDialog)
      : RemoteAudioOutputProtocolServer(
         mutexConnection,
         connectionMap,
         airplayRsaKey,
         hardwareAddressRetriever,
         rsaMutex,
         mainDialog)
   {

   }
      
   
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

