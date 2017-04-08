/*
 *
 *  utils.cpp
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
#include <Strsafe.h>

typedef DWORD (APIENTRY *_GetFileVersionInfoSizeW)(
        LPCWSTR lptstrFilename, /* Filename of version stamped file */
        LPDWORD lpdwHandle
        );    
typedef BOOL (APIENTRY *_GetFileVersionInfoW)(
        LPCWSTR lptstrFilename, /* Filename of version stamped file */
        DWORD dwHandle,         /* Information from GetFileVersionSize */
        DWORD dwLen,            /* Length of buffer for info */
        LPVOID lpData
        );           

typedef BOOL (APIENTRY *_VerQueryValueW)(
        const LPVOID pBlock,
        LPWSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );


static CMyMutex		c_mtxLOG;
static USHORT		c_nUniquePortCounter	= 6000;
static CMyMutex		c_mtxUniquePortCounter;

string			strConfigName(APP_NAME_A);
bool			bPrivateConfig	= false;
bool			bIsWinXP		= false;

bool Log(LPCSTR pszFormat, ...)
{
	if (bLogToFile)
	{
		char	buf[32768];
		bool	bResult = true;

		va_list ptr;
		va_start(ptr, pszFormat);

		c_mtxLOG.Lock();

		try
		{
			vsprintf_s(buf, sizeof(buf), pszFormat, ptr);
			OutputDebugStringA(buf);
			va_end(ptr);

			if (bLogToFile)
			{
				char FilePath[MAX_PATH];
		
				ATLVERIFY(GetModuleFileNameA(NULL, FilePath, sizeof(FilePath)) > 0);
				FilePath[strlen(FilePath)-3] = 'l';
				FilePath[strlen(FilePath)-2] = 'o';
				FilePath[strlen(FilePath)-1] = 'g';

				WTL::CString strFilePath(FilePath);

				FILE* fh = NULL;
					
				if (_tfopen_s(&fh, strFilePath, _T("a")) == ERROR_SUCCESS)
				{
					// Get size of file
					fseek(fh, 0, SEEK_END);
					long fSize = ftell(fh);
					rewind(fh);
				
					if (fSize > 1024*1024)
					{
						fclose(fh);
						_tremove(strFilePath + ".bak");
						_trename(strFilePath, strFilePath + ".bak");
						
						if (_tfopen_s(&fh, strFilePath, _T("a")) != ERROR_SUCCESS)
							fh = NULL;
					}
				}
				else
					fh = NULL;
				if (fh != NULL)
				{
					time_t szClock;

					time(&szClock);
					struct tm	newTime;
					localtime_s(&newTime, &szClock);

					strftime(FilePath, MAX_PATH, "%x %X ", &newTime);
					fwrite(FilePath, sizeof(char), strlen(FilePath), fh);
					fwrite(buf, sizeof(char), strlen(buf), fh);
					fclose(fh);
				}
				else
				{
					bResult = false;
				}
			}
		}
		catch (...)
		{
			// catch that silently
		}
		c_mtxLOG.Unlock();

		return bResult;
	}
	return true;
}

