/*
 *
 *  DacpService.h 
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

#pragma once

class CDacpService
{
public:
	CDacpService(void);
	CDacpService(const ci_string& strHost, USHORT nPort, CRegisterServiceEvent* pEvent)
	{
		m_strDacpHost	= strHost;
		m_nDacpPort		= nPort;

		if (pEvent != NULL)
			m_Event			= *pEvent;
	}
	CDacpService(const CDacpService& i)
	{
		operator=(i);
	}
	~CDacpService(void);

	CDacpService& operator=(const CDacpService& i)
	{
		m_strDacpHost	= i.m_strDacpHost;
		m_nDacpPort		= i.m_nDacpPort;
		m_Event			= i.m_Event;

		return *this;
	}

public:
	ci_string							m_strDacpHost;
	USHORT								m_nDacpPort;		// net order
	CRegisterServiceEvent				m_Event;
};

