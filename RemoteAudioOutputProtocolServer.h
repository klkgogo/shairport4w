#pragma once


#include "Networking.h"
#include "DmapParser.h"
//#include "RaopContext.h"
#include <winsock.h>
#include "SocketConnectionMap.h"

class IAirplayRsaKey;
class IHardwareAddressRetriever;
class CMyMutex;
class CMainDlg;

typedef EXECUTION_STATE(WINAPI *_SetThreadExecutionState)(EXECUTION_STATE esFlags);


class RemoteAudioOutputProtocolServer : public CNetworkServer, protected CDmapParser
{
public:
   RemoteAudioOutputProtocolServer(
      CMyMutex& mutexConnection,
      SocketConnectionMap& connectionMap,
      IAirplayRsaKey& airplayRsaKey,
      IHardwareAddressRetriever& hardwareAddressRetriever,
      CMyMutex& rsaMutex,
      CMainDlg& mainDialog);
   virtual ~RemoteAudioOutputProtocolServer();

protected:
   bool digest_ok(shared_ptr<CRaopContext>& pRaopContext);

protected:
	// Dmap Parser Callbacks
	virtual void on_string(
      void* ctx, 
      const char *code, 
      const char *name, 
      const char *buf, size_t len);
	// Network Callbacks
	virtual bool OnAccept(SOCKET sd);
	virtual void OnRequest(SOCKET sd);
	virtual void OnDisconnect(SOCKET sd);

protected:
   bool m_bV4;
   CMyMutex& m_mutexConnection;
   SocketConnectionMap& m_connectionMap;
   IAirplayRsaKey& m_airplayRsaKey;
   IHardwareAddressRetriever& m_hardwareAddressRetriever;
   CMyMutex& mtxRSA;
   CMainDlg& dlgMain;
};
