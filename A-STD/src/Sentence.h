#ifndef __SENTENCE_H__
#define __SENTENCE_H__

#include <wchar.h>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include "Pool.h"
#include "Config.h"
#include "util.h"
#include "Maps.h"
//#include "code_conversion.h"
using std::map;
class WStrHash;
using std::unordered_set;
using std::unordered_map;

struct SSenNode
{
	static SSenNode * MakeNode(CPool &rPool)
	{
		SSenNode *pNode = (SSenNode *)rPool.Allocate(sizeof(SSenNode));
		memset(pNode, 0, sizeof(SSenNode));
		return pNode;
	}

	SSenNode()								{memset(this, 0, sizeof(*this));}
	void InitIDs()
	{
		s_iEnd 		 = s_nbW.GetId(s_sEnd, true);
		
		if (CConfig::bRmNonPunc)
			s_iPuncNul = -1;
		else
			s_iPuncNul = s_nbW.GetId(s_sPuncNul, true);
		
		m_nSep = 0;
		assert(m_pwzWord != NULL && m_pwzTag != NULL);// && m_pwzPunc != NULL);
		m_iP = s_iPuncNul;//m_pwzPunc == NULL ? s_unkID : s_nbP.GetId(m_pwzPunc, true);		
		m_iW = s_nbW.GetId(m_pwzWord,  true);		
		m_iT = s_nbW.GetId(m_pwzTag,   true);
		m_iL = s_nbW.GetId(m_pwzLabel, true);
	}

	static int	BMap(int ID)								
	{
		unordered_map<int, int>::iterator iter = s_mBPunc.find(ID);
		return iter == s_mBPunc.end() ? -1:iter->second;
	}
	
	static int	EMap(int ID)								
	{
		unordered_map<int, int>::iterator iter = s_mEPunc.find(ID);
		return iter == s_mEPunc.end() ? -1:iter->second;
	}

	static void InitPuncIDSet();
	static int  SEndID()										{return s_iEnd;}   
	static int  Punc2SepID(int puncID)			{return s_mPunc2Sep[puncID];}
	static wstring GetKey(int id)						{return wstring(s_nbW.GetKey(id)->c_str());}
	static int  GetId(const wstring &key)		{return s_nbW.GetId(key,false);}
	static bool IsPunc(int id)							{return s_sPunc.find(id) != s_sPunc.end();}
  static bool IsNonBEPunc(int id)         {return IsPunc(id)  && !IsBEPunc(id);}
	static bool IsBPunc(int id)							{return s_sBPunc.find(id) != s_sBPunc.end();}
	static bool IsEPunc(int id)							{return s_sEPunc.find(id) != s_sEPunc.end();}
	static bool IsBEPunc(int id)						{return IsBPunc(id) || IsEPunc(id);}
	static bool IsBSep(int sepID)						{return s_mBPunc.find(sepID) != s_mBPunc.end();}
	static bool IsESep(int sepID)						{return s_mEPunc.find(sepID) != s_mEPunc.end();}
	static void SaveNumberer(const string &path);
	static void LoadNumberer(const string &path);
	//  members that are not changed
	wchar_t *	m_pwzWord;
	wchar_t *	m_pwzTag;
	wchar_t * m_pwzPunc;
	wchar_t * m_pwzLabel;
	int 			m_iW;
	int				m_iT;
	int				m_iP;
	int				m_iL;
	int       m_nSep;							// sep punct it contains
	static    CNumberer<wstring, WStrHash> s_nbW;
	static    CNumberer<wstring, WStrHash> s_nbT;
	static    CNumberer<wstring, WStrHash> s_nbP;
	static  	wstring 						s_puncs[];
	static  	unordered_set<int> 	s_sPunc;
	static  	unordered_set<int> 	s_sBPunc;
	static  	unordered_set<int> 	s_sEPunc;
	static  	unordered_map<int, int> 	s_mBPunc;
	static  	unordered_map<int, int> 	s_mEPunc;
	static  	unordered_map<int, int> 	s_mPunc2Sep;
//	static  const  int s_unkID = -1; 
	static  int s_iEnd;//  = 0;
	static  int s_iPuncNul;
	static  const  wchar_t *s_sEnd;
	static  const  wchar_t *s_sPuncNul;
};
//----------------------------------------------------------------------------

struct SSentence
{

	static vector<SSentence *> CopySentences(vector<SSentence *> &rSens,  CPool &rPool)
	{
		vector<SSentence *> SenCopied;
		for (size_t i = 0; i < rSens.size(); ++i)
		{
			SSentence *pSen = (SSentence *)rPool.Allocate(sizeof(SSentence));
			pSen->Init(rPool,  *rSens[i]);
			SenCopied.push_back(pSen);
		}
		return SenCopied;
	}

  int GetPuncNum()
  {
    int nPunc = 0;
    for (int i = 0; i < m_len; ++i)
      if (CConfig::puncsSet.find(m_pNodes[i]->m_pwzTag) != CConfig::puncsSet.end())
       ++nPunc;
    return nPunc;
  }


	void Display(FILE *fp = stderr)
	{
		fprintf(fp, "len %d\n", m_len);
		for (int i = 0; i < m_len; ++i)
			fprintf(fp, "%s_%s_%s  ",		wstr2utf8(Word(i)).c_str(), 
							wstr2utf8(Tag(i)).c_str(), 	wstr2utf8(Punc(i)).c_str());
		fwprintf(fp, L"\n");
	}