bool GetValueFromRegistry(HKEY hKey, LPCSTR pValueName, string& strValue, LPCSTR strKeyPath /*= NULL*/)
{
	HKEY	h		= NULL;
	bool	bResult = false;
	string	strKeyName("Software\\");

	if (bPrivateConfig)
	{
		strKeyName += APP_NAME_A;
		strKeyName += "\\";
	}

	strKeyName += strConfigName;

	if (strKeyPath != NULL)
		strKeyName = strKeyPath;

	if (RegOpenKeyExA(hKey, strKeyName.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS)
	{
		DWORD dwSize = 0;

		if (RegQueryValueExA(h, pValueName, NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS)
		{
			dwSize += 2;

			LPBYTE pBuf = new BYTE[dwSize];

			memset(pBuf, 0, dwSize);

			if (RegQueryValueExA(h, pValueName, NULL, NULL, pBuf, &dwSize) == ERROR_SUCCESS)
			{
				strValue	= (LPCSTR)pBuf;
				bResult		= true;
			}
			delete [] pBuf;
		}
		RegCloseKey(h);
	}
	return bResult;
}

bool PutValueToRegistry(HKEY hKey, LPCSTR pValueName, LPCSTR pValue, LPCSTR strKeyPath /*= NULL*/)
{
	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	string	strKeyName("Software\\");

	if (bPrivateConfig)
	{
		strKeyName += APP_NAME_A;
		strKeyName += "\\";
	}

	strKeyName += strConfigName;
	
	if (strKeyPath != NULL)
		strKeyName = strKeyPath;

	if (RegCreateKeyExA(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		bResult = RegSetValueExA(h, pValueName, 0, REG_SZ, (CONST BYTE *)pValue, ((unsigned long)strlen(pValue)+1) * sizeof(char)) == ERROR_SUCCESS;
		RegCloseKey(h);
	}
	return bResult;
}

bool RemoveValueFromRegistry(HKEY hKey, LPCSTR pValueName, LPCSTR strKeyPath /*= NULL*/)
{
	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	string	strKeyName("Software\\");

	if (bPrivateConfig)
	{
		strKeyName += APP_NAME_A;
		strKeyName += "\\";
	}

	strKeyName += strConfigName;
	
	if (strKeyPath != NULL)
		strKeyName = strKeyPath;

	if (RegCreateKeyExA(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		bResult = RegDeleteValueA(h, pValueName) == ERROR_SUCCESS;
		RegCloseKey(h);
	}
	return bResult;
}

typedef	DWORD (WINAPI *typeSHDeleteKeyW)(HKEY, PCWSTR);

bool RemoveKeyFromRegistry(HKEY hKey, PCWSTR pKeyPath)
{
	bool bResult = false;

	if (RegDeleteKeyW(hKey, pKeyPath) == ERROR_SUCCESS)
	{
		bResult = true;
	}
	else
	{
		HMODULE hSHLWAPILib = LoadLibraryW(L"SHLWAPI.DLL");
			  
		if (hSHLWAPILib != NULL)
		{
			typeSHDeleteKeyW pSHDeleteKey = (typeSHDeleteKeyW) GetProcAddress(hSHLWAPILib, "SHDeleteKeyW");

			if (pSHDeleteKey != NULL)
			{
				bResult = pSHDeleteKey(hKey, pKeyPath) == ERROR_SUCCESS ? true : false;
			}
			FreeLibrary(hSHLWAPILib);
		}
	}
	return bResult;
}

CComVariant GetValueFromRegistry(HKEY hKey, LPCWSTR pValueName, CComVariant varDefault /*= CComVariant()*/, LPCWSTR strKeyPath /*= NULL*/)
{
	USES_CONVERSION;

	HKEY		h			= NULL;
	CComVariant	varValue	= varDefault;
	wstring		strKeyName(L"Software\\");

	if (bPrivateConfig)
	{
		strKeyName += APP_NAME_W;
		strKeyName += L"\\";
	}

	strKeyName += A2CW(strConfigName.c_str());

	if (strKeyPath != NULL)
		strKeyName = strKeyPath;

	if (RegOpenKeyExW(hKey, strKeyName.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS)
	{
		DWORD dwSize	= 0;
		DWORD dwType	= 0;

		if (RegQueryValueExW(h, pValueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
		{
			switch (dwType)
			{
				case REG_NONE:
				{
					varValue.Clear();
				}
				break;

				case REG_DWORD:
				{
					ATLASSERT(dwSize == sizeof(varValue.ulVal));

					if (RegQueryValueExW(h, pValueName, NULL, NULL, (LPBYTE)&varValue.ulVal, &dwSize) != ERROR_SUCCESS)
					{
						varValue.Clear();
					}
					else
					{
						varValue.vt = VT_UI4;

						if (varDefault.vt != VT_EMPTY && varDefault.vt != varValue.vt)
						{
							ATLVERIFY(SUCCEEDED(varValue.ChangeType(varDefault.vt)));
						}
					}
				}
				break;

				case REG_QWORD:
				{
					ATLASSERT(dwSize == sizeof(varValue.ullVal));

					if (RegQueryValueExW(h, pValueName, NULL, NULL, (LPBYTE)&varValue.ullVal, &dwSize) != ERROR_SUCCESS)
					{
						varValue.Clear();
					}
					else
					{
						varValue.vt = VT_UI8;

						if (varDefault.vt != VT_EMPTY && varDefault.vt != varValue.vt)
						{
							ATLVERIFY(SUCCEEDED(varValue.ChangeType(varDefault.vt)));
						}
					}
				}
				break;

				case REG_EXPAND_SZ:
				case REG_SZ:
				{
					dwSize += 2;

					LPBYTE pBuf = new BYTE[dwSize];

					memset(pBuf, 0, dwSize);

					if (RegQueryValueExW(h, pValueName, NULL, NULL, pBuf, &dwSize) == ERROR_SUCCESS)
					{
						varValue	= (LPCWSTR)pBuf;
					}
					delete [] pBuf;
				}
				break;

				default:
				{
					ATLASSERT(FALSE);
				}
				break;
			}
		}
		RegCloseKey(h);
	}
	return varValue;
}

bool PutValueToRegistry(HKEY hKey, LPCWSTR pValueName, CComVariant varValue, LPCWSTR strKeyPath /*= NULL*/)
{
	USES_CONVERSION;

	HKEY	h;
	DWORD	disp	= 0;
	bool	bResult	= false;
	wstring	strKeyName(L"Software\\");

	if (bPrivateConfig)
	{
		strKeyName += APP_NAME_W;
		strKeyName += L"\\";
	}

	strKeyName += A2CW(strConfigName.c_str());
	
	if (strKeyPath != NULL)
		strKeyName = strKeyPath;

	if (RegCreateKeyExW(hKey, strKeyName.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &h, &disp) == ERROR_SUCCESS)
	{
		switch (varValue.vt)
		{
			case VT_EMPTY:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_NONE, NULL, 0) == ERROR_SUCCESS;
			}
			break;

			case VT_BSTR:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_SZ, (CONST BYTE *)varValue.bstrVal, ((unsigned long)wcslen(varValue.bstrVal)+1) * sizeof(WCHAR)) == ERROR_SUCCESS;
			}
			break;

			case VT_I4:
			case VT_UI4:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_DWORD, (CONST BYTE *)&varValue.ulVal, sizeof(varValue.ulVal)) == ERROR_SUCCESS;
			}
			break;

			case VT_I8:
			case VT_UI8:
			{
				bResult = RegSetValueExW(h, pValueName, 0, REG_QWORD, (CONST BYTE *)&varValue.ullVal, sizeof(varValue.ullVal)) == ERROR_SUCCESS;
			}
			break;

			case VT_BOOL:
			{
				ULONG ulValue = varValue.boolVal != VARIANT_FALSE ? TRUE : FALSE;
				bResult = RegSetValueExW(h, pValueName, 0, REG_DWORD, (CONST BYTE *)&ulValue, sizeof(ulValue)) == ERROR_SUCCESS;
			}
			break;

			default:
			{
				ATLASSERT(FALSE);
			}
			break;
		}
		RegCloseKey(h);
	}
	return bResult;
}

void EncodeBase64(const BYTE* pBlob, long sizeOfBlob, string& strValue, bool bInsertLineBreaks /*= true*/)
{
	BIO* bio_base64		= BIO_new(BIO_f_base64());
	BIO* bio_mem		= BIO_new(BIO_s_mem());

	if (!bInsertLineBreaks)
		BIO_set_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);
	else
		BIO_clear_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);

	BIO_push(bio_base64, bio_mem);

	BIO_write(bio_base64, pBlob, sizeOfBlob);
	BIO_flush (bio_base64);

	BUF_MEM* pBufferInfo = NULL;
	BIO_get_mem_ptr(bio_mem, &pBufferInfo);

	strValue = string(pBufferInfo->data, pBufferInfo->length);

	BIO_free_all(bio_base64);
}

