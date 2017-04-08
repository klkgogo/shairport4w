/*
 *
 *  MainDlg.cpp 
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
#include "ExtOptsDlg.h"
#include "ChangeNameDlg.h"

static const	WTL::CString	c_strEmptyTimeInfo = _T("--:--");

extern			BYTE			hw_addr[6];
volatile		LONG			g_bMute = 0;

#define STR_EMPTY_TIME_INFO c_strEmptyTimeInfo
#define	REPLACE_CONTROL_VERT(__ctl,___offset) 	__ctl.GetClientRect(&rect);							\
											__ctl.MapWindowPoints(m_hWnd, &rect);				\
											__ctl.SetWindowPos(NULL, rect.left, rect.top+(___offset), 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED); 
#define	REPLACE_CONTROL_HORZ(__ctl,___offset) 	__ctl.GetClientRect(&rect);							\
											__ctl.MapWindowPoints(m_hWnd, &rect);				\
											__ctl.SetWindowPos(NULL, rect.left+(___offset), rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED); 
#define		TIMER_ID_PROGRESS_INFO		1001

static BOOL CreateToolTip(HWND nWndTool, CToolTipCtrl& ctlToolTipCtrl, int nIDS)
{
	ATLASSERT(ctlToolTipCtrl.m_hWnd == NULL);

	ctlToolTipCtrl.Create(::GetParent(nWndTool));

	CToolInfo ti(TTF_SUBCLASS, nWndTool, 0, NULL, MAKEINTRESOURCE(nIDS));
	
	return ctlToolTipCtrl.AddTool(ti);
}

typedef DWORD _ARGB;

static void InitBitmapInfo(__out_bcount(cbInfo) BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp)
{
	ZeroMemory(pbmi, cbInfo);

	pbmi->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes		= 1;
	pbmi->bmiHeader.biCompression	= BI_RGB;
	pbmi->bmiHeader.biWidth			= cx;
	pbmi->bmiHeader.biHeight		= cy;
	pbmi->bmiHeader.biBitCount		= bpp;
}

static HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;

	InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);

	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

static HRESULT AddBitmapToMenuItem(HMENU hmenu, int iItem, BOOL fByPosition, HBITMAP hbmp)
{
	HRESULT hr = E_FAIL;

	MENUITEMINFO mii = { sizeof(mii) };
	mii.fMask = MIIM_BITMAP;
	mii.hbmpItem = hbmp;

	if (SetMenuItemInfo(hmenu, iItem, fByPosition, &mii))
	{
		hr = S_OK;
	}
	return hr;
}

static HRESULT MyLoadIconMetric(HINSTANCE hinst, PCWSTR pszName, int lims, __out HICON *phico)
{
	HRESULT	hr		= E_FAIL;
	HMODULE hMod	= LoadLibraryW(L"Comctl32.dll");

	if (hMod != NULL)
	{
		typedef HRESULT (WINAPI *_LoadIconMetric)(HINSTANCE, PCWSTR, int, __out HICON*);

		_LoadIconMetric pLoadIconMetric = (_LoadIconMetric)GetProcAddress(hMod, "LoadIconMetric");

		if (pLoadIconMetric != NULL)
		{
			hr = pLoadIconMetric(hinst, pszName, lims, phico);
		}
	}
	return hr;
}

static void AddIconToMenu(HMENU hMenu, int nPos, HINSTANCE hinst, PCWSTR pszName)
{
	IWICImagingFactory* pFactory = NULL;	
 
	HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
 
	if (SUCCEEDED(hr))
	{
		HICON hicon = NULL;

		if (SUCCEEDED(MyLoadIconMetric(hinst, pszName, 0 /*LIM_SMALL*/, &hicon)))
		{
			IWICBitmap *pBitmap = NULL;
 
			hr = pFactory->CreateBitmapFromHICON(hicon, &pBitmap);
 
			if (SUCCEEDED(hr))
			{
				IWICFormatConverter *pConverter = NULL;
				
				hr = pFactory->CreateFormatConverter(&pConverter);

				if (SUCCEEDED(hr))
				{
					hr = pConverter->Initialize(pBitmap, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);

					UINT cx;
					UINT cy;
 
					hr = pBitmap->GetSize(&cx, &cy);
 
					if (SUCCEEDED(hr))
					{
						HBITMAP		hbmp	= NULL;
						const SIZE	sizIcon = { (int)cx, -(int)cy };
 
						BYTE* pbBuffer = NULL;
 
						hr = Create32BitHBITMAP(NULL, &sizIcon, reinterpret_cast<void **>(&pbBuffer), &hbmp);
 
						if (SUCCEEDED(hr))
						{
							const UINT cbStride = cx * sizeof(_ARGB);
							const UINT cbBuffer = cy * cbStride;
 
							hr = pBitmap->CopyPixels(NULL, cbStride, cbBuffer, pbBuffer);

							if (SUCCEEDED(hr))
							{
								AddBitmapToMenuItem(hMenu, nPos, TRUE, hbmp);
							}
							else
							{
								DeleteObject(hbmp);
							}
						}
					}
					pConverter->Release();
				}
				pBitmap->Release();
			}
		}
		pFactory->Release();
	}
}

void CMainDlg::TerminateRedirection()
{
	if (m_hRedirData != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hRedirData);
		m_hRedirData = INVALID_HANDLE_VALUE;
	}
	if (m_hRedirProcess != NULL)
		CHairTunes::TerminateRedirection(m_hRedirProcess);
}

bool CMainDlg::StartRedirection()
{
	if (m_bSoundRedirection && !m_strSoundRedirection.IsEmpty() && m_hRedirData == INVALID_HANDLE_VALUE && m_hRedirProcess == NULL)
	{
		return CHairTunes::StartRedirection(m_strSoundRedirection, &m_hRedirProcess, &m_hRedirData);
	}
	return false;
}

void CMainDlg::Elevate(bool bWithChangeApName /*= false*/, bool bWithChangeExtended /*= false*/)
{
	if (is_streaming())
	{
		USES_CONVERSION;

		WTL::CString strMsg;
		ATLVERIFY(strMsg.LoadStringW(IDS_INTERRUPT_PLAY));

		if (IDNO == ::MessageBox(m_hWnd, strMsg, A2CT(strConfigName.c_str()), MB_ICONQUESTION|MB_YESNO))
			return;
	}
	TCHAR FilePath[MAX_PATH];
		
	ATLVERIFY(GetModuleFileName(NULL, FilePath, sizeof(FilePath)/sizeof(TCHAR)) > 0);

	WTL::CString strParameters(_T("/Elevation"));

	if (bPrivateConfig)
	{
		USES_CONVERSION;
		strParameters += _T(" /Config:");
		strParameters += A2CT(strConfigName.c_str());
	}
	if (bWithChangeApName)
		strParameters += _T(" /WithChangeApName");
	if (bWithChangeExtended)
		strParameters += _T(" /WithChangeExtended");

	if (RunAsAdmin(m_hWnd, FilePath, strParameters))
	{
		PostMessage(WM_COMMAND, IDCANCEL);
	}
}

