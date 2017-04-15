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
#include "ShairportContainer.h"



ShairportContainer shairportContainer;
ShairportContainer* pShairportContainer_ = &shairportContainer;
BOOL& bLogToFile(shairportContainer.bLogToFile);

//typedef EXECUTION_STATE (WINAPI *_SetThreadExecutionState)(EXECUTION_STATE esFlags);
//_SetThreadExecutionState	pSetThreadExecutionState = NULL;
extern _SetThreadExecutionState pSetThreadExecutionState;

//static	SocketConnectionMap	c_mapConnection;
//static 	CMyMutex								c_mtxConnection;
//CPlugInManager							g_PlugInManager;
//
//CAppModule		_Module;
//CMutex			mtxAppSessionInstance;
//CMutex			mtxAppGlobalInstance;
//CMainDlg		dlgMain;
//BYTE			hw_addr[6];
//CMyMutex		mtxRSA;
//RSA*			pRSA			= NULL;
//BOOL&			bLogToFile(dlgMain.m_bLogToFile);
//string			strPrivateKey	=	"MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUt"
//									"wC5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4U"
//									"wPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1zNS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf"
//									"/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij2VZ9Zmni/"
//									"UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfW"
//									"BLmkzkEiqoSwF0PsmVrPzH9KsnwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAa"
//									"LlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscEsA5ltpxOgUGCY7b7ez5"
//									"NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jm"
//									"lpPHr0O/KnPQtzI3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurciz"
//									"aaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmotiCcMXFEQEWflzhWYTsXrhUIuz5jFu"
//									"a39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndM"
//									"oPdevWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0Z"
//									"oMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+"
//									"k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzDFxU8hL"
//									"AoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvA"
//									"cJyRM9SJ7OKlGt0FMSdJD5KG0XPIpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf"
//									"54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDaK3TFBVmD7fV0Zhov"
//									"17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc"
//									"1JnLYT4iyUyx6pcZBmCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gI"
//									"LAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vXbKuU4ISgxB2bB3HcYzQMGsz1qJ"
//									"2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=";

//static bool digest_ok(shared_ptr<CRaopContext>& pRaopContext)
//{
//	if (!pRaopContext->m_strNonce.empty())
//	{
//		string& strAuth = pRaopContext->m_CurrentHttpRequest.m_mapHeader["Authorization"];
//
//		if (strAuth.find("Digest ") == 0)
//		{
//			USES_CONVERSION;
//
//			map<ci_string, string> mapAuth;
//			ParseRegEx(strAuth.c_str()+7, "{\\a+}=\"{[^\"]+}\"", mapAuth, FALSE);
//
//			string str1 = mapAuth["username"] + string(":") + mapAuth["realm"] + string(":") + string(T2CA(dlgMain.m_strPassword));
//			string str2 = pRaopContext->m_CurrentHttpRequest.m_strMethod + string(":") + mapAuth["uri"];
//
//			string strDigest1 = MD5_Hex((const BYTE*)str1.c_str(), str1.length(), true);
//			string strDigest2 = MD5_Hex((const BYTE*)str2.c_str(), str2.length(), true);
//
//			string strDigest = strDigest1 + string(":") + mapAuth["nonce"] + string(":") + strDigest2;
//
//			strDigest = MD5_Hex((const BYTE*)strDigest.c_str(), strDigest.length(), true);
//
//			Log("Password Digest: \"%s\" == \"%s\"\n", strDigest.c_str(), mapAuth["response"].c_str());
//
//			return strDigest == mapAuth["response"];
//		}
//	}
//	return false;
//}

//static CRaopServerV6			raop_v6_server;
//static CRaopServerV4			raop_v4_server;
static CDnsSD_Register			dns_sd_register_server;
static CDnsSD_BrowseForService	dns_sd_browse_dacp;

#ifdef _WITH_PROXY
static CDnsSD_BrowseForService	dns_sd_browse_raop;
#endif

bool is_streaming()
{
	bool bResult = false;

   pShairportContainer_->mutexConnection_.Lock();

	for (auto i = pShairportContainer_->socketConnectionMap_.begin();
      i != pShairportContainer_->socketConnectionMap_.end(); ++i)
	{
		if (i->second->m_bIsStreaming)
		{
			bResult = true;
			break;
		}
	}
   pShairportContainer_->mutexConnection_.Unlock();

	return bResult;
}

