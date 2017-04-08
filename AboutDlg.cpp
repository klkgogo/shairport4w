/*
 *
 *  AboutDlg.cpp 
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

WTL::CString	CAboutDlg::m_strRecorderVersion;

#define ABDLG_OFFSET	180

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	m_hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(m_hIconSmall, FALSE);

	DoDataExchange(FALSE);

	if (m_strRecorderVersion.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_SPR1).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_SPR2).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_RECORDER_VERSION).ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_SPR_ICON).ShowWindow(SW_HIDE);
		m_ctlIcons8.ShowWindow(SW_HIDE);

		CRect rect;

		GetDlgItem(IDC_STATIC_FRAME).GetClientRect(rect);
		GetDlgItem(IDC_STATIC_FRAME).SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height() - ABDLG_OFFSET, SWP_NOMOVE|SWP_NOZORDER);

		GetDlgItem(IDOK).GetClientRect(rect);
		GetDlgItem(IDOK).MapWindowPoints(m_hWnd, rect);
		GetDlgItem(IDOK).SetWindowPos(NULL, rect.left, rect.top-ABDLG_OFFSET, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

		GetWindowRect(rect);
		SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height() - ABDLG_OFFSET, SWP_NOMOVE|SWP_NOZORDER);
	}
	else
	{
		m_ctlIcons8.SetHyperLink(_T("http://icons8.com"));
	}
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
