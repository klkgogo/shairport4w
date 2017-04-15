#pragma once

#include "SocketConnectionMap.h"
#include "myMutex.h"
#include "RemoteAudioOutputProtocolV6.h"
#include "RemoteAudioOutputProtocolServerV4.h"
#include "MainDlg.h"
#include "HardwareAddressRetriever.h"
#include "AirplayRsaKey.h"

class ShairportContainer
{
public:
   ShairportContainer();
   ~ShairportContainer();

   SocketConnectionMap socketConnectionMap_;
   CMyMutex mutexConnection_;

   //gotta clean this up still.
   CPlugInManager g_PlugInManager;
   CAppModule _Module;
   CMutex mtxAppSessionInstance;
   CMutex mtxAppGlobalInstance;
   HardwareAddressRetriever hardwareAddressRetriever;
   CMainDlg dlgMain;
   CMyMutex mtxRSA;
   AirplayRsaKey airplayRsaKey;
   BOOL& bLogToFile;

   CRaopServerV6 raop_v6_server;
   CRaopServerV4 raop_v4_server;
};

