#pragma once
#include <list>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <cassert>
#include "Pool.h"
#include "Macros.h"
#include "code_conversion.h"
#include "Hash.h"
using std::list;
using std::wstring;
using std::vector;


#define MIN_SCORE -1.7e308
#define MAX_SCORE 1.7e308

class CSzHashMap
{
public:

	struct HashNode
	{
		NCLP_INT32  second;
		char *first;
		HashNode *pNext;
	};

	typedef HashNode * iterator;
	iterator end()													{return End;}
	NCLP_INT32  size()											{return m_nElement;}
	
	CSzHashMap():End(NULL)
	{
		m_size = 37;
		m_nElement = 0;
		m_pTable = new HashNode *[m_size];
		CHK_NEW_FAILED(m_pTable);
		memset(m_pTable, 0, sizeof(*m_pTable)*m_size);
	}


	~CSzHashMap()
	{
		delete[] m_pTable;
		m_pTable = NULL;
	}

	iterator Insert(const char *pKey, NCLP_INT32  val)
	{
		iterator it = Find(pKey);
		if (it != End)
			return it;

		if (++m_nElement/(double)m_size > 0.7)
			grow();

		NCLP_INT32  index = m_hasher(pKey) % m_size;
		
		/* initialize new node */
		HashNode *pNewNode = (HashNode *) m_pool.Allocate(sizeof(HashNode));
		pNewNode->second = val;

		pNewNode->first = (char *) m_pool.Allocate(sizeof(char) * (strlen(pKey) + 1));
		strcpy(pNewNode->first, pKey);
		pNewNode->first[strlen(pKey)] = 0;

		/* insert at head */
		pNewNode->pNext = m_pTable[index];
		m_pTable[index] = pNewNode;

		return it = pNewNode;
	}


	iterator Find(const char *pKey)
	{
		NCLP_INT32  index = m_hasher(pKey) % m_size;
		iterator it = m_pTable[index];

		while (it != End)
		{
			if (strcmp(it->first, pKey) == 0)
				return it;
			else
				it = it->pNext;
		}
		return End;
	}

private:

	bool grow()
	{
		static NCLP_INT32  sizes[] ={193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 
								196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843};
		static NCLP_INT32  sizeLen = sizeof(sizes)/sizeof(size_t);
		
		NCLP_INT32  k = 0;
		for(k = 0; k < sizeLen && sizes[k] <= m_size; ++k);
	
		NCLP_INT32  oldSize = m_size;
		HashNode **oldTable = m_pTable;

		if(k != sizeLen)
			m_size = sizes[k];
		else
			m_size = nextPrime(m_size * 2);

		m_pTable = new HashNode *[m_size];
		CHK_NEW_FAILED(m_pTable);
		memset(m_pTable, 0, sizeof(HashNode *) * m_size);

		/* rehash elements */
		NCLP_INT32  nProcessed = 0;
		for (NCLP_INT32  i = 0; i < oldSize; ++i)
		{
			iterator iter = oldTable[i];
			while (iter != End)
			{
				// be careful
				++nProcessed;
				NCLP_INT32  index = m_hasher(iter->first) % m_size;
				iterator oldNext = iter->pNext;
				
				iter->pNext = m_pTable[index];
				m_pTable[index] = iter;

				iter = oldNext;
			}
		}

		delete[] oldTable;
		if (nProcessed != m_nElement - 1)
		{
			fprintf(stderr, "Error: Elements inconsistent %d vs %d\n", nProcessed, m_nElement);
			return false;
		}

		return true;
	}

	size_t nextPrime(size_t candidate)
	{
		candidate |= 1;
		while (((size_t)-1) != candidate)
		{
			size_t div = 3;
			size_t squ = div*div;
			while(squ < candidate && candidate % div)
			{
				squ += ++div * 4;
				++div;
			}

			if(candidate % div != 0)
				break;
			candidate += 2;
		}
		return candidate;
	}

	const iterator End;
	int		   m_nElement;
	int		   m_size;
	CPool	   m_pool;


	HashNode   **m_pTable;
	StrHash  m_hasher;
};