long DecodeBase64(string strValue, CTempBuffer<BYTE>& Blob)
{
	long nResult = strValue.length();
	
	// Pad with '=' as per RFC 1521
	while (nResult % 4 != 0)
	{
		strValue += "=";
		++nResult;
	}
	Blob.Reallocate(nResult);

	BIO* bio_base64		= BIO_new(BIO_f_base64());
	BIO* bio_mem		= BIO_new_mem_buf((void*)strValue.c_str(), nResult);

	if (strValue.find('\n') == string::npos)
		BIO_set_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);
	else
		BIO_clear_flags(bio_base64, BIO_FLAGS_BASE64_NO_NL);

	BIO_push(bio_base64, bio_mem);

	nResult = BIO_read(bio_base64, Blob, nResult);

	BIO_free_all(bio_base64);

	return nResult;
}

HANDLE Exec(LPCTSTR strCmd, DWORD dwWait /*= INFINITE*/, bool bStdInput /*= false*/, HANDLE* phWrite /*= NULL*/, WORD wShowWindow /*= SW_HIDE*/, PCWSTR strPath /* = NULL*/, HANDLE* phStdOut /*= NULL*/)
{
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	si.cb			= sizeof(si);
	si.dwFlags		= STARTF_USESHOWWINDOW;
	si.wShowWindow	= wShowWindow;

	HANDLE hwrite	= INVALID_HANDLE_VALUE;

	if (phStdOut)
	{
		*phStdOut = INVALID_HANDLE_VALUE;
	}
	if (bStdInput)
	{
		HANDLE hread	= INVALID_HANDLE_VALUE;
		HANDLE hout		= GetStdHandle(STD_OUTPUT_HANDLE);

		SECURITY_ATTRIBUTES   sa;

		sa.nLength				= sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle		= TRUE;

		if (CreatePipe(&hread, &hwrite, &sa, 0))
		{
			if (phStdOut)
			{
				hout = INVALID_HANDLE_VALUE;
				ATLVERIFY(CreatePipe(phStdOut, &hout, &sa, 0));
				SetHandleInformation(*phStdOut, HANDLE_FLAG_INHERIT, 0);
			}
			SetHandleInformation(hwrite, HANDLE_FLAG_INHERIT, 0);

			if (phWrite != NULL)
				*phWrite = hwrite;

			si.hStdInput  = hread;
			si.hStdOutput = hout;
			si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
			si.dwFlags	|= STARTF_USESTDHANDLES;
		}
	}
	CTempBuffer<TCHAR> _strCmd(_tcslen(strCmd)+1);

	_tcscpy_s(_strCmd, _tcslen(strCmd)+1, strCmd);

	if (CreateProcess(NULL, _strCmd, NULL, NULL, bStdInput ? TRUE : FALSE, 0, NULL, strPath, &si, &pi))
	{
		CloseHandle(pi.hThread);

		if (si.dwFlags & STARTF_USESTDHANDLES)
		{
			ATLVERIFY(CloseHandle(si.hStdInput));
			
			if (phStdOut)
			{
				ATLVERIFY(CloseHandle(si.hStdOutput));
			}
		}
		if (WaitForSingleObject(pi.hProcess, dwWait) != WAIT_OBJECT_0)
			return pi.hProcess;
		CloseHandle(pi.hProcess);
	}
	if (hwrite != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hwrite);

		if (phWrite != NULL)
			*phWrite = INVALID_HANDLE_VALUE;
	}
	if (phStdOut && *phStdOut != INVALID_HANDLE_VALUE)
	{
		CloseHandle(*phStdOut);
		*phStdOut = INVALID_HANDLE_VALUE;
	}
	return NULL;
}

