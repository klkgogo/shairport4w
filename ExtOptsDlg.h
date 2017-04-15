/*
 *
 *  ExtOptsDlg.h 
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

#pragma once

#include "resource.h"

class CExtOptsDlg :	public CDialogImpl<CExtOptsDlg>, public CWinDataExchange<CExtOptsDlg>
{
public:
	CExtOptsDlg(
      CAppModule& module,
      BOOL bLogToFile, 
      BOOL bNoMetaInfo, 
      BOOL bNoMediaControls, 
      WTL::CString strSoundRedirection, 
      bool bSoundRedirection, 
      bool bRedirKeepAlive, 
      bool bRedirStartEarly, 
      int nPos, 
      int nSoundcardId = 0);
	~CExtOptsDlg(void);

	enum { IDD = IDD_EXT_OPTS };

	// Accessors
	WTL::CString	getSoundRedirection();
	BOOL			getLogToFile();
	BOOL			getNoMetaInfo();
	BOOL			getNoMediaControls();
	int				getSoundcardId() { return m_nSoundcardId; }
	int				getPos();
	bool			getRedirKeepAlive();
	bool			getRedirStartEarly();

	bool IsSoundRedirection();

protected:
	BEGIN_DDX_MAP(CMainDlg)
		DDX_CHECK(IDC_LOG_TO_FILE, m_bLogToFile)
		DDX_TEXT(IDC_REDIRECTION, m_strSoundRedirection)
		DDX_CHECK(IDC_NO_META_INFO, m_bNoMetaInfo)
		DDX_CHECK(IDC_NO_NEDIA_CONTROLS, m_bNoMediaControls)
		DDX_CHECK(IDC_REDIR_KEEPALIVE, m_bRedirKeepAlive)
		DDX_CHECK(IDC_IS_SOUND_REDIR, m_bSoundRedirection)
		DDX_CHECK(IDC_REDIR_START_EARLY, m_bRedirStartEarly)
	END_DDX_MAP()

	BEGIN_MSG_MAP_EX(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		MSG_WM_HSCROLL(OnHScroll)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		COMMAND_HANDLER_EX(IDC_REDIRECTION, CBN_SELENDOK, OnSelEndOkRedir)
		COMMAND_ID_HANDLER_EX(IDC_REDIR_KEEPALIVE, OnClickedRedirKeepAlive)
		COMMAND_ID_HANDLER_EX(IDC_IS_SOUND_REDIR, OnClickedRedirectTo)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void OnHScroll(int, short, HWND);
	void OnSelEndOkRedir(UINT, int, HWND); 
	void OnClickedRedirKeepAlive(UINT, int, HWND);
	void OnClickedRedirectTo(UINT, int, HWND);

private:
   CAppModule& _Module;
	BOOL								m_bLogToFile;
	int									m_nSoundcardId;
	int									m_nPos;
	BOOL								m_bNoMetaInfo;
	BOOL								m_bNoMediaControls;
	_CSTRING_NS::CString				m_strSoundRedirection;
	CComboBox							m_ctlSoundRedirection;
	CComboBox							m_ctlSoundcard;
	HICON								m_hIconSmall;
	CTrackBarCtrl						m_ctlBuffering;
	CStatic								m_ctlBufferingLabel;
	bool								m_bRedirKeepAlive;
	bool								m_bRedirStartEarly;
	CButton								m_ctlRedirKeepAlive;
	CButton								m_ctlRedirStartEarly;
	CComboBox							m_ctlRedirection;
public:	
	bool								m_bSoundRedirection;
};