void CMainDlg::ReadConfig()
{
	USES_CONVERSION;

	string _strApName;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "ApName", _strApName))
		m_strApName = A2CW_CP(_strApName.c_str(), CP_UTF8);

	string _strStartMinimized;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "StartMinimized", _strStartMinimized))
		m_bStartMinimized = _strStartMinimized != "no" ? TRUE : FALSE;

	string _strAutostart;
	string _strAutostartName(strConfigName);
	
	if (bPrivateConfig)
		_strAutostartName = string("Shairport4w_") + strConfigName;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, _strAutostartName.c_str(), _strAutostart, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"))
		m_bAutostart = _strAutostart.empty() ? FALSE : TRUE;

	string _strTray;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "Tray", _strTray))
		m_bTray = _strTray != "no" ? TRUE : FALSE;

	string _strPassword;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "Password", _strPassword))
		m_strPassword = _strPassword.c_str();

	string _strSoundRedirection;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "SoundRedirection", _strSoundRedirection))
	{
		m_strSoundRedirection = _strSoundRedirection.c_str();

		if (m_strSoundRedirection == WTL::CString(_T("no redirection")))
			m_strSoundRedirection.LoadString(IDS_NO_REDIRECTION);
	}

	string _strIsSoundRedirection;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "IsSoundRedirection", _strIsSoundRedirection))
	{
		m_bSoundRedirection = _strIsSoundRedirection == "yes" ? true : false;
	}	
	string _strRedirKeepAlive;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "RedirKeepAlive", _strRedirKeepAlive))
		m_bRedirKeepAlive = _strRedirKeepAlive != "no" ? TRUE : FALSE;

	string _strRedirStartEarly;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "RedirStartEarly", _strRedirStartEarly))
		m_bRedirStartEarly = _strRedirStartEarly != "no" ? TRUE : FALSE;

	string _strLogToFile;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "LogToFile", _strLogToFile))
		m_bLogToFile = _strLogToFile != "no" ? TRUE : FALSE;

	string _strNoMetaInfo;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "NoMetaInfo", _strNoMetaInfo))
		m_bNoMetaInfo = _strNoMetaInfo != "no" ? TRUE : FALSE;	
	
	if (m_bNoMetaInfo)
	{
		// we need this feature for recording
		if (CShairportRecorder::IsLoaded())
			m_bNoMetaInfo = FALSE;
	}
	string _strNoMediaControl;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "NoMediaControl", _strNoMediaControl))
		m_bNoMediaControl = _strNoMediaControl != "no" ? TRUE : FALSE;	

	if (m_bNoMediaControl)
	{
		// we need this feature for recording
		if (CShairportRecorder::IsLoaded())
			m_bNoMediaControl = FALSE;
	}
	string _strSoundcardId;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "SoundcardId", _strSoundcardId))
	{
		m_strSoundcardId	= _strSoundcardId;
		CHairTunes::SetSoundId(m_strSoundcardId.c_str());
	}
	string _strStartFillPos;

	if (GetValueFromRegistry(HKEY_LOCAL_MACHINE, "StartFillPos", _strStartFillPos))
	{
		int n = atoi(_strStartFillPos.c_str());

		if (n > MAX_FILL)
			n = MAX_FILL;
		else if (n < MIN_FILL)
			n = MIN_FILL;

		CHairTunes::SetStartFill(n);
	}

	string _strInfoPanel;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "InfoPanel", _strInfoPanel))
		m_bInfoPanel = _strInfoPanel != "no" ? TRUE : FALSE;

	if (!m_bInfoPanel)
	{
		// we need this feature for recording
		if (CShairportRecorder::IsLoaded())
			m_bInfoPanel = TRUE;
	}
	string _strPin;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "Pin", _strPin))
		m_bPin = _strPin != "no" ? TRUE : FALSE;

	string _strTimeMode;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "TimeMode", _strTimeMode))
		 m_strTimeMode = _strTimeMode;

	string _strTrayTrackInfo;

	if (GetValueFromRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", _strTrayTrackInfo))
		m_bTrayTrackInfo = _strTrayTrackInfo != "no" ? TRUE : FALSE;
}

bool CMainDlg::WriteConfig()
{
	char buf[256];

	USES_CONVERSION;

	PutValueToRegistry(HKEY_CURRENT_USER, "InfoPanel", m_bInfoPanel ? "yes" : "no");
	PutValueToRegistry(HKEY_CURRENT_USER, "Pin", m_bPin ? "yes" : "no");
	PutValueToRegistry(HKEY_CURRENT_USER, "TimeMode", m_strTimeMode.c_str());
	PutValueToRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", m_bTrayTrackInfo ? "yes" : "no");

	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "ApName", W2CA_CP(m_strApName, CP_UTF8)))
		return false;
	else
	{
		string strHwAddr;

		EncodeBase64(hw_addr, 6, strHwAddr, false);
		ATLASSERT(!strHwAddr.empty());
		PutValueToRegistry(HKEY_LOCAL_MACHINE, "HwAddr", strHwAddr.c_str());
	}
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "StartMinimized", m_bStartMinimized ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "Tray", m_bTray ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "Password", T2CA(m_strPassword)))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "SoundRedirection", T2CA(m_strSoundRedirection)))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "IsSoundRedirection", m_bSoundRedirection ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "RedirKeepAlive", m_bRedirKeepAlive ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "RedirStartEarly", m_bRedirStartEarly ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "LogToFile", m_bLogToFile ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "NoMetaInfo", m_bNoMetaInfo ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "NoMediaControl", m_bNoMediaControl ? "yes" : "no"))
		return false;
	if (!PutValueToRegistry(HKEY_LOCAL_MACHINE, "StartFillPos", itoa(CHairTunes::GetStartFill(), buf, 10)))
		return false;
	if (!m_strSoundcardId.empty())
	{
		PutValueToRegistry(HKEY_LOCAL_MACHINE, "SoundcardId", m_strSoundcardId.c_str());
		CHairTunes::SetSoundId(m_strSoundcardId.c_str());
	}
	else
	{
		RemoveValueFromRegistry(HKEY_LOCAL_MACHINE, "SoundcardId");
		CHairTunes::SetSoundId(NULL);
	}

	char FilePath[MAX_PATH];
	ATLVERIFY(GetModuleFileNameA(NULL, FilePath, sizeof(FilePath)) > 0);

	if (m_bAutostart)
	{
		string _strAutostart = string("\"") + string(FilePath) + string("\"");

		if (bPrivateConfig)
		{
			_strAutostart += " /Config:";
			_strAutostart += strConfigName;
		}
		string _strAutostartName(strConfigName);
	
		if (bPrivateConfig)
			_strAutostartName = string("Shairport4w_") + strConfigName;
		PutValueToRegistry(HKEY_LOCAL_MACHINE, _strAutostartName.c_str(), _strAutostart.c_str(), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	}
	else
	{
		string _strAutostartName(strConfigName);
	
		if (bPrivateConfig)
			_strAutostartName = string("Shairport4w_") + strConfigName;
		RemoveValueFromRegistry(HKEY_LOCAL_MACHINE, _strAutostartName.c_str(), "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	}
	return true;
}

void CMainDlg::StopStartService(bool bStopOnly /* = false */)
{
	if (is_streaming())
	{
		::MessageBoxA(m_hWnd, "Current stream will be interrupted for reconfiguration, sorry!", strConfigName.c_str(), MB_ICONINFORMATION|MB_OK);
	}
	CWaitCursor wait;

	stop_serving();

	if (!bStopOnly)
	{
		Sleep(1000);
		start_serving();
	}
}

void CMainDlg::PutMMState(typeMMState nMmState)
{
	if (_m_nMMState != nMmState)
	{
		ATLASSERT(sizeof(unsigned int) == sizeof(_m_nMMState));
		InterlockedExchange((volatile unsigned int *)&_m_nMMState, (unsigned int)nMmState);
	
		BOOL bDummy = true;
		OnSetMultimediaState(0, 0, 0, bDummy);
	}
}

bool CMainDlg::SendDacpCommand(const char* strCmd, bool bRetryIfFailed /* = true */)
{
	ATLASSERT(strCmd != NULL);
	bool bResult = false;

	shared_ptr<CDacpService> dacpService;

	if (HasMultimediaControl(&dacpService))
	{
		CSocketBase client;

		Log("Trying to connect to DACP Host %s:%lu ... ", dacpService->m_strDacpHost.c_str(), (ULONG)ntohs(dacpService->m_nDacpPort));

		BOOL bConnected = client.Connect(dacpService->m_strDacpHost.c_str(), dacpService->m_nDacpPort);

		if (!bConnected)
		{
			dacpService->m_Event.Lock();
			dacpService->m_strDacpHost = dacpService->m_Event.m_strHost;
			dacpService->m_Event.Unlock();

			bConnected = client.Connect(dacpService->m_strDacpHost.c_str(), dacpService->m_nDacpPort);
		}
		if (bConnected)
		{
			Log("succeeded\n");

			CHttp request;

			request.Create();

			request.m_strMethod						= "GET";
			request.m_strUrl						= ci_string("/ctrl-int/1/") + ci_string(strCmd);
			request.m_mapHeader["Host"]				= m_strHostname + string(".local.");
			request.m_mapHeader["Active-Remote"]	= m_strActiveRemote.c_str();

			string strRequest = request.GetAsString(false);
						
			Log("Sending DACP request: %s\n", strCmd);

			client.Send(strRequest.c_str(), strRequest.length());
						
			if (client.WaitForIncomingData(2000))
			{
				char buf[256];

				memset(buf, 0, sizeof(buf));
				client.recv(buf, sizeof(buf)-1);

				Log("Received DACP response: %s\n", buf);

				if (strstr(buf, "Forbidden"))
				{
					bResult = false;
					SetDacpID("", "");
					Log("Erased current DACP-ID, since we are not allowed to control this client\n");
				}
				else
					bResult = strstr(buf, "OK") != NULL ? true : false;
			}
			else
			{
				Log("Waiting for DACP response *timedout*\n");
			}
			client.Close();
		}
		else
		{
			Log("*failed*\n");

			if (bRetryIfFailed && dacpService->m_Event.IsValid())
			{
				Log("The DACP service port may have changed - try to resolve service again ... ");
							
				if (dacpService->m_Event.Resolve(this))
					bResult = SendDacpCommand(strCmd, false);
				else
					Log("Re-resolve failed!\n");
			}
		}
	}
	return bResult;
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if(m_hAccelerator != NULL)
	{
		if(::TranslateAccelerator(m_hWnd, m_hAccelerator, pMsg))
			return TRUE;
	}
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateMenuBar();
	return FALSE;
}

void CMainDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	m_bVisible = bShow ? true : false;

	if (bShow)
		OnPin(0, 0, 0);
}

void CMainDlg::OnPin(UINT, int nID, HWND)
{
	if (nID != 0)
	{
		m_bPin = !m_bPin;

		PutValueToRegistry(HKEY_CURRENT_USER, "Pin", m_bPin ? "yes" : "no");
	}
	m_ctlPushPin.SetPinned(m_bPin ? TRUE : FALSE);
	SetWindowPos(m_bPin ? HWND_TOPMOST : HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	USES_CONVERSION;

	ATLVERIFY(CMyThread::Start());

	if (m_strHostname.empty())
	{
		m_strHostname = strConfigName.c_str();
	}

	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	m_hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(m_hIconSmall, FALSE);

	m_hAccelerator			= AtlLoadAccelerators(IDR_MAINFRAME);

	m_hHandCursor			= LoadCursor(NULL, IDC_HAND);
	m_strTimeInfo			= STR_EMPTY_TIME_INFO;
	m_strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;

	m_bmWmc					= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_WMC));
	m_bmProgressShadow		= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_PROGRESS_SHADOW));
	m_bmAdShadow			= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_AD_SHADOW));
	m_bmArtShadow			= BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_ART_SHADOW));

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);
	UIAddMenuBar(m_hWnd);

	m_threadShow.m_hWnd = m_hWnd;
	ATLVERIFY(m_threadShow.Start());

	if (m_bTray)
	{
		InitTray(A2CT(strConfigName.c_str()), m_hIconSmall, IDR_TRAY); 
	}
	m_ctlPassword.Attach(GetDlgItem(IDC_PASSWORD));
	m_ctlSet.Attach(GetDlgItem(IDC_CHANGENAME));
	m_ctlPanelFrame.Attach(GetDlgItem(IDC_STATIC_PANEL));
	m_ctlControlFrame.Attach(GetDlgItem(IDC_CONTROL_PANEL));
	m_ctlStatusFrame.Attach(GetDlgItem(IDC_STATUS_PANEL));
	m_ctlArtist.Attach(GetDlgItem(IDC_STATIC_ARTIST));
	m_ctlAlbum.Attach(GetDlgItem(IDC_STATIC_ALBUM));
	m_ctlTrack.Attach(GetDlgItem(IDC_STATIC_TRACK));