class CStrHashMap
{
public:
	typedef std::pair<wstring, size_t>    ITEM_TYPE;
	typedef list<ITEM_TYPE>::iterator  iterator;

private:
	list<ITEM_TYPE> m_lItemList;
	size_t m_nTableSize;
	size_t m_nItemNum;
	double m_occupyRate;
	struct SBucket
	{
		iterator beg, end;
	};
	WStrHash m_Hash;
	SBucket *m_pBuckets;
	
private:
	size_t nextPrime(size_t candidate)
	{
		candidate |= 1;
		while (((size_t)-1) != candidate)
		{
			size_t div = 3;
			size_t squ = div*div;
			while(squ < candidate && candidate % div)
			{
				squ += ++div * 4;
				++div;
			}

			if(candidate % div != 0)
				break;
			candidate += 2;
		}
	
		return candidate;
	}

	bool Grow()
	{
		/* 1. reallocate space */
		static size_t sizes[] ={	193, 389, 769, 1543, 3079, 6151, 12289, 24593,
									49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
									12582917, 25165843 };
		static size_t sizeLen = sizeof(sizes)/sizeof(size_t);
		size_t k = 1;
		for(k = 1; k < sizeLen && sizes[k] <= m_nTableSize; ++k);
	
		if(k != sizeLen)
			m_nTableSize = sizes[k];
		else
			m_nTableSize = nextPrime(m_nTableSize * 2);


		delete[] m_pBuckets;
		m_pBuckets = new SBucket[m_nTableSize];
		CHK_NEW_FAILED(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;

		/* 2. rehash */
		iterator it = --end, beg = m_lItemList.begin() ;
		while(it != beg)
		{
			size_t index = m_Hash(it->first) % m_nTableSize;
			if(m_pBuckets[index].beg == m_pBuckets[index].end)
			{	
				/* 1. empty buckets */
				m_pBuckets[index].beg = m_pBuckets[index].end = it--;
				m_pBuckets[index].end ++;
			}
			else if(--m_pBuckets[index].beg != it)
			{	
				/* 2. here we need to rearrange the current element */
				m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it--);
				++m_pBuckets[index].beg;
			}
			else 
				/* 3. only update the iterators */
				--it;
		}

		/* 3. handle the last item */
		size_t index = m_Hash(it->first) % m_nTableSize;
		if(m_pBuckets[index].beg == m_pBuckets[index].end)
		{	
			/* 1. empty buckets */
			m_pBuckets[index].beg = m_pBuckets[index].end = it;
			m_pBuckets[index].end ++;
		}
		else if(--m_pBuckets[index].beg != it)
		{	
			/* 2. here we need to rearrange the current element */
			m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it);
			++m_pBuckets[index].beg;
		}

		return true;
	}

public:
	CStrHashMap()
	{
		m_nTableSize = 97, m_nItemNum = 0;
		m_occupyRate = 0.8;
		
		m_pBuckets = new SBucket[ m_nTableSize ];
		CHK_NEW_FAILED(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;
	}

	~CStrHashMap()													{delete[] m_pBuckets;	}
	iterator begin()												{return m_lItemList.begin();}
	iterator end()													{return m_lItemList.end();}
	size_t size()														{return m_nItemNum;}

	iterator Insert(const wstring & key, size_t val)
	{
		return Insert(key.c_str(), val);
	}

	iterator Insert(const wchar_t * pwz, size_t val)
	{
		size_t index = m_Hash(pwz) % m_nTableSize;
		SBucket *bucket = &m_pBuckets[index];
		iterator iter = bucket->beg;

		for (; iter != bucket->end; ++iter)
		{
			if (wcscmp(iter->first.c_str(), pwz) == 0)
				return iter;
		}

		if((double)(++m_nItemNum)/m_nTableSize > m_occupyRate)
		{
			Grow();
			index = m_Hash(pwz) % m_nTableSize;
			bucket = & m_pBuckets[index];	
		}

		if(bucket->beg == bucket->end)
		{
			bucket->end = bucket->beg = m_lItemList.insert(m_lItemList.begin(), ITEM_TYPE(pwz, val));
			return bucket->end++;
		}
		else
			return m_lItemList.insert(bucket->end, ITEM_TYPE(pwz, val));
	}

	iterator Find(const wstring & refStr)
	{
		return Find(refStr.c_str());
	}

	iterator Find(const wchar_t * pwz)
	{
		if(m_nItemNum == 0)
			return m_lItemList.end();

		size_t index = m_Hash(pwz) % m_nTableSize;
		SBucket &backet = m_pBuckets[index];
		iterator it = backet.beg;
		for(; it != backet.end ; ++it)
			if(wcscmp(it->first.c_str(), pwz) == 0)
				return it;

		return m_lItemList.end();
	}
};