bool start_serving()
{
	USES_CONVERSION;

	// try to get hw_addr from registry
	//string				strHwAddr;
	//CTempBuffer<BYTE>	HwAddr;
 //  BYTE hw_addr[6];
	//if (	GetValueFromRegistry(HKEY_LOCAL_MACHINE, "HwAddr", strHwAddr)
	//	&&	DecodeBase64(strHwAddr, HwAddr) == 6
	//	)
	//{
	//	memcpy(hw_addr, HwAddr, 6);
	//}
	//else
	//{
	//	for (long i = 1; i < 6; ++i)
	//	{
 //        hw_addr[i] = (256 * rand()) / RAND_MAX;
	//	}
	//}
 //  hw_addr[0] = 0;

	if (!pShairportContainer_->raop_v4_server.Run())
	{
		Log("can't listen on IPv4\n");
		::MessageBoxA(NULL, "Can't start network service on IPv4", strConfigName.c_str(), MB_ICONERROR|MB_OK);
		return false;
	}
	if (!Log("listening at least on IPv4...port %lu\n", (ULONG)ntohs(pShairportContainer_->raop_v4_server.m_nPort)))
	{
		::MessageBoxA(NULL, "Can't log to file. Please run elevated", strConfigName.c_str(), MB_ICONERROR|MB_OK);
	}
	
#ifndef _WITH_PROXY
	if (pShairportContainer_->raop_v6_server.Run(pShairportContainer_->raop_v4_server.m_nPort))
	{
		Log("listening on IPv6...port %lu\n", (ULONG)ntohs(pShairportContainer_->raop_v6_server.m_nPort));
	}
	else
	{
		Log("can't listen on IPv6. Don't care!\n");
	}
#endif
	if (!InitBonjour())
	{
      pShairportContainer_->raop_v4_server.Cancel();
      pShairportContainer_->raop_v6_server.Cancel();

		WTL::CString strInstBonj;

		ATLVERIFY(strInstBonj.LoadString(IDS_INSTALL_BONJOUR));

		if (IDOK == ::MessageBox(NULL, strInstBonj, A2CT(strConfigName.c_str()), MB_ICONINFORMATION|MB_OKCANCEL))
		{
			ShellExecute(NULL, L"open", L"http://support.apple.com/kb/DL999", NULL, NULL, SW_SHOWNORMAL);
		}
		return false;
	}
   pShairportContainer_->g_PlugInManager.m_strApName = (PCWSTR)pShairportContainer_->dlgMain.m_strApName;

	bool bResult = dns_sd_register_server.Start(
      pShairportContainer_->hardwareAddressRetriever.hardwareAddress(), 
      W2CA_CP(pShairportContainer_->g_PlugInManager.GetApName(), CP_UTF8),
      pShairportContainer_->dlgMain.m_bNoMetaInfo, 
      pShairportContainer_->dlgMain.m_strPassword.IsEmpty(),
      pShairportContainer_->raop_v4_server.m_nPort);

	if (!bResult)
	{
		long nTry			= 10;
		LPCSTR strApName	= W2CA_CP(pShairportContainer_->dlgMain.m_strApName, CP_UTF8);

		do
		{
			Log("Could not register Raop.Tcp Service on Port %lu with dnssd.dll\n", (ULONG)ntohs(pShairportContainer_->raop_v4_server.m_nPort));

			Sleep(1000);
			bResult = dns_sd_register_server.Start(
            pShairportContainer_->hardwareAddressRetriever.hardwareAddress(),
            strApName, 
            pShairportContainer_->dlgMain.m_bNoMetaInfo,
            pShairportContainer_->dlgMain.m_strPassword.IsEmpty(),
            pShairportContainer_->raop_v4_server.m_nPort);
			nTry--;
		}
		while(!bResult && nTry > 0);

		if (!bResult)
			::MessageBoxA(NULL, "Could not register Raop.Tcp Service with dnssd.dll", strConfigName.c_str(), MB_ICONERROR);
	}
	if (bResult)
	{
		Log("Registered Raop.Tcp Service ok\n");

		if (!pShairportContainer_->dlgMain.m_bNoMediaControl)
		{
			if (!dns_sd_browse_dacp.Start("_dacp._tcp", pShairportContainer_->dlgMain.m_hWnd, WM_DACP_REG))
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

      pShairportContainer_->raop_v4_server.Cancel();
      pShairportContainer_->raop_v6_server.Cancel();
	}
	return bResult;
}

void stop_serving()
{
	Log("Stopping Services .... ");

   pShairportContainer_->raop_v6_server.Cancel();
   pShairportContainer_->raop_v4_server.Cancel();

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
   pShairportContainer_->dlgMain.UpdateTrayIcon(hIcon);
}

/////////////////////////////////////////////////////////////////////
// Run

int Run(LPTSTR lpstrCmdLine = NULL)
{
	CMessageLoop theLoop;
   pShairportContainer_->_Module.AddMessageLoop(&theLoop);

   pShairportContainer_->dlgMain.ReadConfig();

	if (pShairportContainer_->dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
	if (start_serving())
	{
		bool bRunElevated			= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("Elevation")) != NULL;
		bool bWithChangeApName		= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("WithChangeApName")) != NULL;
		bool bWithChangeExtended	= lpstrCmdLine != NULL && _tcsstr(lpstrCmdLine, _T("WithChangeExtended")) != NULL;

      pShairportContainer_->dlgMain.ShowWindow(
         pShairportContainer_->dlgMain.m_bStartMinimized && !bRunElevated ? 
         pShairportContainer_->dlgMain.m_bTray ? SW_HIDE : SW_SHOWMINIMIZED : SW_SHOWDEFAULT);

      if (bRunElevated)
      {
         ::SetForegroundWindow(pShairportContainer_->dlgMain.m_hWnd);
      }
      if (bWithChangeApName)
      {
         pShairportContainer_->dlgMain.PostMessage(WM_COMMAND, IDC_CHANGENAME);
      }
      if (bWithChangeExtended)
      {
         pShairportContainer_->dlgMain.PostMessage(WM_COMMAND, IDC_EXTENDED_OPTIONS);
      }
		int nRet = theLoop.Run();

      pShairportContainer_->_Module.RemoveMessageLoop();

		stop_serving();


		return nRet;
	}
	else
      pShairportContainer_->dlgMain.DestroyWindow();
	return 0;
}

