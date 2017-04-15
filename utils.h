/*
 *
 *  Utils.h
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

#include <time.h>

#define NUM_CHANNELS	2
#define	SAMPLE_SIZE		16
#define	SAMPLE_FACTOR	(NUM_CHANNELS * (SAMPLE_SIZE >> 3))
#define	SAMPLE_FREQ		(44100.0l)
#define BYTES_PER_SEC	(SAMPLE_FREQ * SAMPLE_FACTOR)

#define APP_NAME_A "Shairport4w"
#define APP_NAME_W L"Shairport4w"

#define EVENT_NAME_EXIT_PROCESS _T("Shairport4w_ExitProcess")
#define EVENT_NAME_SHOW_WINDOW	_T("Shairport4w_ShowMain")

#ifndef MAKEULONGLONG
#define MAKEULONGLONG(ldw, hdw) ((ULONGLONG(hdw) << 32) | ((ldw) & 0xffffffff))
#endif

#ifndef LODWORD
#define LODWORD(l)           ((DWORD)(((ULONGLONG)(l)) & 0xffffffff))
#define HIDWORD(l)           ((DWORD)((((ULONGLONG)(l)) >> 32) & 0xffffffff))
#endif

template <class T>
void TrimRight(T& s, const char* tr = "\t \n\r")
{
	int n = s.find_last_not_of(tr);
			
	if (n != string::npos)
	   s = s.substr(0, n+1);
	else
	   s.erase();
}

template <class T>
void TrimLeft(T& s, const char* tr = "\t \n\r")
{
	int n = s.find_first_not_of(tr);
			
	if (n != string::npos)
	   s = s.substr(n);
	else
	   s.erase();
}

template <class T>
void Trim(T& s, const char* tr = "\t \n\r")
{
	TrimRight(s, tr);
	TrimLeft(s, tr);
}

template <class T>
void TrimRight(T& s, PCWSTR tr)
{
	int n = s.find_last_not_of(tr);
			
	if (n != string::npos)
	   s = s.substr(0, n+1);
	else
	   s.erase();
}

template <class T>
void TrimLeft(T& s, PCWSTR tr)
{
	int n = s.find_first_not_of(tr);
			
	if (n != string::npos)
	   s = s.substr(n);
	else
	   s.erase();
}

template <class T>
void Trim(T& s, PCWSTR tr)
{
	TrimRight(s, tr);
	TrimLeft(s, tr);
}

__inline _CONST_RETURN char * __CRTDECL memichr(_In_count_(_N) const char *_S, _In_ char _C, _In_ size_t _N)
        {for (; 0 < _N; ++_S, --_N)
                if (tolower(*_S) == tolower(_C))
                        return (_CONST_RETURN char *)(_S);
        return (0); }

// case ignore traits
struct ci_char_traits : public char_traits<char>
{
	static bool eq( char c1, char c2 )
	{ return tolower(c1) == tolower(c2); }

	static bool ne( char c1, char c2 )
	{ return tolower(c1) != tolower(c2); }

	static bool lt( char c1, char c2 )
	{ return tolower(c1) <  tolower(c2); }

	static int compare( const char* s1, const char* s2, size_t n ) 
	{ return _memicmp(s1, s2, n); }

	static const char * find(const char *_First, size_t _Count, const char& _Ch)
	{ return ((const char *) memichr(_First, _Ch, _Count)); }
};

// case ignore compare string-class
typedef basic_string<char, ci_char_traits, allocator<char> > ci_string;

//extern	CMutex		mtxAppSessionInstance;
//extern	CMutex		mtxAppGlobalInstance;
extern	string		strConfigName;
extern	bool		bPrivateConfig;
extern	bool		bIsWinXP;
extern	BOOL&		bLogToFile;

bool GetValueFromRegistry(HKEY hKey, LPCSTR pValueName, string& strValue, LPCSTR strKeyPath = NULL);
bool PutValueToRegistry(HKEY hKey, LPCSTR pValueName, LPCSTR pValue, LPCSTR strKeyPath = NULL);
bool RemoveValueFromRegistry(HKEY hKey, LPCSTR pValueName, LPCSTR strKeyPath = NULL);
bool RemoveKeyFromRegistry(HKEY hKey, PCWSTR pKeyPath);

void DetermineOS();
bool IsAdmin();
bool RunAsAdmin(HWND hWnd, LPCTSTR lpFile, LPCTSTR lpParameters, bool bWait = false, PCWSTR strVerb = L"runas");
bool ReBootMachine();

void ErrorMsg(HWND hwnd, PCTSTR lpszFunction, DWORD dwErr);

CComVariant GetValueFromRegistry(HKEY hKey, LPCWSTR pValueName, CComVariant varDefault = CComVariant(), LPCWSTR strKeyPath = NULL);
bool PutValueToRegistry(HKEY hKey, LPCWSTR pValueName, CComVariant varValue, LPCWSTR strKeyPath = NULL);

bool start_serving();
void stop_serving();
bool is_streaming();

void EncodeBase64(const BYTE* pBlob, long sizeOfBlob, string& strValue, bool bInsertLineBreaks = true);
long DecodeBase64(string strValue, CTempBuffer<BYTE>& Blob);
string MD5_Hex(const BYTE* pBlob, long sizeOfBlob, bool bUppercase = false);

bool Log(LPCSTR pszFormat, ...);

string IPAddrToString(struct sockaddr_in* in);
bool StringToIPAddr(const char* str, struct sockaddr_in* in, int laddr_size);

bool GetPeerIP(SOCKET sd, bool bV4, struct sockaddr_in* in);
void GetPeerIP(SOCKET sd, bool bV4, string& strIP);
void GetLocalIP(SOCKET sd, bool bV4, string& strIP);
int GetLocalIP(SOCKET sd, bool bV4, CTempBuffer<BYTE>& pIP);
bool GetPeerName(const char* strIP, bool bV4, wstring& strHostName);

bool FileExists(PCWSTR strPath, WTL::CString* pStrResult = NULL);

class CMyRECharTraitsA
{
public:
	typedef unsigned char RECHARTYPE;

	static size_t GetBitFieldForRangeArrayIndex(const RECHARTYPE *sz) throw()
	{
#ifndef ATL_NO_CHECK_BIT_FIELD
		ATLASSERT(UseBitFieldForRange());
#endif
		return static_cast<size_t>(static_cast<unsigned char>(*sz));		
	}
	static RECHARTYPE *Next(const RECHARTYPE *sz) throw()
	{
		return (RECHARTYPE *) (sz+1);
	}

	static int Strncmp(const RECHARTYPE *szLeft, const RECHARTYPE *szRight, size_t nCount) throw()
	{
		return strncmp((const char *)szLeft, (const char *)szRight, nCount);
	}

	static int Strnicmp(const RECHARTYPE *szLeft, const RECHARTYPE *szRight, size_t nCount) throw()
	{
		return _strnicmp((const char *)szLeft, (const char *)szRight, nCount);
	}

	_ATL_INSECURE_DEPRECATE("CMyRECharTraitsA::Strlwr must be passed a buffer size.")
	static RECHARTYPE *Strlwr(RECHARTYPE *sz) throw()
	{
		#pragma warning (push)
		#pragma warning(disable : 4996)
		return (RECHARTYPE*) _strlwr((char *)sz);
		#pragma warning (pop)
	}

	static RECHARTYPE *Strlwr(RECHARTYPE *sz, int nSize) throw()
	{
		_strlwr_s((char *)sz, nSize);
		return sz;
	}

	static long Strtol(const RECHARTYPE *sz, RECHARTYPE **szEnd, int nBase) throw()
	{
		return strtol((char *)sz, (char **)szEnd, nBase);
	}

	static int Isdigit(RECHARTYPE ch) throw()
	{
		return isdigit(static_cast<unsigned char>(ch));
	}

	static const RECHARTYPE** GetAbbrevs()
	{
		static const char *s_szAbbrevs[] = 
		{
			"a([a-zA-Z0-9])",	// alpha numeric
			"b([ \\t])",		// white space (blank)
			"c([a-zA-Z])",	// alpha
			"d([0-9])",		// digit
			"h([0-9a-fA-F])",	// hex digit
			"n(\r|(\r?\n))",	// newline
			"q(\"[^\"]*\")|(\'[^\']*\')",	// quoted string
			"w([a-zA-Z]+)",	// simple word
			"z([0-9]+)",		// integer
			NULL
		};

		return (const RECHARTYPE**)s_szAbbrevs;
	}

	static BOOL UseBitFieldForRange() throw()
	{
		return TRUE;
	}

	static int ByteLen(const RECHARTYPE *sz) throw()
	{
		return int(strlen((const char *)sz));
	}
};

bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, map<ci_string, string>& mapKeyValue, BOOL bCaseSensitive = TRUE);
bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, string& strTok, BOOL bCaseSensitive = TRUE);
bool ParseRegEx(LPCSTR strToParse, LPCSTR strRegExp, list<string>& listTok, BOOL bCaseSensitive = TRUE);
bool ParseRegEx(CAtlRegExp<CMyRECharTraitsA>& exp, LPCSTR strToParse, LPCSTR strRegExp, list<string>& listTok, BOOL bCaseSensitive = TRUE, BOOL bMatchAllGroups = TRUE);

HANDLE Exec(LPCTSTR strCmd, DWORD dwWait = INFINITE, bool bStdInput = false, HANDLE* phWrite = NULL, WORD wShowWindow = SW_HIDE, PCWSTR strPath = NULL, HANDLE* phStdOut = NULL);
bool GetVersionInfo(LPCWSTR pszFileName, LPWSTR strVersion, ULONG nBufSizeInChars);

Bitmap* BitmapFromResource(HINSTANCE hInstance, LPCTSTR strID, LPCTSTR strType = _T("PNG"));
Bitmap* BitmapFromBlob(const BYTE* p, long nBytes);

ULONGLONG GetNTPTimeStamp();
USHORT GetUniquePortNumber();
ULONG CreateRand(ULONG nMin = 0, ULONG nMax = MAXDWORD);

ULONGLONG ToDacpID(LPCSTR strID);

#ifndef BCM_SETSHIELD
#define BCM_SETSHIELD            (0x1600 + 0x000C)
#define Button_SetElevationRequiredState(hwnd, fRequired) \
    (LRESULT)SNDMSG((hwnd), BCM_SETSHIELD, 0, (LPARAM)fRequired)
#endif

#ifndef IDI_SHIELD
#define IDI_SHIELD MAKEINTRESOURCE(32518)
#endif

#include <memory>
class CRaopContext;

class IPlugInManager
{
public:
	virtual bool wave_open(int nSamplingRate, HANDLE hEventPlay) = 0;
	virtual int wave_play(std::shared_ptr<CRaopContext> pRaopContext, const unsigned char* output_samples, unsigned int num_bytes, double bf_playback_rate) = 0;
	virtual void wave_close() = 0;
	virtual PCWSTR GetApName() = 0;
	virtual void UpdateTrayIcon(HICON hIcon) = 0;
};

/////////////////////////////////////////////////////////
// CSP4W_TemporaryFile

class CSP4W_TemporaryFile
{
public:
	CSP4W_TemporaryFile(PCWSTR strExt = NULL) 
	{
		if (strExt != NULL)
			m_strExt = strExt;
	}

	~CSP4W_TemporaryFile() 
	{
		Close();
	}
	void Close()
	{
		if (m_file.m_h != NULL)
		{
			m_file.Close();
		}
	}
	HRESULT Create(bool bDeleteOnClose = false)
	{
		LPCTSTR pszDir			= NULL;
		DWORD dwDesiredAccess	= GENERIC_READ|GENERIC_WRITE;
		DWORD dwShareMode		= FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

		TCHAR szPath[_MAX_PATH];
		TCHAR tmpFileName[_MAX_PATH];

		ATLASSUME(m_file.m_h == NULL);

		if (pszDir == NULL)
		{
			DWORD dwRet = GetTempPath(_MAX_DIR, szPath);
			if (dwRet == 0)
			{
				// Couldn't find temporary path;
				return AtlHresultFromLastError();
			}
			else if (dwRet > _MAX_DIR)
			{
				return DISP_E_BUFFERTOOSMALL;
			}
		}
		else
		{
			if(Checked::tcsncpy_s(szPath, _countof(szPath), pszDir, _TRUNCATE)==STRUNCATE)
			{
				return DISP_E_BUFFERTOOSMALL;
			}
		}

		if (!GetTempFileName(szPath, _T("TFR"), 0, tmpFileName))
		{
			// Couldn't create temporary filename;
			return AtlHresultFromLastError();
		}
		tmpFileName[_countof(tmpFileName)-1]='\0';

		m_strTempFileName = tmpFileName;
		
		SECURITY_ATTRIBUTES secatt;
		
		secatt.nLength				= sizeof(secatt);
		secatt.lpSecurityDescriptor = NULL;
		secatt.bInheritHandle		= TRUE;

		m_dwAccess = dwDesiredAccess;

		if (!m_strExt.IsEmpty())
		{
			::DeleteFile(m_strTempFileName);
			m_strTempFileName += m_strExt;
		}
		return m_file.Create(
			m_strTempFileName,
			m_dwAccess,
			dwShareMode,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY | (bDeleteOnClose ? FILE_FLAG_DELETE_ON_CLOSE : 0),
			&secatt);
	}
	operator HANDLE()
	{
		return m_file;
	}

public:
	CAtlFile			m_file;
	WTL::CString		m_strTempFileName;
	DWORD				m_dwAccess;
	WTL::CString		m_strExt;
};

inline WTL::CString GetLanguageAbbr()
{
	WTL::CString strLang = L"en";

	LANGID lid = GetUserDefaultLangID();

	switch(PRIMARYLANGID(lid))
	{
		case LANG_GERMAN:
		{
			strLang = L"de";
		}
		break;
	}
	return strLang;
}