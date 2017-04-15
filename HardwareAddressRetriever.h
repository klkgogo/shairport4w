#pragma once

#include <wtypes.h>
#include <string>
#include <atlapp.h>
#include "utils.h"

class IHardwareAddressRetriever
{
public:
   virtual BYTE* hardwareAddress() = 0;
   virtual int sizeInBytes() = 0;
};

class HardwareAddressRetriever : public IHardwareAddressRetriever
{
public:
   HardwareAddressRetriever() 
   {

   }
   ~HardwareAddressRetriever() {}

   void initializeHardwareAddress()
   {
      std::string				strHwAddr;
      CTempBuffer<BYTE>	HwAddr;

      if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "HwAddr", strHwAddr)
         && DecodeBase64(strHwAddr, HwAddr) == 6
         )
      {
         memcpy(m_hardwareAddress, HwAddr, 6);
      }
      else
      {
         for (long i = 1; i < 6; ++i)
         {
            m_hardwareAddress[i] = (256 * rand()) / RAND_MAX;
         }
      }
      m_hardwareAddress[0] = 0;
   }

   virtual BYTE* hardwareAddress() 
   {
      return m_hardwareAddress;
   }

   virtual int sizeInBytes() 
   {
      return sizeof(m_hardwareAddress);
   }

private:
   BYTE m_hardwareAddress[6];

};