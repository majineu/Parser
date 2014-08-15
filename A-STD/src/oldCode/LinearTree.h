#ifndef __LINEAR_TREE_H__
#define __LINEAR_TREE_H__

struct SDTreeNode
{
	int 	m_idx;
	int 	m_iHead;
	int 	m_iLChild, 		m_iRChild;
	int 	m_iLSlibing,	m_iRSlibing;
	int   m_nLChild, 		m_nRChild;
	SDTreeNode ** m_pNodes;
	const static int ROOT_IDX = -2;
	static int s_IDX_NULL;
	
	void DisplayTreeStructure(FILE *fp, int depth, int dir, SSentence *pSen);
	void InitNull()
	{
		m_pNodes = NULL;
		m_idx = m_iHead = m_iLChild = m_iRChild = m_iLSlibing = m_iRSlibing = s_IDX_NULL; 
		m_nLChild = m_nRChild = 0;
	}

	SDTreeNode()														{InitNull();}
	SDTreeNode(int idx, int hidx)
	{
		InitNull();
		m_idx = idx;
		m_iHead = hidx;
	}

	SDTreeNode * GetLeftMostChild()
	{
		assert(m_pNodes != NULL);
		if (m_iLChild == s_IDX_NULL)
			return NULL;
		return m_pNodes[m_iLChild];
	}

	SDTreeNode * GetRightMostChild()
	{	
		assert(m_pNodes != NULL);
		if (m_iRChild == s_IDX_NULL)
			return NULL;
		return m_pNodes[m_iRChild];
	}

	SDTreeNode * GetL2Child()
	{
		if (m_iLChild != s_IDX_NULL)
			return GetLeftMostChild()->GetRightSlibing();
		return NULL;
	}

	SDTreeNode * GetR2Child()
	{
		if (m_iRChild != s_IDX_NULL)
			return GetRightMostChild()->GetLeftSlibing();
		return NULL;
	}

	SDTreeNode * GetRightSlibing()
	{
		assert(m_pNodes != NULL);
		if (m_iRSlibing == s_IDX_NULL)
			return NULL;
		return m_pNodes[m_iRSlibing]; 
	}
	
	SDTreeNode * GetLeftSlibing()
	{
		assert(m_pNodes != NULL);
		if (m_iLSlibing == s_IDX_NULL)
			return NULL;
		return m_pNodes[m_iLSlibing]; 
	}

	int GetNodeNum()
	{
		int left = m_idx, right = m_idx;
		SDTreeNode *pNode = GetLeftMostChild();
		while (pNode != NULL)
		{
			left = pNode->Index();
			pNode = pNode->GetLeftMostChild();
		}

		pNode = GetRightMostChild();
		while (pNode != NULL)
		{
			right = pNode->Index();
			pNode = pNode->GetRightMostChild();
		}
		return right - left + 1;
	}

	void RemoveChild(int cIdx)		// child index
	{
		assert(m_pNodes != NULL);
		SDTreeNode *pChild  = m_pNodes[cIdx];
		pChild->m_iHead = s_IDX_NULL;
	
		cIdx < m_idx ? (m_nLChild --):(m_nRChild --);
		if (m_iLChild == cIdx)
			m_iLChild = pChild->m_iRSlibing;
		else if (m_iRChild == cIdx)
			m_iRChild = pChild->m_iLSlibing;

		if (pChild->m_iLSlibing != s_IDX_NULL)
			m_pNodes[pChild->m_iLSlibing]->m_iRSlibing = pChild->m_iRSlibing;
		if (pChild->m_iRSlibing != s_IDX_NULL)
			m_pNodes[pChild->m_iRSlibing]->m_iLSlibing = pChild->m_iLSlibing;

		pChild->m_iLSlibing = pChild->m_iRSlibing = s_IDX_NULL;
	}

	void AddLeftChild(SDTreeNode *pNode)
	{
		m_nLChild+= 1;
		pNode->m_iHead = m_idx;
		pNode->m_iRSlibing = m_iLChild;
		if (m_iLChild != s_IDX_NULL)
			m_pNodes[m_iLChild]->m_iLSlibing = pNode->m_idx;
		m_iLChild = pNode->m_idx;
	}

	void AddRightChild(SDTreeNode *pNode)
	{
		m_nRChild += 1;
		pNode->m_iHead = m_idx;
		pNode->m_iLSlibing = m_iRChild;
		if (m_iRChild != -1)
			m_pNodes[m_iRChild]->m_iRSlibing = pNode->m_idx;
		m_iRChild = pNode->m_idx;
	}

	bool IsLeaf()									{return m_iLChild == m_iRChild && m_iLChild == s_IDX_NULL; }
	int HIndex()									{return m_iHead;}
	int Index()										{return m_idx;}
	int GetLeftChdNum()						{return m_nLChild;}
	int GetRightChdNum()					{return m_nRChild;}
};

typedef SDTreeNode _TNODE;

struct CLTree				// nodes are stored in a array.
{
	_TNODE 			**m_pNodes;
	_TNODE			*m_pRoot;
	SSentence 	*m_pSen;	

	CLTree(vector<int> &hIdxVec, SSentence *pSen);
	CLTree(SSentence *pSen);
	CLTree(CLTree &r);								// node are shallow copied
	~CLTree();
	void PrintTree(FILE *fp);
	void RepairPunc();
	void RemovePuncs();
	
	_TNODE * GetNode(int idx)											{return m_pNodes[idx];}
	void DisplayTreeStructure(FILE *fp)						{m_pRoot->DisplayTreeStructure(fp, 0, 0, m_pSen);}
};

//typedef CLTree _TREE;


#endif  /*__LINEAR_TREE_H__*/
