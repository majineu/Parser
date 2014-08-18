#include "DepTree.h"
#include "Macros.h"

template<typename T> CPool * List<T>::s_pool = NULL;
const int CDepTree::ROOT_IDX  = -5;
CDepTree::~CDepTree(void)
{
	m_pParent = NULL;
	m_pSen = NULL;
	m_pRCList = m_pLCList = NULL;
	m_dlID = m_idx = m_idxH = -1;
	m_puncID = SSenNode::s_iPuncNul;
}

CDepTree * CDepTree::
MakeTree(CPool &rPool)
{
	CDepTree *pNew = (CDepTree *)rPool.Allocate(sizeof(CDepTree));
	new (pNew)CDepTree();
	return pNew;
}

CDepTree * CDepTree::
CopyTree(CDepTree *pTree, CPool &rPool)
{
	CDepTree *pNew 	= (CDepTree*)rPool.Allocate(sizeof(CDepTree));
	pNew->m_idx 		= pTree->m_idx;
	pNew->m_dlID 		= pTree->m_dlID;
	pNew->m_idxH		= pTree->m_idxH;
	pNew->m_pParent = NULL;
	pNew->m_pLCList = pNew->m_pRCList = NULL;
	pNew->m_pSen    = pTree->m_pSen;
	pNew->m_puncID  = pTree->m_puncID;
	pNew->m_nSep	  = pTree->m_nSep;

	// shallow copy child
	if (pTree->m_pLCList != NULL)
		pNew->m_pLCList = List<CDepTree *>::CopyList(*pTree->m_pLCList);
	
	if (pTree->m_pRCList != NULL)
		pNew->m_pRCList = List<CDepTree *>::CopyList(*pTree->m_pRCList);
	return pNew;
}



void CDepTree::AddLeftChild(CDepTree * pTree)
{
	assert(pTree != NULL);
	if (m_pLCList == NULL)
		m_pLCList = List<CDepTree*>::CreateList();//new list<CDepTree *>();
	m_pLCList->push_back(pTree);
	pTree->m_pParent = this;
	pTree->m_pDir = RIGHT;
	pTree->m_idxH = m_idx;
}


void CDepTree::AddRightChild(CDepTree * pTree)
{
	assert(pTree != NULL);
	if (m_pRCList == NULL)
		m_pRCList = List<CDepTree*>::CreateList();//new list<CDepTree *>();
	m_pRCList->push_back(pTree);
	pTree->m_pParent = this;
	pTree->m_pDir = LEFT;
	pTree->m_idxH = m_idx;
}


void CDepTree::
PrintTree(FILE * fp,  const vector<wstring *> *pLabelVec)
{
	if (m_pLCList != NULL)
	{
		List<CDepTree*> ::iterator it = m_pLCList->rbegin(), end = m_pLCList->rend();
		for(; it > end; -- it)
			(*it)->PrintTree(fp, pLabelVec);
	}

	fprintf(fp, "%s\t", wstr2utf8(m_pSen->Word(m_idx)).c_str());
	fprintf(fp, "%s\t%d\t",wstr2utf8(m_pSen->Tag(m_idx)).c_str(), 
					m_idxH == ROOT_IDX ? 0: m_idxH + 1);
		
	if (m_dlID == -1 || pLabelVec == NULL)
		fprintf(fp, "%s\n", "NULL");
	else
		fprintf(fp, "%s\n", wstr2utf8(*(*pLabelVec)[m_dlID]).c_str());
	

	if (m_pRCList != NULL)
	{
		List<CDepTree*> ::iterator it = m_pRCList->begin(), end = m_pRCList->end();
		for (; it != end; ++it)
			(*it)->PrintTree(fp, pLabelVec);
	}
}


