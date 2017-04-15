/*
 *
 *  AboutDlg.h 
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

class CAboutDlg : public CDialogImpl<CAboutDlg>, public CWinDataExchange<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

   CAboutDlg(CAppModule& module)
      : _Module(module)
   {

   }
   CAppModule& _Module;

protected:
	BEGIN_DDX_MAP(CAboutDlg)
		DDX_TEXT(IDC_STATIC_VERSION, m_strVersion)
		DDX_TEXT(IDC_STATIC_RECORDER_VERSION, m_strRecorderVersion)
		DDX_CONTROL(IDC_STATIC_SHAIR_LNK, m_ctlShairport4w)
		DDX_CONTROL(IDC_STATIC_ICONS8_LNK, m_ctlIcons8)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	WTL::CString		m_strVersion;
	static WTL::CString	m_strRecorderVersion;

private:
	HICON				m_hIconSmall;
	CHyperLink			m_ctlShairport4w;
	CHyperLink			m_ctlIcons8;
};