string MD5_Hex(const BYTE* pBlob, long sizeOfBlob, bool bUppercase /*= false*/)
{
	string strResult;

	BYTE md5digest[MD5_DIGEST_LENGTH];

	if (MD5(pBlob, sizeOfBlob, md5digest) != NULL)
	{
		for (long i = 0; i < MD5_DIGEST_LENGTH; ++i)
		{
			char buf[16];

			sprintf_s(buf, 16, bUppercase ? "%02X" : "%02x", md5digest[i]);
			strResult += buf;
		}
	}
	return strResult;
}

bool ParseRegEx(CAtlRegExp<CMyRECharTraitsA>& exp, LPCSTR strToParse, LPCSTR strRegExp, list<string>& listTok, BOOL bCaseSensitive /*= TRUE*/, BOOL bMatchAllGroups /*= TRUE*/)
{
	while (strToParse != NULL && *strToParse != 0)
	{
		if (listTok.empty())
		{
			if (exp.Parse((const unsigned char *)strRegExp, bCaseSensitive) != REPARSE_ERROR_OK)
				return false;
		}
		CAtlREMatchContext<CMyRECharTraitsA> mc; 

		if (exp.Match((const unsigned char *)strToParse, &mc))
		{
			const CAtlREMatchContext<CMyRECharTraitsA>::RECHAR* szEnd = NULL; 

			for (UINT nGroupIndex = 0; nGroupIndex < mc.m_uNumGroups; ++nGroupIndex) 
			{ 
				const CAtlREMatchContext<CMyRECharTraitsA>::RECHAR* szStart = NULL; 
				
				mc.GetMatch(nGroupIndex, &szStart, &szEnd);
				
				ptrdiff_t nLength = szEnd - szStart; 
		
				if (nLength > 0)
				{
					listTok.push_back(string((const char*)szStart, nLength));
				
					if (!bMatchAllGroups)
						break;
				}
			}
			strToParse = (const char*)szEnd;
		}
		else
			break;
	}
	return true;
}

bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, list<string>& listTok, BOOL bCaseSensitive /*= TRUE*/)
{
	CAtlRegExp<CMyRECharTraitsA> exp;
	
	return ParseRegEx(exp, strToParse, strRegExp, listTok, bCaseSensitive);
}

bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, string& strTok, BOOL bCaseSensitive /*= TRUE*/)
{
	CAtlRegExp<CMyRECharTraitsA> exp;

	list<string> listTok;
	
	ParseRegEx(exp, strToParse, strRegExp, listTok, bCaseSensitive);

	if (!listTok.empty())
	{
		strTok = *listTok.begin();
		return true;
	}
	return false;
}

bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, map<ci_string, string>& mapKeyValue, BOOL bCaseSensitive /*= TRUE*/)
{
	CAtlRegExp<CMyRECharTraitsA> exp;

	list<string> listTok;

	if (ParseRegEx(exp, strToParse, strRegExp, listTok, bCaseSensitive))
	{
		for (list<string>::iterator i = listTok.begin(); i != listTok.end(); ++i)
		{
			string& strValue = mapKeyValue[i->c_str()];

			if (++i == listTok.end())
				break;
			strValue = *i;
		}
		return true;
	}
	return false;
}

string IPAddrToString(struct sockaddr_in* in)
{
	char	buf[256];
	string	strResult = "";

	switch(in->sin_family)
	{
		case AF_INET:
		{
			sprintf_s(buf, 256, "%ld.%ld.%ld.%ld", (long)in->sin_addr.S_un.S_un_b.s_b1
											, (long)in->sin_addr.S_un.S_un_b.s_b2
											, (long)in->sin_addr.S_un.S_un_b.s_b3
											, (long)in->sin_addr.S_un.S_un_b.s_b4);
			strResult = buf;
		}
		break;

		case AF_INET6:
		{
			DWORD dwLen = sizeof(buf) / sizeof(buf[0]);

			if (WSAAddressToStringA((LPSOCKADDR)in, sizeof(struct sockaddr_in6), NULL, buf, &dwLen) == 0)
			{
				buf[dwLen] = 0;
				strResult = buf;
			}
		}
		break;
	}
	return strResult;
}

bool StringToIPAddr(const char* str, struct sockaddr_in* in, int laddr_size)
{
	bool bResult = false;

	if (str != NULL && strlen(str) > 0)
	{
		ULONG	nBufLen		= (ULONG)(strlen(str)+1);
		char*	pBuf		= new char[nBufLen];

		if (pBuf != NULL)
		{
			strcpy_s(pBuf, nBufLen, str);

			if (laddr_size == sizeof(struct sockaddr_in))
			{
				INT dwSize = laddr_size;

				bResult = WSAStringToAddressA(pBuf, AF_INET, NULL, (LPSOCKADDR)in, &dwSize) == 0 ? true : false;
			}
			else
			{
				ATLASSERT(laddr_size == sizeof(struct sockaddr_in6));

				INT dwSize = laddr_size;

				bResult = WSAStringToAddressA(pBuf, AF_INET6, NULL, (LPSOCKADDR)in, &dwSize) == 0 ? true : false;
			}
			delete [] pBuf;
		}
	}
	return bResult;
}

bool GetPeerIP(SOCKET sd, bool bV4, struct sockaddr_in* in)
{
	int laddr_size = sizeof(struct sockaddr_in);

	if (!bV4)
	{
		laddr_size = sizeof(struct sockaddr_in6);
	}
	return (0 == getpeername(sd, (sockaddr *)in, &laddr_size)) ? true : false;
}

void GetPeerIP(SOCKET sd, bool bV4, string& strIP)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getpeername(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString(&laddr_in);
	}
	else
	{
		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getpeername(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString((struct sockaddr_in*)&laddr_in);
	}
}

