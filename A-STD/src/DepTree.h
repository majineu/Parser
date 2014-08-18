#pragma once
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <list>
#include <map>
#include "Macros.h"
#include "Sentence.h"
using std::list;
using std::vector;
using std::wstring;
using std::map;

template<typename t>
class List
{
	static CPool *s_pool;
	const static int s_growFac = 2;
private:
	t *  m_pAry;
	size_t  m_nCap;			//capacity
	size_t  m_nSize;

	void grow()
	{
		t * pOldAry = m_pAry;
		m_pAry = (t*)s_pool->Allocate(sizeof(t) * m_nCap * s_growFac);	
		memcpy(m_pAry,  pOldAry,  sizeof(t) * m_nCap);
		m_nCap *= s_growFac;
	}
	

public:
	typedef t* iterator;
	List()
	{
		m_nCap = 3;
		m_nSize = 0;
		m_pAry = (t*)s_pool->Allocate(sizeof(t) * m_nCap);
	}

	static List<t> * CreateList()
	{
		List<t> *pList = (List<t> *)s_pool->Allocate(sizeof(List<t>));
		new (pList)List<t>;
		return pList;
	}

	static List<t> * CopyList(const List<t> &r)
	{
		List<t> *pList = (List<t> *)s_pool->Allocate(sizeof(List<t>));
		pList->m_nCap  = r.m_nCap;
		pList->m_nSize = r.m_nSize;
		pList->m_pAry	 = (t*)s_pool->Allocate(sizeof(t) * pList->m_nCap);
		memcpy(pList->m_pAry,  r.m_pAry,  sizeof(t) * pList->m_nSize);
		return pList;
	}

	iterator begin() 										{return m_pAry;}
	iterator end()	 										{return m_pAry + m_nSize;}
	iterator rbegin()										{return m_pAry + m_nSize - 1;}
	iterator rend()	 										{return m_pAry - 1;}
	t & back()													{return m_pAry[m_nSize - 1];}
	size_t size() 											{return m_nSize;}
	bool empty()												{return m_nSize == 0;}
	t & operator [](int idx)						{return m_pAry[idx];}
	static void RecyclePool()						{s_pool->Recycle();}
	static void SetPool(CPool *pPool)		{s_pool = pPool;}

	void push_back(const t &Tgt)
	{
		if (m_nSize == m_nCap)
			grow();
		m_pAry[m_nSize ++] = Tgt;
	}
};


class CDepTree
{
public:
	const static int  ROOT_IDX;
	CDepTree(void)
	{
		m_pParent = NULL;
		m_pLCList = m_pRCList = NULL;
		m_pSen = NULL;
		m_dlID = m_idx = m_idxH = -1;
		m_puncID = -1;
	}

	~CDepTree(void);
	CDepTree (CDepTree * pTree);
	static CDepTree *CopyTree(CDepTree *pTree, CPool &rPool);
	static CDepTree *MakeTree(CPool &rPool);
	static CDepTree *BuildTree(vector<int> &vHIdx, 
														 SSentence *pSen,
														 CPool &rPool);
	
	void AddLeftChild(CDepTree * tree);
	void AddRightChild(CDepTree * tree);
	void PrintTree(FILE *fp, const vector<wstring *> *pLabelVec);
	void CollectTreeNodes(vector<CDepTree*> & refVec);
	void DisplayTreeStructure(FILE *fp, int depth = 0, 
											 		  const vector<wstring *> *pLabelVec = NULL,
											 		  int dir = 0,
														vector<bool> *pVWrong = NULL);
	
	int GetDepId()												{return m_dlID;}
	int PuncID()													{return m_puncID;}
	int PuncNum()													{return m_nSep;}
	int Index()														{return m_idx;}
	int HIndex()													{return m_idxH;}
	int GetLCNum()												{return m_pLCList == NULL ? 0 : m_pLCList->size();}
	int GetRCNum()												{return m_pRCList == NULL ? 0 : m_pRCList->size();}
	bool IsLeaf()													{return GetLCNum() + GetRCNum() == 0;}	
	List<CDepTree*> * GetLChildList()			{return m_pLCList;}
	List<CDepTree*> * GetRChildList()			{return m_pRCList;}
	
	CDepTree * GetPi()										{return m_pParent;}
	CDepTree * GetRC()										{return m_pRCList == NULL ? NULL : m_pRCList->back();}
	CDepTree * GetLC()										{return m_pLCList == NULL ? NULL : m_pLCList->back();}
	CDepTree * GetR2C()										{return GetRCNum() < 2 ? NULL: (*m_pRCList)[m_pRCList->size() - 2];}
	CDepTree * GetL2C()										{return GetLCNum() < 2 ? NULL: (*m_pLCList)[m_pLCList->size() - 2];}

	void SetPuncID(int ID)								{m_puncID = ID;}
	void SetPuncNum(int n)								{m_nSep = n;}
	void SetHIdx(int idxH)								{m_idxH = idxH;}
	void SetIndex(size_t pos)							{m_idx = pos;}
	void SetPi(CDepTree *tree)						{m_pParent = tree;}
	void SetDepId(int id)									{m_dlID = id;}
	void SetSen(_SENTENCE *pSen)					{m_pSen = pSen;}
	_SENTENCE * GetSen()									{return m_pSen;}


private:
	void collectTreeNodesHelper(vector<CDepTree *> &refVec);

private:
	enum PARENT_DIRECTION{LEFT, RIGHT}  m_pDir;

	CDepTree					*m_pParent;
	int								m_dlID;		//dependency label id
	int 							m_idx;
	int								m_idxH;
	int								m_puncID;
	int								m_nSep;
	List<CDepTree *>  *m_pLCList, *m_pRCList;
	SSentence         *m_pSen;

	bool							m_bEPunc;
};

