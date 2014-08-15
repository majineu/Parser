/*************************************************************************************
 * Copyright 2012 NEU NLP Lab. All rights reserved.
 * File Name: code_conversion.h
 * Author: Bai Longfei
 * Email: s-kira@live.cn
 * Time: 2012.07.29
 * Create for MT-PreProcessing Engine
 *************************************************************************************/

#ifndef __CODE_CONVERSION_H__
#define __CODE_CONVERSION_H__

#include <string>
#include <locale.h>

#ifndef WIN32
#include <cstdlib>
#else
#include <Windows.h>
#endif

using std::string;
using std::wstring;

class CCodeConversion
{
public:
	static wstring UTF8ToUnicode(const string& line)
	{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.UTF-8");
		size_t size = mbstowcs(NULL, line.c_str(), 0);
		wchar_t* wcstr = new wchar_t[size + 1];
		if (!wcstr)
			return L"";
		mbstowcs(wcstr, line.c_str(), size+1);
#else
		size_t size = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, NULL, 0);
		wchar_t* wcstr = new wchar_t[size];
		if (!wcstr)
			return L"";
		MultiByteToWideChar(CP_UTF8, 0, line.c_str(), -1, wcstr, size);	
#endif
		wstring final(wcstr);
		delete[] wcstr;

		return final;
	}

	static string UnicodeToUTF8(const wstring& line)
	{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.UTF-8");
		size_t size = wcstombs(NULL, line.c_str(), 0);
		char* mbstr = new char[size + 1];
		if (!mbstr)
			return "";
		wcstombs(mbstr, line.c_str(), size+1);
#else
		size_t size = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, NULL, 0, NULL, NULL);
		char* mbstr = new char[size];
		if (!mbstr)
			return "";
		WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, mbstr, size, NULL, NULL);
#endif
		string final(mbstr);
		delete[] mbstr;

		return final;
	}

	static wstring GBToUnicode(string& line)
	{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.GB2312");
		size_t size = mbstowcs(NULL, line.c_str(), 0);
		wchar_t* wcstr = new wchar_t[size + 1];
		if (!wcstr)
			return L"";
		mbstowcs(wcstr, line.c_str(), size+1);
#else
		size_t size = MultiByteToWideChar(CP_ACP, 0, line.c_str(), -1, NULL, 0);
		wchar_t* wcstr = new wchar_t[size];
		if (!wcstr)
			return L"";
		MultiByteToWideChar(CP_ACP, 0, line.c_str(), -1, wcstr, size);	
#endif
		wstring final(wcstr);
		delete[] wcstr;

		return final;
	}

	static string UnicodeToGB(wstring& line)
	{
#ifndef WIN32
		setlocale(LC_ALL, "zh_CN.GB2312");
		size_t size = wcstombs(NULL, line.c_str(), 0);
		char* mbstr = new char[size + 1];
		if (!mbstr)
			return "";
		wcstombs(mbstr, line.c_str(), size+1);
#else
		size_t size = WideCharToMultiByte(CP_ACP, 0, line.c_str(), -1, NULL, 0, NULL, NULL);
		char* mbstr = new char[size];
		if (!mbstr)
			return "";
		WideCharToMultiByte(CP_ACP, 0, line.c_str(), -1, mbstr, size, NULL, NULL);
#endif
		string final(mbstr);
		delete[] mbstr;

		return final;
	}
};

inline string wstr2utf8(const wstring &wStr)
{
	return CCodeConversion::UnicodeToUTF8(wStr);
}

inline wstring utf82wstr(const string &wStr)
{
	return CCodeConversion::UTF8ToUnicode(wStr);
}

#endif