class CIDMap
{
public:
	enum ACTION_TYPE {SHIFT, LEFT_REDUCE, RIGHT_REDUCE};
	const static size_t UNK_ID = (size_t) -1;
	static bool ReadDepLabelFile(const string &strDepFile)
	{
		FILE *fp = fopen(strDepFile.c_str(), "r");
		CHK_OPEN_FAILED(fp, strDepFile.c_str()); 
		wchar_t buf[100];

		while (fgetws(buf, 100, fp))
		{
			removeNewLine(buf);
			int len = wcslen(buf);
			while (len-1 >=0 && (buf[len - 1] == ' '|| buf[len - 1] == '\t'))
				buf[len - 1] = 0;

			/* skip empty line */
			if (wcslen(buf) == 0)
				continue;

			AddDependencyLabel(buf);
		}
		fclose(fp);

		BuildOutcomeIds();

		fwprintf(stderr, L"\n");
		fwprintf(stderr, L"# dependency label: %d\n", (int)GetDepLabelNum());
		fwprintf(stderr, L"# outcomes:         %d\n", (int)GetOutcomeNum());
		return true;
	}

	static bool SaveDepLabel(const string &strPath)
	{
		FILE *fp = fopen(strPath.c_str(), "w");
		CHK_OPEN_FAILED(fp, strPath.c_str());
		for (size_t i = 0; i < m_depLabelVec.size(); ++i)
			fwprintf(fp, L"%ls\n", m_depLabelVec[i]->c_str());

		fclose(fp);
		return true;
	}

	static size_t AddDependencyLabel(const wchar_t * pwzDepLabel){return add(pwzDepLabel, DEP_MAP);}
	static size_t AddPredict(const wchar_t * pwzFeature)		{return add(pwzFeature, PRED_MAP);}
	static size_t GetPredictId(const wchar_t *pwzFeature)		{return getId(pwzFeature, PRED_MAP);}
	static size_t GetOutcomeId(const wchar_t *pwzLabel)			{return getId(pwzLabel, OUTCOME_MAP);}
	static const int GetDepLabelId(const wchar_t *pwz)			{return getId(pwz, DEP_MAP);}
	static CStrHashMap & GetDepLabelMap()										{return m_depLabelMap;}
	static vector<wstring *> & GetDepLabelVec()							{return m_depLabelVec;}
	static size_t GetPredNum()															{return m_predVec.size();}
	static size_t GetOutcomeNum()														{return m_outcomeVec.size();}
	static size_t GetDepLabelNum()													{return m_depLabelMap.size();	}

	static const wchar_t * GetPredict(size_t fid)
	{
		if (fid < m_predVec.size())
			return m_predVec[fid]->c_str();
		else
		{
			fwprintf(stderr, L"Error: Invalid predict id %d\n", fid);
			return NULL;
		}
	}

	static const wchar_t * GetDepLabel(const int Id) 
	{
		assert(Id < m_nLabel);// m_depLabelVec.size())
		return m_depLabelVec[Id]->c_str();				//Ji 2012-06-13
	}


	static int GetOutcomeId(const ACTION_TYPE action, 
													const int label)
	{
		if (action == SHIFT)
			return 0;
		
		int oId = 1 + label;
		return action == LEFT_REDUCE ? oId: (oId + m_nLabel);
	}