	SSentence()
	{
		m_pNodes = NULL;
		m_len = 0;
	}


	void Init(CPool &rPool, SSentence &refSen)
	{
		m_len = (int)refSen.Length();
		m_pNodes = (SSenNode **)rPool.Allocate(sizeof(SSenNode*) * m_len);
		for (int i = 0; i < m_len; ++i)
		{
			m_pNodes[i] = SSenNode::MakeNode(rPool);
			m_pNodes[i]->m_pwzWord = copyWcs(refSen.Word(i), &rPool);
			m_pNodes[i]->m_pwzTag  = copyWcs(refSen.Tag(i),  &rPool);
			m_pNodes[i]->m_pwzPunc = copyWcs(refSen.Punc(i), &rPool);
			m_pNodes[i]->m_pwzLabel = copyWcs(refSen.Label(i), &rPool);
			m_pNodes[i]->InitIDs();
		}
	}

	void Display(FILE *fp, vector<int> &vHIDs);
	void PrintMaltFormat(FILE *fp, vector<int> &vHIDs);


	static SSentence 
	*CreateSentence(CPool &rPool, vector<wstring> lines, 
	 							  bool copyTag, vector<int> *pHidxVec = NULL)
	{
		SSentence *pSen = (SSentence*)rPool.Allocate(sizeof(SSentence));
		pSen->m_pNodes = (SSenNode **)rPool.Allocate(sizeof(SSenNode*) * lines.size());
		pSen->m_len	   = lines.size();
	
		const int BUF_LEN = 65535;
		wchar_t buf[BUF_LEN];
		vector<wchar_t *> itemVec;
		wchar_t nullPunc[] = L"null";
		enum IDX {_WORD, _TAG, _HEAD_INDEX, _D_LABEL, _PUNC};

		if (pHidxVec != NULL)
			pHidxVec->resize(lines.size());
		
		for (size_t i = 0; i < lines.size(); ++i)
		{
			pSen->m_pNodes[i] = SSenNode::MakeNode(rPool);
			wcscpy(buf, lines[i].c_str());
		
			itemVec = Split(buf, L" \t\r\n");// itemVec);
			if (itemVec.size() != 5)
				itemVec.push_back(nullPunc);

			pSen->m_pNodes[i]->m_pwzWord = copyWcs(itemVec[_WORD], &rPool);//(wchar_t *)rPool.Allocate(sizeof(wchar_t) * (wcslen(itemVec[_WORD]) + 1) );
			pSen->m_pNodes[i]->m_pwzTag  = copyWcs(itemVec[_TAG],  &rPool);//rPool.Allocate(sizeof(wchar_t) * (wcslen(itemVec[_TAG]) + 1));
			pSen->m_pNodes[i]->m_pwzLabel= copyWcs(itemVec[_D_LABEL],  &rPool);//rPool.Allocate(sizeof(wchar_t) * (wcslen(itemVec[_TAG]) + 1));
			pSen->m_pNodes[i]->m_pwzPunc = copyWcs(itemVec[_PUNC], &rPool);
			pSen->m_pNodes[i]->InitIDs();
			if (pHidxVec != NULL)
      {
        int hIdx = -1;
        if (swscanf(itemVec[_HEAD_INDEX], L"%d", &hIdx) != 0)
          (*pHidxVec)[i] = hIdx - 1;
        else
				{
					fprintf(stderr, "Error: failed to convert head index\n");
					exit(0);
				}
      }
		}

		return pSen;
	}


	int Length()												{return m_len;}
	int WID(int i)											{return m_pNodes[i]->m_iW;}
	int TID(int i)											{return m_pNodes[i]->m_iT;}
	int PuncID(int i)										{return m_pNodes[i]->m_iP;}
	int LabelID(int i)									{return m_pNodes[i]->m_iL;}
	int SepNum(int i)										{return m_pNodes[i]->m_nSep;}
	bool AppendPuncs(CPool &rPool, vector<int> &vHIDs);
	void RmPuncsNearBE(vector<int> &vHIDs, map<int, int> &rmCounter);
	void ConvertHpy(CPool &rPool);
	void AttachNonSepPunc(vector<int> &vHIDs);
	wchar_t * Word(int i)
	{
		assert(i < m_len);
		assert(m_pNodes != NULL);
		return m_pNodes[i]->m_pwzWord;
	}

	wchar_t * Tag(int i)
	{
		assert(i < m_len);
		assert(m_pNodes != NULL);
		return m_pNodes[i]->m_pwzTag;
	}

	wchar_t *Punc(int i)
	{
		assert(i < m_len && i >= 0 && m_pNodes != NULL);
		return m_pNodes[i]->m_pwzPunc;
	}

	wchar_t *Label(int i)
	{
		assert(i < m_len && i >= 0 && m_pNodes != NULL);
		return m_pNodes[i]->m_pwzLabel;
	}

	void SetTag(int i , wchar_t *pwzTag)
	{
		assert(i < m_len && m_pNodes != NULL);
		m_pNodes[i]->m_pwzTag = pwzTag;
	}

	~SSentence()
	{
		if (m_pNodes == NULL)
			return;
		for (int i = 0; i < m_len; ++i)
			m_pNodes[i] = NULL;

		m_pNodes = NULL;
		m_len = 0;
	}
	
	SSenNode **m_pNodes;
	int		 m_len;
};

typedef SSentence _SENTENCE;
#endif  /*__SENTENCE_H__*/