#ifdef _WITH_PROXY
	m_ctlProxy.Attach(GetDlgItem(IDC_PROXY));
	m_ctlProxy.EnableWindow();
	m_ctlProxy.ShowWindow(SW_NORMAL);
#endif
	m_ctlTimeInfo.Attach(GetDlgItem(IDC_STATIC_TIME_INFO));
	m_ctlPushPin.SubclassWindow(GetDlgItem(IDC_PUSH_PIN));
	m_ctlInfoBmp.SubclassWindow(GetDlgItem(IDC_INFO_BMP));
	m_ctlTimeBarBmp.SubclassWindow(GetDlgItem(IDC_TIME_BAR));

	m_ctlStatusInfo.Attach(GetDlgItem(IDC_STATIC_STATUS));

	ATLVERIFY(m_strReady.LoadString(ATL_IDS_IDLEMESSAGE));
	m_ctlStatusInfo.SetWindowText(m_strReady);

	CRect rect;
	
	m_ctlStatusFrame.GetClientRect(rect);
	m_ctlStatusFrame.MapWindowPoints(m_hWnd, rect);
	m_ctlStatusFrame.MoveWindow(rect.left, rect.top-1, rect.Width(), rect.Height()+1);

	m_ctlTimeBarBmp.GetClientRect(&rect);
	m_ctlTimeBarBmp.MapWindowPoints(m_hWnd, &rect);

	rect.left	-= 4;
	rect.top	-= 3;
	rect.right	+= 7;
	rect.bottom	+= 7;

	m_ctlTimeBarBmp.MoveWindow(rect, FALSE);

	m_ctlInfoBmp.SetWindowPos(NULL, 0, 0, 114, 114, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

	m_ctlInfoBmp.GetClientRect(&rect);
	m_ctlInfoBmp.MapWindowPoints(m_hWnd, &rect);

	rect.left	-= 3;
	rect.top	-= 3;
	rect.right	+= 8;
	rect.bottom	+= 9;

	m_ctlInfoBmp.MoveWindow(rect, FALSE);
	 
	GetDlgItem(IDC_INFOPANEL).EnableWindow(!CShairportRecorder::IsLoaded() && (m_bNoMetaInfo ? FALSE : TRUE));

	m_ctlPlay.Set(IDB_PLAY, IDB_PLAY_PRESSED, IDB_PLAY_DISABLED);
	m_ctlPause.Set(IDB_PAUSE, IDB_PAUSE_PRESSED, IDB_PAUSE_DISABLED);
	m_ctlFFw.Set(IDB_FFW, IDB_FFW_PRESSED, IDB_FFW_DISABLED);
	m_ctlRew.Set(IDB_REW, IDB_REW_PRESSED, IDB_REW_DISABLED);
	m_ctlSkipNext.Set(IDB_SKIP_NEXT, IDB_SKIP_NEXT_PRESSED, IDB_SKIP_NEXT_DISABLED);
	m_ctlSkipPrev.Set(IDB_SKIP_PREV, IDB_SKIP_PREV_PRESSED, IDB_SKIP_PREV_DISABLED);
	m_ctlMute.Set(IDB_MUTE, IDB_MUTE_PRESSED, IDB_MUTE);
	m_ctlMute.Add(IDB_MUTE_PRESSED);

	DoDataExchange(FALSE);

	Button_SetElevationRequiredState(m_ctlSet, TRUE);
	m_ctlSet.GetClientRect(&rect);
	m_ctlSet.MapWindowPoints(m_hWnd, &rect);
	m_ctlSet.SetWindowPos(NULL, rect.left, rect.top-1, rect.Width(), rect.Height()+2, SWP_NOZORDER | SWP_FRAMECHANGED);

	AddIconToMenu(GetSubMenu(GetMenu(), 1), 0, NULL, IDI_SHIELD);

	WTL::CString strMainFrame;

	if (!strMainFrame.LoadString(IDR_MAINFRAME) || strMainFrame != WTL::CString(A2CT(strConfigName.c_str())))
		SetWindowText(A2CT(strConfigName.c_str()));

	OnClickedInfoPanel(0, 0, NULL);

	SendMessage(WM_RAOP_CTX); 
	SendMessage(WM_SET_MMSTATE); 

	ATLVERIFY(CreateToolTip(m_ctlTrack, m_ctlToolTipTrackName, IDS_TRACK_LABEL));
	ATLVERIFY(CreateToolTip(m_ctlAlbum, m_ctlToolTipAlbumName, IDS_ALBUM_LABEL));
	ATLVERIFY(CreateToolTip(m_ctlArtist, m_ctlToolTipArtistName, IDS_ARTIST_LABEL));

	UISetCheck(ID_EDIT_TRAYTRACKINFO, m_bTrayTrackInfo);

	if (m_bRedirStartEarly)
	{
		if (!StartRedirection())
		{
			Log("Early Start of Redirection Process \"%s\" failed!", T2CA(m_strSoundRedirection));
		}
	}
	if (CShairportRecorder::IsLoaded())
	{
		REPLACE_CONTROL_HORZ(m_ctlPlay		, -20);
		REPLACE_CONTROL_HORZ(m_ctlPause		, -20);
		REPLACE_CONTROL_HORZ(m_ctlFFw		, -20);
		REPLACE_CONTROL_HORZ(m_ctlRew		, -20);
		REPLACE_CONTROL_HORZ(m_ctlSkipNext	, -20);
		REPLACE_CONTROL_HORZ(m_ctlSkipPrev	, -20);
		REPLACE_CONTROL_HORZ(m_ctlMute		, -20);

		bHandled = FALSE;
		SetMsgHandled(FALSE);
	}
#ifdef SPR_PLUGIN_INIT
#ifdef _DEBUG
	m_strSetupVersion = L"1.0.0.1";
#endif
	SPR_PLUGIN_INIT
#endif
	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
#ifdef SPR_PLUGIN_INIT
	m_ExitThread.Stop();
#endif
	CMyThread::Stop();

	m_threadShow.Stop();
	
	if (CShairportRecorder::IsLoaded())
	{
		bHandled = FALSE;
		SetMsgHandled(FALSE);
	}
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	if (m_bmWmc != NULL)
	{
		delete m_bmWmc;
		m_bmWmc = NULL;
	}
	if (m_bmProgressShadow != NULL)
	{
		delete m_bmProgressShadow;
		m_bmProgressShadow = NULL;
	}
	if (m_bmAdShadow != NULL)
	{
		delete m_bmAdShadow;
		m_bmAdShadow = NULL;
	}
	if (m_bmArtShadow != NULL)
	{
		delete m_bmArtShadow;
		m_bmArtShadow = NULL;
	}
	if (m_pRaopContext)
		m_pRaopContext.reset();
	
	m_mtxDacpService.Lock();
	m_mapDacpService.clear();
	m_mtxDacpService.Unlock();

	TerminateRedirection();

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WCHAR	FileName[MAX_PATH];
	CAboutDlg dlg;

	if (GetModuleFileNameW(NULL, FileName, sizeof(FileName)/sizeof(WCHAR)) > 0)
	{
		WCHAR strVersion[256];

		if (GetVersionInfo(FileName, strVersion, _countof(strVersion)))
			dlg.m_strVersion = strVersion;
	}
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	if (CShairportRecorder::IsOkToClose(m_hWnd))
	{
		DestroyWindow();
		::PostQuitMessage(nVal);
	}
}

