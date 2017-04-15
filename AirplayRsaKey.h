#pragma once


class IAirplayRsaKey
{
public:
   virtual RSA* rsaKey() = 0;
};

class AirplayRsaKey : public IAirplayRsaKey
{
public:
   AirplayRsaKey();
   virtual ~AirplayRsaKey();

   virtual RSA* rsaKey();
   bool SetupRsaKey();

private:
   CTempBuffer<BYTE> m_pKey;
   RSA* m_airplayRsaKey;


};

