#include "stdafx.h"
#include "ShairportContainer.h"


ShairportContainer::ShairportContainer()
   : socketConnectionMap_()
   , mutexConnection_()

   , g_PlugInManager()
   , _Module()
   , mtxAppSessionInstance()
   , mtxAppGlobalInstance()
   , hardwareAddressRetriever()
   , dlgMain(
      _Module, 
      hardwareAddressRetriever)
   , mtxRSA()
   , airplayRsaKey()
   , bLogToFile(dlgMain.m_bLogToFile)




   , raop_v6_server(
      mutexConnection_, 
      socketConnectionMap_,
      airplayRsaKey,
      hardwareAddressRetriever,
      mtxRSA,
      dlgMain)
   , raop_v4_server(
      mutexConnection_,
      socketConnectionMap_,
      airplayRsaKey,
      hardwareAddressRetriever,
      mtxRSA,
      dlgMain)
{
}


ShairportContainer::~ShairportContainer()
{
}
