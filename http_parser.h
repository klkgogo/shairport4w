/*
 *
 *  http_parser.h
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

class CHttp
{
public:
	CHttp()
	{
		m_nStatus = 0;
	}
	~CHttp()
	{
	}
	bool Parse(bool bRequest = true)
	{
		if (m_strBuf.length() == 0)
			return false;

		ULONG	nBuf	= m_strBuf.length() + 1;
		char*	pBuf	= new char[nBuf];
		
		if (pBuf != NULL)
		{
			map<long, string> mapByLine;

			string::size_type nSep = m_strBuf.find("\r\n\r\n");

			if (nSep == string::npos)
			{
				strcpy_s(pBuf, nBuf, m_strBuf.c_str());
				m_strBody.clear();
			}
			else
			{
				strcpy_s(pBuf, nBuf, m_strBuf.substr(0, nSep).c_str());
				m_strBody = m_strBuf.substr(nSep+4);
			}

			char* ctx	= NULL;
			char* p		= strtok_s(pBuf, "\r\n", &ctx);

			long n = 0;

			while (p != NULL)
			{
				if (strlen(p) > 0)
					mapByLine[n++] = p;

				p = strtok_s(NULL, "\r\n", &ctx);
			}
			for (map<long, string>::iterator i = mapByLine.begin(); i != mapByLine.end(); ++i)
			{
				if (i == mapByLine.begin())
				{
					strcpy_s(pBuf, nBuf, i->second.c_str());

					ctx	= NULL;
					p	= strtok_s(pBuf, " \t", &ctx);

					if (p != NULL)
					{
						if (bRequest)
							m_strMethod = p;
						else
							m_strProtocol = p;
	
						p = strtok_s(NULL, " \t", &ctx);

						if (p != NULL)
						{
							if (bRequest)
								m_strUrl = p;
							else
								m_nStatus = atol(p);

							p = strtok_s(NULL, " \t", &ctx);

							if (p != NULL)
							{
								if (bRequest)
									m_strProtocol = p;
								else
									m_strStatus = p;
							}
						}
					}
					else
					{
						// improve parser!
						ATLASSERT(FALSE);
					}
				}
				else
				{
					string::size_type n = i->second.find(':');

					if (n != string::npos)
					{
						ci_string	strTok		= i->second.substr(0, n).c_str();
						string		strValue	= i->second.substr(n+1);

						Trim(strTok);
						Trim(strValue);

						m_mapHeader[strTok] = strValue;
					}
				}
			}
			delete [] pBuf;
			return true;
		}
		return false;
	}
	void Create(LPCSTR strProtocol = "HTTP/1.1", long nStatus = 200, const char* strStatus = "OK")
	{
		m_nStatus		= nStatus;
		m_strProtocol	= strProtocol;
		m_strStatus		= strStatus != NULL ? strStatus : "OK";
	}
	void SetStatus(long nStatus, const char* strStatus)
	{
		m_nStatus		= nStatus;
		m_strStatus		= strStatus;
	}
	string GetAsString(bool bResponse = true)
	{
		string strResult;

		char buf[1024];

		if (bResponse)
		{
			sprintf_s(buf, 256, "%s %ld %s\r\n", m_strProtocol.c_str(), m_nStatus, m_strStatus.c_str());
			strResult += buf;
		}
		else
		{
			sprintf_s(buf, 256, "%s %s %s\r\n", m_strMethod.c_str(), m_strUrl.c_str(), m_strProtocol.c_str());
			strResult += buf;

			if (m_strBody.length() > 0)
			{
				if (_itoa_s(m_strBody.length(), buf, _countof(buf), 10) == ERROR_SUCCESS)
					m_mapHeader["content-length"] = buf;
			}
		}
		for (map<ci_string, string>::iterator i = m_mapHeader.begin(); i != m_mapHeader.end(); ++i)
		{
			strResult += i->first.c_str();
			strResult += ": ";
			strResult += i->second;
			strResult += "\r\n";
		}
		if (!bResponse)
		{
			strResult += "\r\n";
			strResult += m_strBody;
		}
		return strResult;
	}
	void InitNew()
	{
		m_mapHeader.clear();
		m_strMethod.clear();
		m_strUrl.clear();
		m_strProtocol.clear();
		m_strBody.clear();
		m_nStatus= 0;
		m_strStatus.clear();
		m_strBuf.clear();
	}
public:
	map<ci_string, string>	m_mapHeader;
	string					m_strMethod;
	ci_string				m_strUrl;
	ci_string				m_strProtocol;
	string					m_strBody;
	long					m_nStatus;
	string					m_strStatus;
	string					m_strBuf;
};