	static void BuildOutcomeIds()
	{
		if (m_depLabelMap.size() == 0)
		{
			fwprintf(stderr, L"Error: Building class label with empty depLabelMap or tagMap\n");
			exit(0);
		}

		wchar_t buf[200] = L"shift";
		add(buf, OUTCOME_MAP);
	
		m_nLabel = (int)m_depLabelVec.size();
		for (int iter = 0; iter < 2; ++ iter)
		{
			for (int i= 0; i < (int)m_depLabelVec.size(); ++i)
			{
				wcscpy(buf, iter == 0 ? L"left_": L"right_");
				wcscat(buf, m_depLabelVec[i]->c_str());
				add(buf, OUTCOME_MAP);
			}
		}
	}

	static ACTION_TYPE Interprate(const int classId,   
															  int &label)
	{
		label = classId - 1;
		if (classId == 0)
			return SHIFT;
		
		if (classId > m_nLabel)
		{
			label -= m_nLabel;
			return RIGHT_REDUCE;
		}
		else
			return LEFT_REDUCE;
	}

	


private:
	enum MAP_TYPE {PRED_MAP, OUTCOME_MAP, DEP_MAP};

	static size_t getId(const wchar_t *pwzStr, MAP_TYPE type)
	{
		CStrHashMap &refMap = type == PRED_MAP ? m_predMap:
							  type == OUTCOME_MAP ? m_outcomeMap:m_depLabelMap;//:m_tagMap;
									
		CStrHashMap::iterator iter = refMap.Find(pwzStr);
		return iter != refMap.end() ? iter->second : UNK_ID;
	}

	static size_t add(const wchar_t *pwzStr, MAP_TYPE type)
	{
		CStrHashMap &refMap = type == PRED_MAP ? m_predMap:
							  type == OUTCOME_MAP ? m_outcomeMap:m_depLabelMap;//:m_tagMap;
		
		int size = refMap.size();
		CStrHashMap::iterator iter = refMap.Find(pwzStr);
		if (iter != refMap.end())
			return iter->second;
		
		iter = refMap.Insert(pwzStr, size);
		assert(iter != refMap.end());
	
		/* register in the corresponding vector */
		std::vector<wstring *> &refVec = type == PRED_MAP ? m_predVec:
										 type == OUTCOME_MAP ? m_outcomeVec: m_depLabelVec;
		refVec.push_back(&iter->first);
		return iter->second;
	}

	static CStrHashMap m_predMap;		// predMap
	static CStrHashMap m_outcomeMap;	// class label
	static CStrHashMap m_depLabelMap;	// dependency label

	static int m_nLabel;
	static std::vector<wstring *> m_predVec;
	static std::vector<wstring *> m_outcomeVec;
	static std::vector<wstring *> m_depLabelVec;
};



namespace Perceptron
{
struct SSimpleEvent
{
	size_t size;
	int oId;
	int *pPred;
	SSimpleEvent *pNext;
};


class CAccEventCreator
{
public:
	CAccEventCreator()						{};
	void ResetMem()								{m_Pool.Recycle();}


	SSimpleEvent * CreateSimpleEvent()
	{
		SSimpleEvent *pSimpleEvent = (SSimpleEvent *)m_Pool.Allocate(sizeof(SSimpleEvent));
		assert(pSimpleEvent);
		pSimpleEvent->oId = pSimpleEvent->size = 0;
		pSimpleEvent->pPred = NULL;
		pSimpleEvent->pNext = NULL;
		return pSimpleEvent;
	}