bool GetPeerName(const char* strIP, bool bV4, wstring& strHostName)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;

		if (StringToIPAddr(strIP, (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in)))
		{
			WCHAR buf[NI_MAXHOST];

			if (GetNameInfoW((const SOCKADDR *)&laddr_in, sizeof(struct sockaddr_in), buf, _countof(buf), NULL, 0, NI_NAMEREQD|NI_NOFQDN) == 0)
			{
				strHostName = buf;
				return true;
			}
		}
	}
	else
	{
		string _strIP;

		if (strIP[0] == '[')
		{
			_strIP = strIP + 1;

			string::size_type n = _strIP.find('%');

			if (n != string::npos)
			{
				_strIP = _strIP.substr(0, n);
			}
			else
			{
				n = _strIP.find(']');

				if (n != string::npos)
				{
					_strIP = _strIP.substr(0, n);
				}
				else
					_strIP.clear();
			}
		}
		struct sockaddr_in6	laddr_in;

		if (	StringToIPAddr(strIP, (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in6))
			||	(!_strIP.empty() && StringToIPAddr(_strIP.c_str(), (struct sockaddr_in*)&laddr_in, sizeof(struct sockaddr_in6)))
			)
		{
			WCHAR buf[NI_MAXHOST];

			if (GetNameInfoW((const SOCKADDR *)&laddr_in, sizeof(struct sockaddr_in6), buf, _countof(buf), NULL, 0, NI_NAMEREQD|NI_NOFQDN) == 0)
			{
				strHostName = buf;
				return true;
			}
		}
	}
	return false;
}

void GetLocalIP(SOCKET sd, bool bV4, string& strIP)
{
	if (bV4)
	{
		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString(&laddr_in);
	}
	else
	{
		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
			strIP = IPAddrToString((struct sockaddr_in*)&laddr_in);
	}
}

int GetLocalIP(SOCKET sd, bool bV4, CTempBuffer<BYTE>& pIP)
{
	int nResult = 0;

	if (bV4)
	{
		pIP.Allocate(4);

		struct sockaddr_in	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
		{
			nResult = 4;
			memcpy(pIP, &laddr_in.sin_addr.s_addr, nResult);
		}
	}
	else
	{
		pIP.Allocate(16);

		struct sockaddr_in6	laddr_in;
		int					laddr_size = sizeof(laddr_in);

		if (0 == getsockname(sd, (sockaddr *)&laddr_in, &laddr_size))
		{
			nResult = 16;
			memcpy(pIP, &laddr_in.sin6_addr, nResult);
		}
	}
	return nResult;
}

Bitmap* BitmapFromBlob(const BYTE* p, long nBytes)
{
	ATLASSERT(p != NULL && nBytes > 0);

	Bitmap* pResult = 0;

	HGLOBAL hBlob = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, nBytes);  

	if (hBlob != NULL)
	{
		IStream* pStream = NULL;

		if (SUCCEEDED(CreateStreamOnHGlobal(hBlob, TRUE, &pStream)))
		{
			ULONG nWritten = 0;
			pStream->Write(p, nBytes, &nWritten);
			ATLASSERT(nBytes == nWritten);

			pResult	= Bitmap::FromStream(pStream);

			pStream->Release();
		}
		else
		{
			GlobalFree(hBlob);
		}
	}
	return pResult;
}

Bitmap* BitmapFromResource(HINSTANCE hInstance, LPCTSTR strID, LPCTSTR strType /* = _T("PNG") */)
{
	Bitmap* pResult		= NULL;
	HRSRC	hResource	= FindResource(hInstance, strID, strType);

	if (hResource != NULL)
	{
		DWORD	nSize			= SizeofResource(hInstance, hResource);
	    HANDLE	hResourceMem	= LoadResource(hInstance, hResource);
	    BYTE*	pData			= (BYTE*)LockResource(hResourceMem);

		if (pData != NULL)
		{
			HGLOBAL hBuffer  = GlobalAlloc(GMEM_MOVEABLE, nSize);

			if (hBuffer != NULL) 
			{
				void* pBuffer = GlobalLock(hBuffer);
				
				if (pBuffer != NULL)
				{
					CopyMemory(pBuffer, pData, nSize);
					GlobalUnlock(hBuffer);

					IStream* pStream = NULL;
			
					if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream)))
					{
						pResult = Bitmap::FromStream(pStream);
						pStream->Release();
					}
					else
						GlobalFree(hBuffer);
				}
				else
					GlobalFree(hBuffer);
			}
		}
	}
	return pResult;
}