void CMainDlg::OnClickedSetApName(UINT, int, HWND)
{
	if (!IsAdmin())
	{
		Elevate(true);
		return;
	}
	ChangeNameDlg dialog(m_strApName, m_strPassword);

	dialog.m_bTray				= m_bTray;
	dialog.m_bStartMinimized	= m_bStartMinimized;
	dialog.m_bAutostart			= m_bAutostart;

	if (dialog.DoModal() == IDOK)
	{
		bool bWriteConfig = false;

		if (m_strApName != dialog.getAirportName() || m_strPassword != dialog.getPassword())
		{
			m_strApName			= dialog.getAirportName();
			m_strPassword		= dialog.getPassword();

			bWriteConfig = true;
			StopStartService();
		}
		if (m_bStartMinimized != dialog.m_bStartMinimized)
		{
			m_bStartMinimized	= dialog.m_bStartMinimized;
			bWriteConfig		= true;
		}
		if (m_bAutostart != dialog.m_bAutostart)
		{
			m_bAutostart		= dialog.m_bAutostart;
			bWriteConfig		= true;
		}
		if (m_bTray	!= dialog.m_bTray)
		{
			bWriteConfig	= true;
			m_bTray			= dialog.m_bTray;

			if (m_bTray)
			{
				USES_CONVERSION;

				InitTray(A2CT(strConfigName.c_str()), m_hIconSmall, IDR_TRAY); 
			}
			else
				RemoveTray();
		}
		DoDataExchange(FALSE);

		if (bWriteConfig)	
			WriteConfig();
	}
}

void CMainDlg::OnSysCommand(UINT wParam, CPoint pt)
{
	SetMsgHandled(FALSE);

	if (m_bTray)
	{
		if (GET_SC_WPARAM(wParam) == SC_MINIMIZE)
		{
			ShowWindow(SW_HIDE);		
			SetMsgHandled(TRUE);

			string strDummy;
			
			if (!GetValueFromRegistry(HKEY_CURRENT_USER, "TrayAck", strDummy))
			{
				WTL::CString strAck;
				USES_CONVERSION;

				ATLVERIFY(strAck.LoadString(IDS_TRAY_ACK));
				
				SetInfoText(A2CT(strConfigName.c_str()), strAck);
				PutValueToRegistry(HKEY_CURRENT_USER, "TrayAck", "ok");
			}
		}
	}
}

void CMainDlg::OnShow(UINT, int, HWND)
{
	ShowWindow(SW_SHOWNORMAL);
	::SetForegroundWindow(m_hWnd);
}

void CMainDlg::OnClickedExtendedOptions(UINT, int, HWND)
{
	if (!IsAdmin())
	{
		Elevate(false, true);
		return;
	}
	CExtOptsDlg dlg(m_bLogToFile, m_bNoMetaInfo, m_bNoMediaControl, m_strSoundRedirection, m_bSoundRedirection, m_bRedirKeepAlive, m_bRedirStartEarly, CHairTunes::GetStartFill(), m_strSoundcardId.empty() ? 0 : (UINT)atoi(m_strSoundcardId.c_str()));

	if (dlg.DoModal(m_hWnd) == IDOK)
	{
		USES_CONVERSION;

		const char* strSoundRedirection = T2CA(dlg.getSoundRedirection());
		
		for (long n = 0; n < 20; ++n)
		{
			char	buf[64];
			string	str;

			sprintf_s(buf, 64, "RecentRedir%ld", n);
			
			if (!GetValueFromRegistry(HKEY_CURRENT_USER, buf, str))
			{
				PutValueToRegistry(HKEY_CURRENT_USER, buf, strSoundRedirection);
				break;
			}
			else if (strcmp(str.c_str(), strSoundRedirection) == 0)
				break;
		}
		string _strSoundcardId;

		if (dlg.getSoundcardId() > 0)
		{
			char buf[64];

			sprintf_s(buf, 64, "%ld", dlg.getSoundcardId());
			_strSoundcardId = buf;
		}
		const bool bRestartService =	(m_bLogToFile &&		!dlg.getLogToFile())
									||	(m_bNoMetaInfo			!= dlg.getNoMetaInfo())
									||	(m_bNoMediaControl		!= dlg.getNoMediaControls())
									||	(m_strSoundRedirection	!= dlg.getSoundRedirection())
									||  (m_bSoundRedirection    != dlg.m_bSoundRedirection)
									||	(m_bRedirKeepAlive		!= dlg.getRedirKeepAlive())
									||	(m_strSoundcardId		!= _strSoundcardId)
									? true : false;

		if (m_bNoMetaInfo != dlg.getNoMetaInfo())
		{
			m_bNoMetaInfo	= dlg.getNoMetaInfo();
			m_bInfoPanel	= m_bNoMetaInfo ? FALSE : TRUE;

			DoDataExchange(FALSE);
			OnClickedInfoPanel(0, 0, NULL);

			GetDlgItem(IDC_INFOPANEL).EnableWindow(m_bInfoPanel);
		}
		if (	m_strSoundRedirection	!= dlg.getSoundRedirection()
			||	m_bSoundRedirection		!= dlg.m_bSoundRedirection
			||	m_bRedirKeepAlive		!= dlg.getRedirKeepAlive()
			||	m_bRedirStartEarly		&& !dlg.getRedirKeepAlive()
			)
		{
			StopStartService(true);
			TerminateRedirection();
		}

		m_bNoMediaControl		= dlg.getNoMediaControls();
		m_bLogToFile			= dlg.getLogToFile();
		m_bNoMetaInfo			= dlg.getNoMetaInfo();
		m_strSoundRedirection	= dlg.getSoundRedirection();
		m_bSoundRedirection		= dlg.m_bSoundRedirection;
		m_strSoundcardId		= _strSoundcardId;
		m_bRedirKeepAlive		= dlg.getRedirKeepAlive();
		m_bRedirStartEarly		= dlg.getRedirStartEarly();

		if (m_bNoMediaControl)
		{
			// we need this feature for recording
			if (CShairportRecorder::IsLoaded())
				m_bNoMediaControl = FALSE;
		}
		CHairTunes::SetStartFill(dlg.getPos());

		WriteConfig();

		if (m_bRedirStartEarly)
			StartRedirection();

		if (bRestartService)
		{
			StopStartService();
		}
	}
}

void CMainDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
		case TIMER_ID_PROGRESS_INFO:
		{
			WTL::CString strTimeInfo		= STR_EMPTY_TIME_INFO;
			WTL::CString strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;

			if (m_pRaopContext)
			{
				m_pRaopContext->Lock();

				if (m_pRaopContext->m_nTimeStamp != 0 && m_pRaopContext->m_nTimeTotal != 0)
				{
					time_t nCurPosInSecs = m_pRaopContext->m_nTimeCurrentPos 
										+ (::time(NULL) - m_pRaopContext->m_nTimeStamp)
										- 2;
					if (nCurPosInSecs >= 0)
					{
						int nNewCurPos = (int) ((nCurPosInSecs * 100) / m_pRaopContext->m_nTimeTotal);

						if (nNewCurPos > 100)
							nNewCurPos = 100;
	
						if (nNewCurPos != m_nCurPos)
						{
							m_nCurPos = nNewCurPos;
							m_ctlTimeBarBmp.Invalidate();
						}

						// allow max 2 secs overtime
						if (nCurPosInSecs - m_pRaopContext->m_nTimeTotal > 2)
							nCurPosInSecs = m_pRaopContext->m_nTimeTotal+2;

						if (m_pRaopContext->m_nDurHours == 0)
						{
							long nMinCurrent = (long)(nCurPosInSecs / 60);
							long nSecCurrent = (long)(nCurPosInSecs % 60);

							if (m_strTimeMode == string("time_total"))
							{
								strTimeInfoCurrent.Format(_T("%02d:%02d")	, nMinCurrent               , nSecCurrent);
								strTimeInfo.Format		 (_T("%02d:%02d")	, m_pRaopContext->m_nDurMins, m_pRaopContext->m_nDurSecs);
							}
							else
							{
								long nRemain = m_pRaopContext->m_nTimeTotal >= nCurPosInSecs 
																? (long)(m_pRaopContext->m_nTimeTotal - nCurPosInSecs)
																: (long)(nCurPosInSecs - m_pRaopContext->m_nTimeTotal);

								long nMinRemain = nRemain / 60;
								long nSecRemain = nRemain % 60;

								strTimeInfoCurrent.Format(_T("%02d:%02d"), nMinCurrent               , nSecCurrent);
								strTimeInfo.Format		 (_T("%s%02d:%02d")
															, nCurPosInSecs > m_pRaopContext->m_nTimeTotal ? _T("+") : _T("-")																			
															, nMinRemain				, nSecRemain);
							}
						}
						else
						{
							long	nHourCurrent  = (long)(nCurPosInSecs / 3600);
							time_t _nCurPosInSecs = nCurPosInSecs - (nHourCurrent*3600);

							long nMinCurrent = (long)(_nCurPosInSecs / 60);
							long nSecCurrent = (long)(_nCurPosInSecs % 60);

							if (m_strTimeMode == string("time_total"))
							{
								strTimeInfoCurrent.Format(_T("%02d:%02d:%02d")
															, nHourCurrent               , nMinCurrent               , nSecCurrent);
								strTimeInfo.Format		 (_T("%02d:%02d:%02d")
															, m_pRaopContext->m_nDurHours, m_pRaopContext->m_nDurMins, m_pRaopContext->m_nDurSecs);
							}
							else
							{
								long nRemain = m_pRaopContext->m_nTimeTotal >= nCurPosInSecs
																? (long)(m_pRaopContext->m_nTimeTotal - nCurPosInSecs)
																: (long)(nCurPosInSecs - m_pRaopContext->m_nTimeTotal);

								long nHourRemain = nRemain / 3600;
								nRemain -= (nHourRemain*3600);

								long nMinRemain = nRemain / 60;
								long nSecRemain = nRemain % 60;

								strTimeInfoCurrent.Format(_T("%02d:%02d:%02d")	
															, nHourCurrent               , nMinCurrent               , nSecCurrent);
								strTimeInfo.Format		 (_T("%s%02d:%02d:%02d")
															, nCurPosInSecs > m_pRaopContext->m_nTimeTotal ? _T("+") : _T("-")
															, nHourRemain				 , nMinRemain				 , nSecRemain);
							}
						}
					}
				}
				m_pRaopContext->Unlock();
			}
			if (m_strTimeInfo != strTimeInfo)
			{
				m_strTimeInfo = strTimeInfo;
				DoDataExchange(FALSE, IDC_STATIC_TIME_INFO);
			}
			if (m_strTimeInfoCurrent != strTimeInfoCurrent)
			{
				m_strTimeInfoCurrent = strTimeInfoCurrent;
				DoDataExchange(FALSE, IDC_STATIC_TIME_INFO_CURRENT);

				if (m_strTimeInfoCurrent == STR_EMPTY_TIME_INFO)
				{
					if (0 != m_nCurPos)
					{
						m_nCurPos = 0;
						m_ctlTimeBarBmp.Invalidate();
					}
				}
			}
			return;
		}
		break;
	}
	SetMsgHandled(FALSE);
}

