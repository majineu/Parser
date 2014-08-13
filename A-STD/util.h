#include <vector>
#include <cstring>
#include <wchar.h>
#include "Pool.h"
#ifndef __UTIL_H__
#define __UTIL_H__

using std::vector;
vector<wchar_t *> Split(wchar_t *pBuf, const wchar_t *pDelem);
vector<char *> Split(char *pBuf, const char *pDelem);
wchar_t * copyWcs(const wchar_t *, CPool *);
#endif  /*__UTIL_H__*/