/////////////////////////////////////////////////////////////////////
// _tWinMain

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	USES_CONVERSION;

   shairportContainer.hardwareAddressRetriever.initializeHardwareAddress();

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
         pShairportContainer_->dlgMain.m_strApName = A2CT(strConfigName.c_str());
		}
	}
	CTempBuffer<BYTE> pKey;
	HMODULE hKernel32 = NULL;

	hRes = pShairportContainer_->_Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR			gdiplusToken				= 0;

	gdiplusStartupInput.GdiplusVersion				= 1;
	gdiplusStartupInput.DebugEventCallback			= NULL;
	gdiplusStartupInput.SuppressBackgroundThread	= FALSE;
	gdiplusStartupInput.SuppressExternalCodecs		= FALSE; 
		
	ATLVERIFY(GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) == Gdiplus::Ok);

	if (pShairportContainer_->mtxAppSessionInstance.Create(NULL, FALSE, WTL::CString(_T("__")) + WTL::CString(A2CT(strConfigName.c_str())) + WTL::CString(_T("SessionInstanceIsRunning"))))
	{
		while (WaitForSingleObject(pShairportContainer_->mtxAppSessionInstance, 0) != WAIT_OBJECT_0)
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
	if (!pShairportContainer_->mtxAppGlobalInstance.Create(
      NULL, FALSE, 
      WTL::CString(_T("Global\\__")) + WTL::CString(A2CT(strConfigName.c_str())) + WTL::CString(_T("GlobalInstanceIsRunning"))) || WaitForSingleObject(pShairportContainer_->mtxAppGlobalInstance, 0) != WAIT_OBJECT_0)
	{
		goto exit;
	}

	//long sizeKey = DecodeBase64(strPrivateKey, pKey);
	//ATLASSERT(pKey != NULL);

	//const unsigned char* _pKey = pKey;

	//d2i_RSAPrivateKey(&pShairportContainer_->pRSA, &_pKey, sizeKey);
	//ATLASSERT(pRSA != NULL);

   shairportContainer.airplayRsaKey.SetupRsaKey();

	srand(GetTickCount());

	hKernel32 = ::LoadLibraryA("Kernel32.dll");

	if (hKernel32 != NULL)
	{
      pSetThreadExecutionState = (_SetThreadExecutionState) GetProcAddress(hKernel32, "SetThreadExecutionState");
	}
	nRet = Run(lpstrCmdLine);

	//RSA_free(pShairportContainer_->pRSA);

   pShairportContainer_->mutexConnection_.Lock();
   pShairportContainer_->socketConnectionMap_.clear();
   pShairportContainer_->mutexConnection_.Unlock();
exit:
	GdiplusShutdown(gdiplusToken);

	if (hKernel32 != NULL)
	{
		FreeLibrary(hKernel32);
	}

   pShairportContainer_->_Module.Term();
	DeInitBonjour();

	::CoUninitialize();

	return nRet;
}
