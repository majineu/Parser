#ifndef __HASH_H__
#define __HASH_H__
#include <cstdlib>
#include <string>
using std::string;
using std::wstring;
typedef int NCLP_INT32 ; 
class WStrHash
{
public:
	size_t operator()(const wchar_t * pwz) const 
	{
		const wchar_t *buf = pwz;
		unsigned int index = 1315423911;
		while(*buf)
			index^=((index<<5) + (index>>2) + *buf++);// 5.92

		return (index & 0x7FFFFFFF);
	}

	size_t operator()(const wstring & refStr) const
	{
		return operator()(refStr.c_str());
	}
};


class StrHash
{
public:
	size_t operator()(const char * psz) const 
	{
		const char *buf = psz;
		unsigned int index = 1315423911;
		while(*buf)
			index^=((index<<5) + (index>>2) + *buf++);// 5.92

		return (index & 0x7FFFFFFF);
	}

	size_t operator()(const std::string & refStr) const
	{
		return operator()(refStr.c_str());
	}
};

#endif  /*__HASH_H__*/
