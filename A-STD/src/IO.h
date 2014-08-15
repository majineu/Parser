#ifndef __GUARD__READER
#define __GUARD__READER
#include "Pool.h"
#include "Sentence.h"
#include "DepTree.h"

class CReader
{
public:
	static	vector<SSentence *> ReadSentences(const char *pszInPath, CPool &rPool);//, bool AddDLabel = true);
	static	vector<CDepTree *> ReadTrees(const char *pszPath, 
																			 CPool &rPool, 
																			 vector<SSentence *> *pSenVec = NULL, 
																			 bool AddDLabel = true);
	static	vector<SSentence *> ReadSenPerLine(const char *pszInPath, CPool &rPool);
	static	bool ReadDepConfig(const char *pszCfgPath); 
	static const int BUF_LEN = 65536;	// hard code input buffer length
};


class CWriter
{
public:
	static  bool WriteSentence(vector<SSentence *> &senVec, const char *pszPath);
};

#endif