bool GetVersionInfo(LPCWSTR pszFileName, LPWSTR strVersion, ULONG nBufSizeInChars)
{
	HINSTANCE hVLib = LoadLibraryA("version.dll");

	if (hVLib == NULL)
		return false;

	_GetFileVersionInfoSizeW	pGetFileVersionInfoSizeW	= (_GetFileVersionInfoSizeW)GetProcAddress(hVLib, "GetFileVersionInfoSizeW");	
   	_GetFileVersionInfoW		pGetFileVersionInfoW		= (_GetFileVersionInfoW)	GetProcAddress(hVLib, "GetFileVersionInfoW");	
 	_VerQueryValueW				pVerQueryValueW				= (_VerQueryValueW)			GetProcAddress(hVLib, "VerQueryValueW");	

	DWORD			cbVerInfo;
	DWORD			dummy;

    // How big is the version info?
    cbVerInfo = pGetFileVersionInfoSizeW(pszFileName, &dummy);

    if (!cbVerInfo)
        return false;
    
    // Allocate space to hold the info
    PBYTE pVerInfo = new BYTE[cbVerInfo];

    if (pVerInfo == NULL)
        return false;

	bool bResult = false;

    if (pGetFileVersionInfoW(pszFileName, 0, cbVerInfo, pVerInfo))
	{
		VS_FIXEDFILEINFO*	pFixedVer = NULL;
		WCHAR				szQueryStr[0x100];
		UINT				cbReturn	= 0;

		swprintf_s(szQueryStr, _countof(szQueryStr), L"\\");

		BOOL bFound	= pVerQueryValueW(pVerInfo, szQueryStr,
								(LPVOID *)&pFixedVer, &cbReturn);
		if (bFound)
		{
			bResult = true;

			swprintf_s(strVersion, nBufSizeInChars, L"%ld.%ld.%ld.%ld", (long)HIWORD(pFixedVer->dwFileVersionMS), (long)LOWORD(pFixedVer->dwFileVersionMS)
												 , (long)HIWORD(pFixedVer->dwFileVersionLS), (long)LOWORD(pFixedVer->dwFileVersionLS));
		}
		else
		{
			struct LANGANDCODEPAGE
			{
			  WORD wLanguage;
			  WORD wCodePage;
			} *lpTranslate = NULL;
			
			UINT	cbTranslate = 0;

			// Read the list of languages and code pages.
			pVerQueryValueW(pVerInfo, 
				  L"\\VarFileInfo\\Translation",
				  (LPVOID*)&lpTranslate,
				  &cbTranslate);

			PWSTR pszVerRetVal	= NULL;

			for (long i = 0; i < (long)(cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++)
			{
				// Format the string with the codepage
				swprintf_s(szQueryStr, _countof(szQueryStr), L"\\StringFileInfo\\%04X%04X\\FileVersion",
							lpTranslate[i].wLanguage,
							lpTranslate[i].wCodePage);

				// Try first with the 1252 codepage
				bFound = pVerQueryValueW(pVerInfo, szQueryStr,
										(LPVOID *)&pszVerRetVal, &cbReturn);
				if (bFound)
					break;
			}
			if (bFound && pszVerRetVal != NULL)
			{
				bResult = true;

				wcscpy_s(strVersion, nBufSizeInChars, pszVerRetVal);

				long lauf = 0;

				while(strVersion[lauf] != 0)
				{
					if (strVersion[lauf] == L',')
						strVersion[lauf] = L'.';
					lauf++;
				}
			}
		}
    }
    delete [] pVerInfo;

	return bResult;
}
									
#define DELTA_EP_IN_MICRO_SECS		9435484800000000ULL

ULONGLONG GetNTPTimeStamp()
{
	FILETIME			ft		= { 0 };
	ULONGLONG			tmpres	= 0;
 
	GetSystemTimeAsFileTime(&ft);
 
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;
 
	// convert to micro secs
	tmpres /= 10ULL;  
	
	// offset from 1601 to 1970
	tmpres -= DELTA_EP_IN_MICRO_SECS; 

	return MAKEULONGLONG(tmpres % 1000000ULL, tmpres / 1000000ULL);  
}

USHORT GetUniquePortNumber()
{
	c_mtxUniquePortCounter.Lock();

	if (c_nUniquePortCounter > 12000)
		c_nUniquePortCounter = 6000;

	USHORT nResult = c_nUniquePortCounter++;

	c_mtxUniquePortCounter.Unlock();

	return nResult;
}

ULONG CreateRand(ULONG nMin /*= 0*/, ULONG nMax /*= MAXDWORD*/)
{
	ULONG nResult = MAKELONG(rand()+rand(), rand()+rand());

	while(nResult < nMin)
		nResult *= (rand()+1);

	while (nResult > nMax)
	{
		if (nResult &  0x80000000)
			nResult &= 0x7fffffff;
		else
			nResult >>= 1;
	}
	ATLASSERT(nResult >= nMin && nResult <= nMax);

	return nResult;
}

ULONGLONG ToDacpID(LPCSTR strID)
{
	ULONGLONG nResult = 0;

	ATLASSERT(strID != NULL);

	long n = (long)strlen(strID);

	if (n > 0)
	{
		ULONGLONG f = 1;

		for (long i = n-1; i >= 0; --i)
		{
			char c = strID[i];

			if (c >= '0' && c <= '9')
				nResult += (f*(c - '0'));
			else
			{
				c = tolower(c);

				if (c >= 'a' && c <= 'f')
					nResult += (f*(c - 'a' + 10));
				else
					break;
			}
			f *= 16ull;
		}
	}
	return nResult;
}

bool FileExists(PCWSTR strPath, WTL::CString* pStrResult /*= NULL*/)
{
	WIN32_FIND_DATAW fd;

	HANDLE	h = FindFirstFileW(strPath, &fd);

	if (h == INVALID_HANDLE_VALUE)
		return false;
	else
	{
		if (pStrResult != NULL)
		{
			*pStrResult = fd.cFileName;
		}
		FindClose(h);
	}
	return true;
}

void DetermineOS()
{
    bIsWinXP = false;

	OSVERSIONINFOEX Info;

	Info.dwOSVersionInfoSize = sizeof(Info);

	if (GetVersionEx((LPOSVERSIONINFO)&Info))
	{
		if (Info.dwMajorVersion <= 5)
		{
			bIsWinXP = true;
		}
	}
}

bool IsAdmin()
{
	typedef BOOL (WINAPI *_IsUserAnAdmin)(void);

	_IsUserAnAdmin IsUserAnAdmin = NULL;
	
	HINSTANCE hDll = LoadLibrary(_T("shell32.dll"));
				  
	if (hDll != NULL)
	{
		IsUserAnAdmin = (_IsUserAnAdmin)GetProcAddress(hDll, "IsUserAnAdmin");

		if (IsUserAnAdmin == NULL)
			IsUserAnAdmin = (_IsUserAnAdmin)GetProcAddress(hDll, (LPCSTR)0x02A8);

		if (IsUserAnAdmin != NULL)
			return IsUserAnAdmin() ? true : false;
	}
	return true;
}

bool RunAsAdmin(HWND hWnd, LPCTSTR lpFile, LPCTSTR lpParameters, bool bWait /*= false*/, PCWSTR strVerb /*= L"runas"*/)
{
	if (bIsWinXP)
	{
		if (strVerb != NULL && wcscmp(strVerb, L"runas") == 0)
		{
			if (IsAdmin())
				strVerb = L"open";
		}
	}
    SHELLEXECUTEINFO   sei;
    
	ZeroMemory( &sei, sizeof(sei) );

    sei.cbSize          = sizeof(SHELLEXECUTEINFOW);
    sei.hwnd            = hWnd;
    sei.fMask           = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI | (bWait ? SEE_MASK_NOASYNC : 0);
    sei.lpVerb          = strVerb;
    sei.lpFile          = lpFile;
    sei.lpParameters    = lpParameters;
    sei.nShow           = _tcsicmp(lpFile, _T("netsh")) != 0 ? SW_SHOWNORMAL : SW_HIDE;

	bool bResult = ShellExecuteEx(&sei) ? true : false;

	if (bResult && sei.hProcess != NULL)
		CloseHandle(sei.hProcess);

	return bResult;
}

bool ReBootMachine()
{
	bool bResult = false;

	HANDLE				hToken = NULL; 
	TOKEN_PRIVILEGES	Priv;	

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
	{
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &Priv.Privileges[0].Luid);
		
		Priv.PrivilegeCount				= 1;
		Priv.Privileges[0].Attributes	= SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(hToken, FALSE, &Priv, 0, (PTOKEN_PRIVILEGES) NULL, 0);
		
		bResult = ExitWindowsEx(EWX_REBOOT|EWX_FORCE, 0) ? true : false;
	}
	return bResult;
}

void ErrorMsg(HWND hwnd, PCTSTR lpszFunction, DWORD dwErr) 
{ 
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf		= NULL;
	LPVOID lpDisplayBuf	= NULL;
	
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen(lpszFunction) + 40) * sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s: %s"), 
		lpszFunction, lpMsgBuf); 
	MessageBox(hwnd, (LPCTSTR)lpDisplayBuf, TEXT("Shairport4w"), MB_OK | (dwErr == ERROR_SUCCESS ? MB_ICONINFORMATION : MB_ICONEXCLAMATION)); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}


#ifdef _RECORD_LOG
#include "RecordLog.cpp"
#endif