void  CMainDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CWindow wnd = ::RealChildWindowFromPoint(m_hWnd, point);

	if (wnd.m_hWnd == m_ctlTimeInfo.m_hWnd)
	{
		if (m_pRaopContext && m_strTimeInfo != STR_EMPTY_TIME_INFO)
		{
			if (m_strTimeMode == string("time_total"))
				m_strTimeMode = string("time_remaining");
			else
				m_strTimeMode = string("time_total");

			OnTimer(TIMER_ID_PROGRESS_INFO);
			PutValueToRegistry(HKEY_CURRENT_USER, "TimeMode", m_strTimeMode.c_str());
		}
	}
	else if (wnd.m_hWnd == m_ctlInfoBmp.m_hWnd)
	{
		if (m_bAd)
		{
			CWaitCursor wait;

			ShellExecuteW(m_hWnd, L"open", WTL::CString(L"http://www.lightweightdream.com/sp4w/shairport4w/?lang=")+GetLanguageAbbr(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
	SetMsgHandled(FALSE);
}

LRESULT CMainDlg::Pause(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// omit pause in case we're receiving this in order to a "FLUSH" while skipping
	if (wParam == 0 || (m_nMMState != skip_next && m_nMMState != skip_prev))
	{
		if (m_nMMState != pause)
		{
			KillTimer(TIMER_ID_PROGRESS_INFO);
			m_nMMState = pause;
		}
	}
	return 0l;
}

LRESULT CMainDlg::Resume(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// omit play in case we're receiving this in order to a "SET_PARAMETER/progress" while ffw or rew
	if (wParam == 0 || (m_nMMState != ffw && m_nMMState != rew))
	{
		if (m_nMMState != play)
		{
			m_nMMState = play;
			SetTimer(TIMER_ID_PROGRESS_INFO, 1000);

			if (m_pRaopContext)
			{
				m_pRaopContext->Lock();
				ci_string strPeerIP = m_pRaopContext->m_strPeerIP;
				m_pRaopContext->Unlock();

				m_mtxQueryConnectedHost.Lock();
		
				m_listQueryConnectedHost.push_back(strPeerIP);
				SetEvent(m_hEvent);

				m_mtxQueryConnectedHost.Unlock();
			}
		}
	}
	return 0l;
}

void CMainDlg::OnEvent()
{
	ci_string strPeerIP;

	m_mtxQueryConnectedHost.Lock();
	
	if (!m_listQueryConnectedHost.empty())
	{
		strPeerIP = m_listQueryConnectedHost.front();
		m_listQueryConnectedHost.pop_front();
	}
	m_mtxQueryConnectedHost.Unlock();

	if (!strPeerIP.empty())
	{
		wstring strHostName;

		if (GetPeerName(strPeerIP.c_str(), strPeerIP.find('.') != ci_string::npos ? true : false, strHostName))
		{
			PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new WTL::CString(strHostName.c_str()));
		}
		else
		{
			USES_CONVERSION;

			// wait 2 secs
			if (WaitForSingleObject(m_hStopEvent, 2000) == WAIT_OBJECT_0)
				return;

			shared_ptr<CDacpService> dacpService;

			if (HasMultimediaControl(&dacpService) && !dacpService->m_strDacpHost.empty())
			{
				PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new WTL::CString(A2CW_CP(dacpService->m_strDacpHost.c_str(), CP_UTF8)));
			}
			else
			{
				PostMessage(WM_SET_CONNECT_STATE, 0, (LPARAM) new WTL::CString(A2CW(strPeerIP.c_str())));
			}
		}
	}
	else
	{
		ATLASSERT(FALSE);
	}
}

LRESULT CMainDlg::OnSetConnectedState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam != 0)
	{
		WTL::CString* pstrHostName = (WTL::CString*) lParam;

		if (m_pRaopContext)
		{
			WTL::CString strStatusText;

			ATLVERIFY(strStatusText.LoadString(IDS_STATUS_CONNECTED));

			int nLocalStuff = pstrHostName->Find(_T(".local."));

			if (nLocalStuff > 0)
				*pstrHostName = pstrHostName->Left(nLocalStuff);

			strStatusText += (*pstrHostName);

			strStatusText += _T("\"");
			m_ctlStatusInfo.SetWindowText(strStatusText);
		}
		else
		{
			m_ctlStatusInfo.SetWindowText(m_strReady);
		}
		delete pstrHostName;
	}
	else
	{
		if (m_pRaopContext)
		{
			WTL::CString strStatusText;

			ATLVERIFY(strStatusText.LoadString(IDS_STATUS_CONNECTED));
			int n = strStatusText.Find(_T(' '));

			if (n > 0)
			{
				strStatusText = strStatusText.Left(n);
				m_ctlStatusInfo.SetWindowText(strStatusText);
			}
			else
			{
				ATLASSERT(FALSE);
			}
		}
		else
			m_ctlStatusInfo.SetWindowText(m_strReady);
	}
	return 0l;
}

LRESULT CMainDlg::OnSetRaopContext(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam != NULL)
	{
		if (m_pRaopContext)
			m_pRaopContext.reset();
		m_pRaopContext = *(shared_ptr<CRaopContext>*) lParam;
	}
	else
	{
		m_pRaopContext.reset();
	}
	PostMessage(WM_SET_CONNECT_STATE);

	// prepare info bmp
	m_ctlInfoBmp.Invalidate();

	// prepare time info
	m_strTimeInfo			= STR_EMPTY_TIME_INFO;
	m_strTimeInfoCurrent	= STR_EMPTY_TIME_INFO;
	m_nCurPos				= 0;

	m_ctlTimeBarBmp.Invalidate();
	DoDataExchange(FALSE, IDC_STATIC_TIME_INFO);
	DoDataExchange(FALSE, IDC_STATIC_TIME_INFO_CURRENT);

	// prepare song info
	BOOL bDummy = TRUE;
	OnSongInfo(0, 0, 0, bDummy);

	if (m_pRaopContext)
	{
		Resume();
	}
	else
	{
		Pause();
		PostMessage(WM_SET_MMSTATE);
	}
	return 0l;
}

