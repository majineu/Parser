#ifndef __UTIL_H__
#define __UTIL_H__
#include <vector>
#include <cstring>
#include <string>
#include <locale.h>
#include <wchar.h>
#include <cstdlib>
#include "Pool.h"
#ifndef WIN32
#include <cstdlib>
#else
#include <Windows.h>
#endif

using std::vector;
using std::string;
using std::wstring;
vector<wchar_t *> Split(wchar_t *pBuf, const wchar_t *pDelem);
vector<char *> Split(char *pBuf, const char *pDelem);
wchar_t * copyWcs(const wchar_t *, CPool *);



inline string wstr2utf8(const wstring &source)
{
#ifndef WIN32
	setlocale(LC_ALL, "zh_CN.UTF-8");
	size_t size = wcstombs(NULL, source.c_str(), 0);
	char* mbstr = new char[size + 1];
	if (!mbstr)
		return "";
	wcstombs(mbstr, source.c_str(), size+1);
#else
	size_t size = WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, NULL, 0, NULL, NULL);
	char* mbstr = new char[size];
	if (!mbstr)
		return "";
	WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, mbstr, size, NULL, NULL);
#endif
	string final(mbstr);
	delete[] mbstr;

	return final;
}

inline wstring utf82wstr(const string& source)
{
#ifndef WIN32
	setlocale(LC_ALL, "zh_CN.UTF-8");
	size_t size = mbstowcs(NULL, source.c_str(), 0);
	wchar_t* wcstr = new wchar_t[size + 1];
	if (!wcstr)
		return L"";
	mbstowcs(wcstr, source.c_str(), size+1);
#else
	size_t size = MultiByteToWideChar(CP_UTF8, 0, source.c_str(), -1, NULL, 0);
	wchar_t* wcstr = new wchar_t[size];
	if (!wcstr)
		return L"";
	MultiByteToWideChar(CP_UTF8, 0, source.c_str(), -1, wcstr, size);	
#endif
	wstring final(wcstr);
	delete[] wcstr;
	return final;
}

inline wstring gb2wstr(const string & source)
{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.GB2312");
		size_t size = mbstowcs(NULL, source.c_str(), 0);
		wchar_t* wcstr = new wchar_t[size + 1];
		if (!wcstr)
			return L"";
		mbstowcs(wcstr, source.c_str(), size+1);
#else
		size_t size = MultiByteToWideChar(CP_ACP, 0, source.c_str(), -1, NULL, 0);
		wchar_t* wcstr = new wchar_t[size];
		if (!wcstr)
			return L"";
		MultiByteToWideChar(CP_ACP, 0, source.c_str(), -1, wcstr, size);	
#endif
		wstring final(wcstr);
		delete[] wcstr;
		return final;
}

inline string wstr2gb(const wstring & source)
{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.GB2312");
		size_t size = wcstombs(NULL, source.c_str(), 0);
		char* mbstr = new char[size + 1];
		if (!mbstr)
			return "";
		wcstombs(mbstr, source.c_str(), size+1);
#else
		size_t size = WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, NULL, 0, NULL, NULL);
		char* mbstr = new char[size];
		if (!mbstr)
			return "";
		WideCharToMultiByte(CP_ACP, 0, source.c_str(), -1, mbstr, size, NULL, NULL);
#endif
		string final(mbstr);
		delete[] mbstr;
		return final;
}

#endif  /*__UTIL_H__*/