CDepTree *CDepTree::
BuildTree(vector<int> &hIdxVec, SSentence *pSen, CPool & rPool)
{
	if (pSen == NULL || hIdxVec.size() == 0)
		return NULL;

	// check valid head index
	for (size_t i = 0; i < hIdxVec.size(); ++i)
		if (hIdxVec[i] > (int)hIdxVec.size() - 1)
			fprintf(stderr, "Error: head index [%lu]:%d out of range\n",
					i, hIdxVec[i]);

	vector<CDepTree *> treeVec;
	int rootOft = -1;
	for (size_t i = 0; i < hIdxVec.size(); ++i)
	{
		CDepTree *pTree = CDepTree::MakeTree(rPool);
		treeVec.push_back(pTree);
		
		pTree->m_idx 	= i;
		pTree->m_idxH = hIdxVec[i];
		pTree->m_pSen = pSen;
		pTree->m_dlID = CIDMap::GetDepLabelId(pSen->Label(i));
		pTree->m_puncID = pSen->PuncID(i);
		pTree->m_nSep	= pSen->SepNum(i);
		if (hIdxVec[i] == -1)
		{
			rootOft = i;
			pTree->m_idxH = ROOT_IDX;
		}
	}

	// construct tree structure
	for (int i = 1; i < (int)hIdxVec.size(); ++i)
		if (hIdxVec[i] != -1 && hIdxVec[i] < i)
			treeVec[hIdxVec[i]]->AddRightChild(treeVec[i]);
	
	for (int i = (int)hIdxVec.size() - 1; i !=  -1; --i)
		if (hIdxVec[i] != -1 && hIdxVec[i] > i)
			treeVec[hIdxVec[i]]->AddLeftChild(treeVec[i]);
	
	return	treeVec[rootOft]; 
}

void CDepTree::
DisplayTreeStructure(FILE *fp, int depth, 
										 const vector<wstring *> *pLabelVec,
										 int dir,	vector<bool> * pVWrong)
{
	if (m_pLCList != NULL)
	{
		List<CDepTree*> ::iterator it = m_pLCList->rbegin(), end = m_pLCList->rend();
		for(; it != end; -- it)
			(*it)->DisplayTreeStructure(fp, depth + 1, pLabelVec, 
																	1, pVWrong);
	}

	string space("");
	space.assign(depth * 5, ' ');
	if (depth >0)
		space += dir == 1 ? "/--":"\\--";

	fprintf(fp, "%-2d:%s[%s %s %s %d ", m_idx, space.c_str(),
					wstr2utf8(m_pSen->Word(m_idx)).c_str(), 
					wstr2utf8(m_pSen->Tag(m_idx)).c_str(),
					m_puncID == -1 ? "" : wstr2utf8(SSenNode::GetKey(m_puncID)).c_str(),
					m_idxH == ROOT_IDX ? 0: m_idxH + 1);
	if (pVWrong != NULL && (*pVWrong)[m_idx] == true)
		fprintf(fp, ":::::::::");
	
  if (m_dlID == -1 || pLabelVec == NULL)
		fprintf(fp, "%s]\n", "NULL");
	else
		fprintf(fp, "%s]\n", wstr2utf8((*pLabelVec)[m_dlID]->c_str()).c_str());

	if (m_pRCList != NULL)
	{
		List<CDepTree*> ::iterator it = m_pRCList->begin(), end = m_pRCList->end();
		for (; it != end; ++it)
			(*it)->DisplayTreeStructure(fp, depth + 1, pLabelVec, 
																	2, pVWrong);
	}
}

void CDepTree::
collectTreeNodesHelper(vector<CDepTree*> &refVec)
{
	List<CDepTree*> ::iterator it, end;
	if(m_pLCList != NULL)
	{
		it = m_pLCList->rbegin(), end = m_pLCList->rend();
		for(; it != end; -- it)
			(*it)->collectTreeNodesHelper(refVec);
	}

	refVec.push_back(this);
	if(m_pRCList != NULL)
	{
		it = m_pRCList->begin(), end = m_pRCList->end();
		for(; it != end; ++it)
			(*it)->collectTreeNodesHelper(refVec);
	}
}

void CDepTree::
CollectTreeNodes(vector<CDepTree* > & refVec)
{
	refVec.clear();   // 2012-05-24
	collectTreeNodesHelper(refVec); // 2012-05-24
}