LRESULT CMainDlg::OnSongInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	WTL::CString	str_daap_songalbum;
	WTL::CString	str_daap_songartist;	
	WTL::CString	str_daap_trackname;

	CRaopContext* pRaopContext = NULL;

	if (lParam)
	{
		pRaopContext = (CRaopContext*)lParam;
	}
	else
	{
		if (m_pRaopContext)
			pRaopContext = m_pRaopContext.get();
	}

	if (pRaopContext)
	{
		pRaopContext->Lock();

		str_daap_songalbum		= pRaopContext->m_str_daap_songalbum;
		str_daap_songartist		= pRaopContext->m_str_daap_songartist;
		str_daap_trackname		= pRaopContext->m_str_daap_trackname;
		
		pRaopContext->Unlock();
	}
	if (!lParam)
	{
		m_ctlArtist.SetWindowText(str_daap_songartist);
		m_ctlAlbum.SetWindowText(str_daap_songalbum);
		m_ctlTrack.SetWindowText(str_daap_trackname);

		m_ctlArtist.EnableWindow(!str_daap_songartist.IsEmpty());
		m_ctlAlbum.EnableWindow(!str_daap_songalbum.IsEmpty());
		m_ctlTrack.EnableWindow(!str_daap_trackname.IsEmpty());
	}
	if (	m_bTray && m_bTrayTrackInfo
		&&	(	!str_daap_songartist.IsEmpty()
			 || !str_daap_songalbum.IsEmpty()
			 || !str_daap_trackname.IsEmpty()
			 )
		)
	{
		if (!m_bVisible)
		{
			WTL::CString	strFmtNowPlaying;
			WTL::CString	strTrackLabel;
			WTL::CString	strAlbumLabel;
			WTL::CString	strArtistLabel;
			WTL::CString	strNowPlayingLabel;

			WTL::CString	strSep(_T(":\t"));
			WTL::CString	strNewLine(_T("\n"));

			ATLVERIFY(strTrackLabel.LoadString(IDS_TRACK_LABEL));
			ATLVERIFY(strAlbumLabel.LoadString(IDS_ALBUM_LABEL));
			ATLVERIFY(strArtistLabel.LoadString(IDS_ARTIST_LABEL));
			ATLVERIFY(strNowPlayingLabel.LoadString(IDS_NOW_PLAYING));

			if (!str_daap_songartist.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strArtistLabel + strSep + str_daap_songartist);
			}
			if (!str_daap_trackname.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strTrackLabel + strSep + str_daap_trackname);
			}
			if (!str_daap_songalbum.IsEmpty())
			{
				if (!strFmtNowPlaying.IsEmpty())
					strFmtNowPlaying += strNewLine;

				strFmtNowPlaying += (strAlbumLabel + strSep + str_daap_songalbum);
			}
			HICON hIcon = NULL;

			if (pRaopContext)
			{
				if (!pRaopContext->GetHICON(&hIcon))
				{
					hIcon = NULL;
				}
			}			
			if (!g_bMute || lParam)
				SetInfoText(strNowPlayingLabel, strFmtNowPlaying, hIcon);

			if (hIcon != NULL)
				::DestroyIcon(hIcon);
		}
	}
	return 0l;
}

void CMainDlg::OnInfoBmpPaint(HDC wParam)
{
	CRect rect;

	if (m_ctlInfoBmp.GetUpdateRect(&rect))
	{
		CPaintDC dcPaint(m_ctlInfoBmp.m_hWnd);

		if (!dcPaint.IsNull())
		{
			CBrush br;

			br.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));

			dcPaint.FillRect(&rect, br);

			Graphics dcGraphics(dcPaint);

			CRect rectClient;

			m_ctlInfoBmp.GetClientRect(&rectClient);

			rect = rectClient;

			rect.left	+= 3;
			rect.top	+= 3;
			rect.right	-= 8;
			rect.bottom -= 9;

			Rect rClient(0, 0, rectClient.Width(), rectClient.Height());
			Rect r(rect.left, rect.top, rect.Width(), rect.Height());

			bool bDrawn = false;

			if (m_pRaopContext)
			{
				m_pRaopContext->Lock();

				if (m_pRaopContext->m_bmInfo != NULL)
				{
					dcGraphics.DrawImage(m_bmArtShadow, rClient);
					bDrawn = dcGraphics.DrawImage(m_pRaopContext->m_bmInfo, r) == Gdiplus::Ok ? true : false;
				}
				m_pRaopContext->Unlock();
			}
			if (!bDrawn)
			{
				m_bAd = true;

				CBrush			brShadow;
				brShadow.CreateSolidBrush(::GetSysColor(COLOR_BTNSHADOW));

				CBrushHandle oldBrush = dcPaint.SelectBrush(brShadow);
				
				dcPaint.RoundRect(rect, CPoint(16, 16));
				dcPaint.SelectBrush(oldBrush);

				dcGraphics.DrawImage(m_bmAdShadow, rClient);

				r.Inflate(-6, -6);
				dcGraphics.DrawImage(m_bmWmc, r);
			}
			else
				m_bAd = false;
		}
	}
}

void CMainDlg::OnTimeBarBmpPaint(HDC wParam)
{
	CRect rect;

	if (m_ctlTimeBarBmp.GetUpdateRect(&rect))
	{
		CPaintDC dcPaint(m_ctlTimeBarBmp.m_hWnd);

		if (!dcPaint.IsNull())
		{
			Graphics		dcGraphics(dcPaint);
			CBrush			brFace;
			CBrush			brShadow;
			CBrush			brBar;
			CBrushHandle	oldBrush;
			CPoint			rnd(6, 6);
			CRect			rectClient;

			m_ctlTimeBarBmp.GetClientRect(&rectClient);

			Rect rClient(0, 0, rectClient.Width(), rectClient.Height());

			rect		= rectClient;
			rect.left	+= 4;
			rect.top	+= 3;
			rect.right	-= 7;
			rect.bottom -= 7;

			brFace.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
			brShadow.CreateSolidBrush(::GetSysColor(COLOR_BTNSHADOW));
			brBar.CreateSolidBrush(::GetActiveWindow() == m_hWnd ? ::GetSysColor(COLOR_GRADIENTACTIVECAPTION) : ::GetSysColor(COLOR_GRADIENTINACTIVECAPTION));
			
			CRgn rgn;

			rgn.CreateRoundRectRgn(rect.left, rect.top, rect.right+1, rect.bottom+1, rnd.x, rnd.y);

			dcPaint.SelectClipRgn(rgn, RGN_DIFF);
			dcPaint.FillRect(&rectClient, brFace);
			
			dcGraphics.DrawImage(m_bmProgressShadow, rClient);

			dcPaint.SelectClipRgn(rgn);

			CRect rectLeft(rect.TopLeft(), CSize((rect.Width() * m_nCurPos) / 100, rect.Height()));

			if (rectLeft.Width() > 2)
			{
				CRgn rgn1;

				rgn1.CreateRoundRectRgn(rectLeft.left, rectLeft.top, rectLeft.right+1, rectLeft.bottom+1, rnd.x, rnd.y);

				dcPaint.SelectClipRgn(rgn1, RGN_DIFF);

				oldBrush = dcPaint.SelectBrush(brShadow);
				dcPaint.RoundRect(rect, rnd);

				dcPaint.SelectClipRgn(rgn1);
				
				dcPaint.SelectBrush(brBar);
				dcPaint.RoundRect(rectLeft, rnd);
				
				dcPaint.SelectBrush(oldBrush);
			}
			else
			{
				oldBrush = dcPaint.SelectBrush(brShadow);
				dcPaint.RoundRect(rect, rnd);
				dcPaint.SelectBrush(oldBrush);
			}
		}
	}
}

void CMainDlg::OnActivate(UINT nState, BOOL bMinimized, CWindow wndOther)
{
	m_ctlTimeBarBmp.Invalidate();
	SetMsgHandled(FALSE);
}

