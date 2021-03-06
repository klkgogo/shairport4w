/*
 *
 *  TrayIcon.h
 *
 *  Copyright (C) Frank Friemel - Feb 2013
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

// The version of the structure supported by Shell v7
typedef struct _NOTIFYICONDATA_4 
{
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	TCHAR szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	TCHAR szInfo[256];
	union 
	{
		UINT uTimeout;
		UINT uVersion;
	} DUMMYUNIONNAME;
	TCHAR szInfoTitle[64];
	DWORD dwInfoFlags;
	GUID guidItem;
	HICON hBalloonIcon;
} NOTIFYICONDATA_4;

#ifndef NIIF_LARGE_ICON
#define NIIF_LARGE_ICON 0x00000020
#endif

#define WM_TRAYICON		(WM_APP+0)

template <class T>
class CTrayIconImpl
{
public:
	CTrayIconImpl()
	{
		m_bInit			= false;
		memset(&m_nid, 0, sizeof(m_nid));
	}
	~CTrayIconImpl()
	{
		RemoveTray();
	}
	bool InitTray(LPCTSTR lpszToolTip, HICON hIcon, UINT nID)
	{
		if (m_bInit)
			return true;

		T* pT = static_cast<T*>(this);
	
		_tcsncpy_s(m_nid.szTip, _countof(m_nid.szTip), lpszToolTip, _TRUNCATE);

		m_nid.cbSize			= bIsWinXP ? sizeof(NOTIFYICONDATA) : sizeof(NOTIFYICONDATA_4);
		m_nid.hWnd				= pT->m_hWnd;
		m_nid.uID				= nID;
		m_nid.hIcon				= hIcon;
		m_nid.uFlags			= NIF_MESSAGE | NIF_ICON | NIF_TIP;
		m_nid.uCallbackMessage	= WM_TRAYICON;
		m_bInit					= Shell_NotifyIcon(NIM_ADD, (PNOTIFYICONDATA)&m_nid) ? true : false;

		return m_bInit;
	}
	void UpdateTrayIcon(HICON hIcon)
	{
		if (m_bInit)
		{
			T* pT = static_cast<T*>(this);

			if (pT->m_bTray)
			{
				if (hIcon == NULL)
					hIcon = pT->m_hIconSmall;

				m_nid.uFlags	= NIF_ICON;
				m_nid.hIcon		= hIcon;

				Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);
			}
		}
	}
	void SetTooltipText(LPCTSTR pszTooltipText)
	{
		if (m_bInit && pszTooltipText != NULL)
		{
			m_nid.uFlags = NIF_TIP;
			_tcsncpy_s(m_nid.szTip, _countof(m_nid.szTip), pszTooltipText, _TRUNCATE);

			Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);
		}
	}
	void SetInfoText(LPCTSTR pszInfoTitle, LPCTSTR pszInfoText, HICON hIcon = NULL)
	{
		if (m_bInit && pszInfoText != NULL)
		{
			m_nid.uFlags		= NIF_INFO;
			m_nid.dwInfoFlags	= NIIF_USER | (hIcon != NULL && !bIsWinXP ? NIIF_LARGE_ICON : 0);
			m_nid.uTimeout		= 15000;

			if (pszInfoTitle != NULL)
				_tcsncpy_s(m_nid.szInfoTitle, _countof(m_nid.szInfoTitle), pszInfoTitle, _TRUNCATE);
			_tcsncpy_s(m_nid.szInfo, _countof(m_nid.szInfo), pszInfoText, _TRUNCATE);

			m_nid.hBalloonIcon = hIcon;

			Shell_NotifyIcon(NIM_MODIFY, (PNOTIFYICONDATA)&m_nid);
		}
	}
	void RemoveTray()
	{
		if (m_bInit)
		{
			m_nid.uFlags = 0;
			Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&m_nid);
			m_bInit = false;
		}
	}

	BEGIN_MSG_MAP(CTrayIconImpl<T>)
		MESSAGE_HANDLER(WM_TRAYICON, OnTrayIcon)
	END_MSG_MAP()

	LRESULT OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if (wParam != m_nid.uID)
			return 0;

		T* pT = static_cast<T*>(this);

		if (LOWORD(lParam) == WM_RBUTTONUP)
		{
			CMenu Menu;

			if (!Menu.LoadMenu(m_nid.uID))
				return 0;

			CMenuHandle Popup(Menu.GetSubMenu(0));

			CPoint pos;
			GetCursorPos(&pos);

			SetForegroundWindow(pT->m_hWnd);

			Popup.SetMenuDefaultItem(0, TRUE);

			Popup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, pT->m_hWnd);
			pT->PostMessage(WM_NULL);
			Menu.DestroyMenu();
		}
		else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
		{
			::ShowWindow(pT->m_hWnd, SW_SHOWNORMAL);
			::SetForegroundWindow(pT->m_hWnd);
		}
		return 0;
	}
protected:
	bool				m_bInit;
	NOTIFYICONDATA_4	m_nid;
};
