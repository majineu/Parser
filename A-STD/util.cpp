#include "util.h"
vector<wchar_t *> Split(wchar_t *pBuf, const wchar_t *pDelem)
{
	vector<wchar_t *> resVec;
	if (pBuf == NULL || pDelem == NULL)
		return resVec;

	wchar_t *p = pBuf;
	while (true)
	{
		while (*p && wcschr(pDelem, *p) != NULL)
			*p ++ = 0;

		if (!*p)
			break;

		resVec.push_back(p);
		while (*p && wcschr(pDelem, *p) == NULL)
			p++;
	}

	return resVec;
}

vector<char *> Split(char *pBuf, const char *pDelem)
{
	vector<char *> resVec;
	if (pBuf == NULL || pDelem == NULL)
		return resVec;

	char *p = pBuf;
	while (true)
	{
		while (*p && strchr(pDelem, *p) != NULL)
			*p ++ = 0;

		if (!*p)
			break;

		resVec.push_back(p);
		while (*p && strchr(pDelem, *p) == NULL)
			p++;
	}

	return resVec;
}

wchar_t *copyWcs(const wchar_t *pwzStr, 
								 CPool *pPool)
{
	if (pwzStr == NULL)
		throw("Error: copy null str\n");

	wchar_t *pwzRes = NULL;
	if (pPool == NULL)
		pwzRes = new wchar_t[wcslen(pwzStr) + 1];
	else
		pwzRes = (wchar_t *)pPool->Allocate(sizeof(wchar_t) * (wcslen(pwzStr) + 1));

	CHK_NEW_FAILED(pwzRes);
	wcsncpy(pwzRes, pwzStr, wcslen(pwzStr) + 1);
	return pwzRes;
}