BOOL CMainDlg::OnAppCommand(HWND, short cmd, WORD uDevice, int dwKeys)
{
	if (HasMultimediaControl() && (!CShairportRecorder::IsLoaded() || (g_bMute & MUTE_FROM_PLUGIN) == 0))
	{
		switch(cmd)
		{
			case APPCOMMAND_MEDIA_NEXTTRACK:
			{
				if (m_ctlSkipNext.IsWindowEnabled())
				{
					OnSkipNext(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_STOP:
			case APPCOMMAND_MEDIA_PAUSE:
			{
				if (m_ctlPause.IsWindowEnabled())
				{
					OnPause(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_FAST_FORWARD:
			{
				if (m_ctlFFw.IsWindowEnabled())
				{
					OnForward(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PLAY:
			{
				if (m_ctlPlay.IsWindowEnabled())
				{
					OnPlay(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PLAY_PAUSE:
			{
				if (m_ctlPlay.IsWindowEnabled())
				{
					OnPlay(0, 0, NULL);
					return TRUE;
				}
				else if (m_ctlPause.IsWindowEnabled())
				{
					OnPause(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_PREVIOUSTRACK:
			{
				if (m_ctlSkipPrev.IsWindowEnabled())
				{
					OnSkipPrev(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_MEDIA_REWIND:
			{
				if (m_ctlRew.IsWindowEnabled())
				{
					OnRewind(0, 0, NULL);
					return TRUE;
				}
			}
			break;

			case APPCOMMAND_VOLUME_MUTE:
			{
				if (m_ctlMute.IsWindowEnabled())
				{
					OnMute(0, 0, NULL);
					return TRUE;
				}
			}
			break;		
		}
	}
	SetMsgHandled(FALSE);
	return FALSE;
}


LRESULT CMainDlg::OnPowerBroadcast(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetMsgHandled(FALSE);

	Log("OnPowerBroadcast: %d\n", wParam);

	// Prevent suspend or standby is we are streaming
	// Note that you only get to this message when system is timing out.  You
	// will not get it when a user forces a standby or suspend
	if ((wParam == PBT_APMQUERYSUSPEND || wParam == PBT_APMQUERYSTANDBY) &&
		is_streaming())
	{
		SetMsgHandled(TRUE);
		Log("OnPowerBroadcast: System wants to suspend and since we are streaming we deny the request\n");
		return BROADCAST_QUERY_DENY;
	}

	// If we did not get the query to know if we deny the standby/suspend
	// we have to close all clients because we will be suspended anyways
	// and all our connections will be closed by the OS.
	if ((wParam == PBT_APMSUSPEND || wParam == PBT_APMSTANDBY) &&
		is_streaming())
	{
		SetMsgHandled(TRUE);

		Log("OnPowerBroadcast: System is going to suspend and since we are streaming we will close the connections\n");

		stop_serving();
		Sleep(1000);
	}

	if (wParam == PBT_APMRESUMESUSPEND ||
		wParam == PBT_APMRESUMESTANDBY)
	{
		SetMsgHandled(TRUE);
		StopStartService();
	}
	return 0;
}

BOOL CMainDlg::OnSetCursor(CWindow wnd, UINT nHitTest, UINT message)
{
	if (m_bAd)
	{
		CPoint pt;

		if (GetCursorPos(&pt))
		{
			ScreenToClient(&pt);

			CWindow wnd = ::RealChildWindowFromPoint(m_hWnd, pt);

			if (wnd.m_hWnd == m_ctlInfoBmp.m_hWnd)
			{
				SetCursor(m_hHandCursor);
				return TRUE;
			}
		}
	}
	SetMsgHandled(FALSE);
	return FALSE;
}

LRESULT CMainDlg::OnInfoBitmap(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_ctlInfoBmp.Invalidate();
	return 0l;
}

void CMainDlg::OnClickedInfoPanel(UINT, int, HWND)
{
	const int nOffsetValue = 134;

	DoDataExchange(TRUE, IDC_INFOPANEL);

	PutValueToRegistry(HKEY_CURRENT_USER, "InfoPanel", m_bInfoPanel ? "yes" : "no");

	CRect rect;
	GetWindowRect(&rect);

	if ((!m_bNoMetaInfo && m_bInfoPanel) && rect.Height() < 320)
	{
		SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()+nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlPanelFrame.GetWindowRect(&rect);
		m_ctlPanelFrame.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()+nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlControlFrame.GetClientRect(&rect);
		m_ctlControlFrame.MapWindowPoints(m_hWnd, &rect);
		m_ctlControlFrame.SetWindowPos(NULL, rect.left, rect.top+nOffsetValue, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);

		REPLACE_CONTROL_VERT(m_ctlPlay,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlPause,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlFFw,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlRew,		nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipNext,	nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipPrev,	nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlMute	,	nOffsetValue)

		m_ctlInfoBmp.ShowWindow(SW_NORMAL);
		m_ctlArtist.ShowWindow(SW_NORMAL);
		m_ctlAlbum.ShowWindow(SW_NORMAL);
		m_ctlTrack.ShowWindow(SW_NORMAL);

		m_ctlInfoBmp.Invalidate();
	}
	else if ((m_bNoMetaInfo || !m_bInfoPanel) && rect.Height() > 280)
	{
		SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()-nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlPanelFrame.GetWindowRect(&rect);
		m_ctlPanelFrame.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height()-nOffsetValue, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

		m_ctlControlFrame.GetClientRect(&rect);
		m_ctlControlFrame.MapWindowPoints(m_hWnd, &rect);
		m_ctlControlFrame.SetWindowPos(NULL, rect.left, rect.top-nOffsetValue, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED);

		REPLACE_CONTROL_VERT(m_ctlPlay,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlPause,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlFFw,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlRew,		-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipNext,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlSkipPrev,	-nOffsetValue)
		REPLACE_CONTROL_VERT(m_ctlMute	,	-nOffsetValue)

		m_ctlInfoBmp.ShowWindow(SW_HIDE);
		m_ctlArtist.ShowWindow(SW_HIDE);
		m_ctlAlbum.ShowWindow(SW_HIDE);
		m_ctlTrack.ShowWindow(SW_HIDE);
	}
}

#ifdef _WITH_PROXY

LRESULT CMainDlg::OnRaopRegistered(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	ATLASSERT(lParam != NULL);

	CRegisterServiceEvent* pEvent = (CRegisterServiceEvent*) lParam;

	Log("Service %sregistered: %s, %s\n", pEvent->m_bRegister ? "" : "de", pEvent->m_strService.c_str(), pEvent->m_strRegType.c_str());

	if (pEvent->m_bRegister)
	{
		if (pEvent->Resolve(this))
		{
			USES_CONVERSION;

			string strApName;
			ParseRegEx(pEvent->m_strService.c_str(), "[^@]+@{.+}", strApName);

			if (!strApName.empty() && ci_string(strApName.c_str()) != ci_string(T2CA(m_strApName)))
			{
				int nCount = m_ctlProxy.GetCount();

				if (nCount > 0)
				{
					for (int n = 0; n < nCount; n++)
					{
						CRegisterServiceEvent* pItemData = (CRegisterServiceEvent*)m_ctlProxy.GetItemData(n);

						if (*pItemData == *pEvent)
						{
							delete pEvent;
							return 0l;
						}
					}
				}
				m_ctlProxy.SetItemDataPtr(m_ctlProxy.AddString(A2CT(strApName.c_str())), pEvent);
				pEvent = NULL;
			}
		}
	}
	else
	{
		int nCount = m_ctlProxy.GetCount();

		if (nCount > 0)
		{
			for (int n = 0; n < nCount; n++)
			{
				CRegisterServiceEvent* pItemData = (CRegisterServiceEvent*)m_ctlProxy.GetItemData(n);

				if (*pItemData == *pEvent)
				{
					delete pItemData;
					m_ctlProxy.DeleteString(n);
					break;
				}
			}
		}
	}
	if (pEvent != NULL)
		delete pEvent;

	return 0l;
}
#endif

LRESULT CMainDlg::OnDACPRegistered(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	ATLASSERT(lParam != NULL);

	CRegisterServiceEvent* pEvent = (CRegisterServiceEvent*) lParam;

	Log("Service %sregistered: %s, %s\n", pEvent->m_bRegister ? "" : "de", pEvent->m_strService.c_str(), pEvent->m_strRegType.c_str());

	if (pEvent->m_bRegister)
	{
		pEvent->Resolve(this);
	}
	else
	{
		bool						bPostNewState = false;
		shared_ptr<CDacpService>	pDacpService;

		ULONGLONG DacpID = ToDacpID(pEvent->m_strService.c_str());

		m_mtxDacpService.Lock();

		if (DacpID == m_DACP_ID)
			bPostNewState = true;

		auto e = m_mapDacpService.find(DacpID);

		if (e != m_mapDacpService.end())
		{
			pDacpService = e->second;
			m_mapDacpService.erase(e);
		}
		m_mtxDacpService.Unlock();
		
		if (bPostNewState)
			PostMessage(WM_SET_MMSTATE);
	}
	delete pEvent;

	return 0l;
}

bool CMainDlg::OnServiceResolved(CRegisterServiceEvent* pServiceEvent, const char* strHost, USHORT nPort)
{
	ATLASSERT(pServiceEvent != NULL);

	Log("OnServiceResolved: %s  -  Host: %s : %lu\n", pServiceEvent->m_strService.c_str(), strHost, (ULONG)ntohs(nPort));

#ifdef _WITH_PROXY
	if (pServiceEvent->m_strRegType == ci_string("_raop._tcp."))
	{
		pServiceEvent->m_strHost	= strHost;
		pServiceEvent->m_nPort		= nPort;
		return true;
	}
#endif
	bool bPostNewState = false;

	ULONGLONG DacpID = ToDacpID(pServiceEvent->m_strService.c_str());

	m_mtxDacpService.Lock();

	bool bNewItem = m_mapDacpService.find(DacpID) == m_mapDacpService.end() ? true : false;

	m_mapDacpService[DacpID] = make_shared<CDacpService>(strHost, nPort, pServiceEvent);
	ATLVERIFY(m_mapDacpService[DacpID]->m_Event.QueryHostAddress());

	if (bNewItem && DacpID == m_DACP_ID)
		bPostNewState = true;

	m_mtxDacpService.Unlock();

	if (bPostNewState)
		PostMessage(WM_SET_MMSTATE);
	
	return true;
}

LRESULT CMainDlg::OnSetMultimediaState(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (m_ctlPlay.IsWindow())
	{
		if (HasMultimediaControl())
		{
			switch(_m_nMMState)
			{
				case pause:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(false);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case play:
				{
					m_ctlPlay.EnableWindow(false);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(true);
					m_ctlRew.EnableWindow(true);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case ffw:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;

				case rew:
				{
					m_ctlPlay.EnableWindow(true);
					m_ctlPause.EnableWindow(true);
					m_ctlFFw.EnableWindow(false);
					m_ctlRew.EnableWindow(false);
					m_ctlSkipNext.EnableWindow(true);
					m_ctlSkipPrev.EnableWindow(true);
				}
				break;
			}
		}
		else
		{
			m_ctlPlay.EnableWindow(false);
			m_ctlPause.EnableWindow(false);
			m_ctlFFw.EnableWindow(false);
			m_ctlRew.EnableWindow(false);
			m_ctlSkipNext.EnableWindow(false);
			m_ctlSkipPrev.EnableWindow(false);
		}
	}
	return 0l;
}

bool CMainDlg::OnPlay(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		break;

		case pause:
		{
			if (bResult = SendDacpCommand("play"))
				Resume();
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnSkipPrev(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("previtem"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
				{
					m_nMMState = skip_prev;
				}
			}
		}
		break;

		case pause:
		{
			bResult = SendDacpCommand("previtem");
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();
			if (bResult = SendDacpCommand("previtem"))
				m_nMMState = skip_prev;
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnRewind(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("beginrew"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
				{
					m_nMMState = rew;
				}
			}
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnPause(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("pause"))
				Pause();
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();

			if (bResult = SendDacpCommand("pause"))
				Pause();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnForward(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("beginff"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
					m_nMMState = ffw;
			}
		}
		break;

		case pause:
		{
		}
		break;

		case ffw:
		case rew:
		{
			if (bResult = SendDacpCommand("playresume"))
				Resume();
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnSkipNext(UINT, int nID, HWND)
{
	bool bResult = false;

	switch(m_nMMState)
	{
		case play:
		{
			if (bResult = SendDacpCommand("nextitem"))
			{
				if (m_pRaopContext && m_pRaopContext->m_nTimeTotal > 0)
					m_nMMState = skip_next;
			}
		}
		break;

		case pause:
		{
			bResult = SendDacpCommand("nextitem");
		}
		break;

		case ffw:
		case rew:
		{
			if (SendDacpCommand("playresume"))
				Resume();
			if (bResult = SendDacpCommand("nextitem"))
				m_nMMState = skip_next;
		}
		break;
	}
	return bResult;
}

bool CMainDlg::OnMute(UINT, int nID, HWND)
{
	g_bMute ^= MUTE_FROM_TOGGLE_BUTTON;
	m_ctlMute.SetSelected(g_bMute & MUTE_FROM_TOGGLE_BUTTON);
	return true;
}

bool CMainDlg::OnMuteOnOff(UINT, int nID, HWND)
{
	if (IDC_MM_MUTE_ON == nID)
	{
		if ((g_bMute & MUTE_FROM_PLUGIN) == 0)
		{
			g_bMute |= MUTE_FROM_PLUGIN;
		}
	}
	else
	{
		ATLASSERT(nID == IDC_MM_MUTE_OFF);
		if (g_bMute & MUTE_FROM_PLUGIN)
		{
			g_bMute &= ~MUTE_FROM_PLUGIN;
		}
	}
	return true;
}

void CMainDlg::OnEditTrayTrackInfo(UINT, int, HWND)
{
	m_bTrayTrackInfo = !m_bTrayTrackInfo;
	UISetCheck(ID_EDIT_TRAYTRACKINFO, m_bTrayTrackInfo);

	PutValueToRegistry(HKEY_CURRENT_USER, "TrayTrackInfo", m_bTrayTrackInfo ? "yes" : "no");
}

void CMainDlg::OnRefresh(UINT uMsg /*= 0*/, int nID /*= 0*/, HWND hWnd /*= NULL*/)
{
	Invalidate();
	SetMsgHandled(FALSE);
}

void CMainDlg::OnOnlineUpdate(UINT uMsg /*= 0*/, int nID /*= 0*/, HWND hWnd /*= NULL*/)
{
#ifdef SPR_PLUGIN_OLU
	CWaitCursor wait;

	CAtlHttpClient	version;		
					
	if (version.Navigate(SPR_PLUGIN_OLU + GetLanguageAbbr() + _T("/current.txt")))
	{
		DWORD dwLen = version.GetBodyLength();
		
		if (dwLen && dwLen < 1024)
		{
			USES_CONVERSION;
			
			char buf[1024];
			
			memset(buf, 0, sizeof(buf));
			memcpy(buf, version.GetBody(), dwLen);
			
			ATLTRACE("Current available Version is: %s\n", buf);
			ATLASSERT(m_strSetupVersion.vt == VT_BSTR);
			
			char* s = strchr(buf, '\r');

			if (s)
			{
				*s++ = 0;

				if (*s == '\n')
					s++;
			}
			else
			{
				s = strchr(buf, '\n');

				if (s)
					*s++ = 0;
			}
			PCWSTR strVersion = A2CW(buf);

			if (wcscmp(OLE2CW(m_strSetupVersion.bstrVal), strVersion) < 0)
			{
				WTL::CString strNewVersionAvailable;
				ATLVERIFY(strNewVersionAvailable.LoadString(IDS_NEW_VERSION_AVAILABLE));
				
				if (s)
				{
					strNewVersionAvailable += _T("\r\n");
					strNewVersionAvailable += A2CT(s);
				}
	
				if (::MessageBoxW(m_hWnd, strNewVersionAvailable, L"Shairport4w", MB_YESNO|MB_ICONQUESTION) == IDYES)
				{
					CWaitCursor wait;
					
					CAtlHttpClient	setup;
					
					if (setup.Navigate(SPR_PLUGIN_OLU + _T("SP4W_Setup.exe")))
					{
						CSP4W_TemporaryFile tf(L".exe");
						
						if (SUCCEEDED(tf.Create()))
						{
							ATLTRACE(L"Create Temporyary file %s\n", (PCWSTR)tf.m_strTempFileName);
						
							DWORD dwWritten = 0;
						
							if (WriteFile(tf, setup.GetBody(), setup.GetBodyLength(), &dwWritten, NULL) && dwWritten == setup.GetBodyLength())
							{
								tf.Close();
								
								if (VerifyEmbeddedSignature(tf.m_strTempFileName))
								{
									CComVariant varTempFile = GetValueFromRegistry(HKEY_CURRENT_USER, L"TEMP_SETUP_FILE");
									
									if (varTempFile.vt == VT_BSTR)
									{								
										ATLVERIFY(DeleteInstalledFile(varTempFile.bstrVal));
									}
									ATLVERIFY(PutValueToRegistry(HKEY_CURRENT_USER, L"TEMP_SETUP_FILE", (PCWSTR)tf.m_strTempFileName));
									
									if (!RunAsAdmin(m_hWnd, tf.m_strTempFileName, L"/Install /Restart", false))
									{
										ErrorMsg(m_hWnd, L"Run downloaded Setup", GetLastError());
									}
								}
								else
								{
									WTL::CString strMsg;
									ATLVERIFY(strMsg.LoadString(IDS_UPDATE_NOT_SIGNED));
									::MessageBoxW(m_hWnd, strMsg, L"Shairport4w", MB_OK|MB_ICONEXCLAMATION);
								}
							}
							else
							{
								WTL::CString strFunction;
								ATLVERIFY(strFunction.LoadString(IDS_ONLINE_UPDATE));
								ErrorMsg(m_hWnd, strFunction, GetLastError());
							}
						}
						else
						{
							WTL::CString strFunction;
							ATLVERIFY(strFunction.LoadString(IDS_ONLINE_UPDATE));
							ErrorMsg(m_hWnd, strFunction, GetLastError());
						}
					}
					else
					{
						WCHAR buf[256];
						
						WTL::CString strOnlineUpdateErrorFmt;
						ATLVERIFY(strOnlineUpdateErrorFmt.LoadString(IDS_DOWNLOAD_ERROR_FMT));
						swprintf_s(buf, 256, strOnlineUpdateErrorFmt, setup.GetLastError(), (long)setup.GetStatus());
						
						::MessageBoxW(m_hWnd, buf, L"Shairport4w", MB_OK|MB_ICONEXCLAMATION);
					}
				}
			}
			else
			{
				WTL::CString strMsg;
				ATLVERIFY(strMsg.LoadString(IDS_NO_NEW_VERSION));
				::MessageBoxW(m_hWnd, strMsg, L"Shairport4w", MB_OK|MB_ICONINFORMATION);
			}
		}
	}	
	else
	{
		WCHAR buf[256];
		
		WTL::CString strOnlineUpdateErrorFmt;
		ATLVERIFY(strOnlineUpdateErrorFmt.LoadString(IDS_UPDATE_ERROR_FMT));
		swprintf_s(buf, 256, strOnlineUpdateErrorFmt, version.GetLastError(), (long)version.GetStatus());
		
		::MessageBoxW(m_hWnd, buf, L"Shairport4w", MB_OK|MB_ICONEXCLAMATION);
	}
#endif
}
