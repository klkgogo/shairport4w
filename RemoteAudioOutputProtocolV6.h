#pragma once

#include "RemoteAudioOutputProtocolServer.h"

class CRaopServerV6 : public RemoteAudioOutputProtocolServer
{
public:
   CRaopServerV6(
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

