/*
 *
 *  MyBitmapButton.cpp 
 *
 *  Copyright (C) Frank Friemel - Mar 2013
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
#include "MyBitmapButton.h"

bool CMyBitmapButton::Resize(Bitmap& bmToResize, const CSize& szDest, HBITMAP* bmResult, COLORREF dwBkCol, bool bFillBk /* = false*/)
{
	LONG cx = szDest.cx > 0 ? szDest.cx : bmToResize.GetWidth();
	LONG cy = szDest.cy > 0 ? szDest.cy : bmToResize.GetHeight();

	Bitmap bm(cx, cy);

	// create a memdc
	Graphics* dcGraphics = Graphics::FromImage(&bm);

	if (bFillBk)
	{
		SolidBrush br(dwBkCol);
		dcGraphics->FillRectangle(&br, 0, 0, cx, cy);
	}

	// draw image into memory dc
	Rect r(0, 0, cx, cy);
	dcGraphics->DrawImage(&bmToResize, r);

	// done with the dc
	delete dcGraphics;

	return bm.GetHBITMAP(dwBkCol, bmResult) == Gdiplus::Ok ? true : false;
}

void CMyBitmapButton::Set(int nNormalID, int PressedID, int nDisabledID, HINSTANCE hInstance /*= _Module.GetResourceInstance()*/)
{
	m_bmNormal	= BitmapFromResource(hInstance, MAKEINTRESOURCE(nNormalID));

	if (m_bmNormal == NULL)
		return;

	m_bmDisabled= BitmapFromResource(hInstance, MAKEINTRESOURCE(nDisabledID));
	ATLASSERT(m_bmDisabled != NULL);
	m_bmPressed	= BitmapFromResource(hInstance, MAKEINTRESOURCE(PressedID));
	ATLASSERT(m_bmPressed != NULL);
	
	HBITMAP		hbm = NULL;
	Color		bck_col(::GetSysColor(COLOR_3DFACE));
	CImageList	il;

	il.Create(m_bmNormal->GetWidth(), m_bmNormal->GetHeight(), ILC_COLOR32|ILC_MASK, 0, 1);

	m_bmNormal->GetHBITMAP(bck_col, &hbm);
	il.Add(hbm);
	m_bmPressed->GetHBITMAP(bck_col, &hbm);
	il.Add(hbm);
	m_bmDisabled->GetHBITMAP(bck_col, &hbm);
	il.Add(hbm);

	this->SetImageList(il);
	this->SetImages(0, 1, -1, 2);
}

void CMyBitmapButton::Set(CSize szDest, int nNormalID, int PressedID, int nDisabledID, HINSTANCE hInstance /*= _Module.GetResourceInstance()*/)
{
	m_bmNormal	= BitmapFromResource(hInstance, MAKEINTRESOURCE(nNormalID));
	ATLASSERT(m_bmNormal != NULL);
	m_bmDisabled= BitmapFromResource(hInstance, MAKEINTRESOURCE(nDisabledID));
	ATLASSERT(m_bmDisabled != NULL);
	m_bmPressed	= BitmapFromResource(hInstance, MAKEINTRESOURCE(PressedID));
	ATLASSERT(m_bmPressed != NULL);
	
	HBITMAP		hbm = NULL;
	COLORREF	bck_col(::GetSysColor(COLOR_3DFACE));
	CImageList	il;

	il.Create(szDest.cx, szDest.cy, ILC_COLOR32|ILC_MASK, 0, 1);

	ATLVERIFY(Resize(*m_bmNormal, szDest, &hbm, bck_col));
	il.Add(hbm);
	ATLVERIFY(Resize(*m_bmPressed, szDest, &hbm, bck_col));
	il.Add(hbm);
	ATLVERIFY(Resize(*m_bmDisabled, szDest, &hbm, bck_col));
	il.Add(hbm);

	this->SetImageList(il);
	this->SetImages(0, 1, -1, 2);
}