	bool UpdateEventVec(SSimpleEvent **pHead, const vector<int> &rFIdVec, int outcomeId)
	{
		if (pHead == NULL && rFIdVec.size() == 0)
			return false;

		size_t fNum = rFIdVec.size();
		
		// 1. initialize outcome id 
		SSimpleEvent *pSEvent = CreateSimpleEvent();
		pSEvent->oId = outcomeId;
		
		// 2. initialize feature id array 
		pSEvent->size = fNum;
		pSEvent->pPred = (int*)m_Pool.Allocate(sizeof(int) * fNum);
		CHK_NEW_FAILED(pSEvent->pPred);

		for(size_t i = 0; i < fNum; ++i)
			pSEvent->pPred[i] = rFIdVec[i];

		// 3. insert at head 
		pSEvent->pNext = *pHead;
		*pHead = pSEvent;
		
		return true;
	}

	
private:
	CPool m_Pool;
};


// forward declearation
class CPerceptronScorer;


class CPerceptronScorer
{
private:
	size_t 	m_nParamNum;
	size_t 	m_nPredNum;
	size_t 	m_nOutcomeNum;
	size_t 	m_It;
	int 		correctNum;
	int 		totalNum;
	int   	m_nDelay;
	int    *m_nUps;
	double *m_pParams;
	double *m_pSumParams;
	const static int MAX_LENGTH = 65535;


public:
	CPerceptronScorer(); 
	~CPerceptronScorer();
	bool Load(FILE *fp);
	bool Save(FILE *fp);
	bool Load(const char * pszModelFile);
	bool Save(const char * pszModelFile);
	bool TrainingWithAccEvent(SSimpleEvent *pGoldBeg,   
														SSimpleEvent *pGoldEnd, 
														SSimpleEvent *pWinnerBeg, 
														SSimpleEvent *pWinnerEnd, 
														double rate);
	
	bool Init(int nDelay)
	{
		m_nPredNum		= CIDMap::GetPredNum();
		m_nOutcomeNum	= CIDMap::GetOutcomeNum();
		m_nParamNum		= m_nPredNum * m_nOutcomeNum;

		if (m_nParamNum == 0)
			return false;

		m_pSumParams = new double[m_nParamNum];
		m_pParams	 	 = new double[m_nParamNum];
		m_nUps		 	 = new int[m_nParamNum];
		CHK_NEW_FAILED(m_pSumParams);
		CHK_NEW_FAILED(m_pParams);
		CHK_NEW_FAILED(m_nUps);
		
		/* all parameter values are initialized to zeros */
		memset(m_pSumParams, 0, sizeof(double) * m_nParamNum);
		memset(m_pParams, 0, sizeof(double) * m_nParamNum);
		memset(m_nUps, 0, sizeof(int) * m_nParamNum);

		m_nDelay = nDelay;
		fprintf(stderr, "nDelay %d\n", m_nDelay);
		return true;
	}

	double GetVal(int FId, int OId, bool isAvg)
	{
		assert (FId < (int)m_nPredNum);

		int index = OId * m_nPredNum + FId;
		if (isAvg == true)
			return m_pParams[index];
		else
			return m_pParams[index] - m_pSumParams[index]/m_It;
	}

	void ComputeAvgParam(vector<double> &vParam)
	{
		vParam.resize(m_nParamNum);
		ComputeAvgParam(&vParam[0]);
	}

	void ComputeAvgParam(double * pWeights)
	{
		bool verbose = false;
		if(verbose == true)
			std::cerr<<"m_It:"<<m_It<<std::endl;
		for(size_t i = 0; i < m_nParamNum; ++i )
		{
			pWeights[i] = m_pParams[i] - m_pSumParams[i]/m_It;
			if(i < 100 && verbose == true)
				std::cerr<<"w:"<<m_pParams[i]<<" wsum:"<<m_pSumParams[i]<<" avgW:"<<pWeights[i]<<std::endl;
		}
	}

	double * GetParams()												{return m_pParams;}
	size_t GetParamNum()												{return m_nParamNum;}
	void SetParams(double *param)								{m_pParams = param;}
	void ResetIt()															{m_It = 1;}
	void IncIt()																{++m_It;}
	void GetStatistics(int & corr, int &total)	{corr = correctNum,total = totalNum;}
	void ResetStatistics()											{correctNum = totalNum = 0;}

	// 2012 07 08
	size_t Eval(int * pFIds, int nFid, vector<double> &OutcomeScoreVec);

private:
	size_t eval(const vector<size_t> &predVec, vector<double> & OutcomeScoreVec);
};


class CCompactor
{
public:
	static bool Compressing(CPerceptronScorer &rScorer, const char * compactModel, bool isAvg);
	static bool CompressingAll(CPerceptronScorer &rScorer, const char * compactModel,
		bool isAvg, vector<std::string> & templates, NCLP_INT32  beamSize);
};

}
