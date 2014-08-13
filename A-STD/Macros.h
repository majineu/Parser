#ifndef  _MACROS_H
#define _MACROS_H


#include <cstdlib>
#include <cstring>
#include <float.h>
#include <wchar.h>
#include <cstdlib>
#include <cstdio>
#define CHK_NEW_FAILED(p)\
	if(p == NULL)\
{\
	printf("Error: out of memory\n%s: line%d\n",__FILE__, __LINE__);\
	exit(0);\
}

#define CHK_OPEN_FAILED(p , path)\
	if(p == NULL)\
{\
	printf("Error: fail %s open failed\n",path);\
	exit(0);\
}

inline void removeNewLine(wchar_t * buf)
{
	if(buf[ wcslen(buf) - 1 ] == L'\n')
		buf[ wcslen(buf) - 1 ] = L'\0';

	if(buf[ wcslen (buf) - 1 ] == L'\r')
		buf[ wcslen(buf) - 1 ] = L'\0';
}

inline void removeNewLine(char * buf)
{
	if(buf[ strlen(buf) - 1 ] == '\n')
		buf[ strlen(buf) - 1 ] = '\0';

	if(buf[ strlen (buf) - 1 ] == L'\r')
		buf[ strlen(buf) - 1 ] = '\0';
}


#if defined(_MSC_VER) || defined(__BORLANDC__)
/* In linux, we can use finite directly,
 * In windows, we have to use _finite instead
 */
inline int finite(double x) { return _finite(x); }
inline int nan(double x){return _isnan(x);}
#endif

#endif
