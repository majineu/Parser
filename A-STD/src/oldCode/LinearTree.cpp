/*******************************************************************************
 *
 *      Filename:  LinearTree.cpp
 *
 *       Version:  1.0
 *       Created:  2014-02-14 17:03:59
 *      Revision:  none
 *      Compiler:  g++
 *
 *        Author:  Ao Zhang (NLP-LAB), zhangao.cs@gmail.com
 *  Organization:  NEU, China
 *
 *   Description:
 *                 
 *
 ******************************************************************************/
CLTree::CLTree(SSentence *pSen)
{
	if (pSen == NULL)
	{
		fprintf(stderr, "Warning: initializing tree with empty sentnece\n");
		return ;
	}
	m_pSen   = pSen;
	m_pNodes = NULL;
	m_pNodes = new SDTreeNode *[pSen->Length()];
	
	for (int i = 0; i < pSen->Length(); ++i)
	{
		m_pNodes[i] = new SDTreeNode();
		m_pNodes[i]->m_idx = i;
		m_pNodes[i]->m_pNodes = m_pNodes;
	}
}


CLTree::CLTree(vector<int> &hIdxVec, SSentence *pSen)
{
	if (pSen == NULL || hIdxVec.size() == 0)
		return ;
	
	for (size_t i = 0; i < hIdxVec.size(); ++i)
		if (hIdxVec[i] > (int)hIdxVec.size() - 1)
		{
			fprintf(stderr, "Error: head index out of range\n");
			return ;
		}
	
	m_pSen 	= pSen;
	m_pNodes = new SDTreeNode *[pSen->Length()];
	for (int i = 0; i < pSen->Length(); ++i)
	{
		m_pNodes[i] = new SDTreeNode();
		m_pNodes[i]->m_idx = i;
		m_pNodes[i]->m_pNodes = m_pNodes;
	}

	for (int i = 1; i < (int)hIdxVec.size(); ++i)
		if (hIdxVec[i] != -1 && hIdxVec[i] < i)
			m_pNodes[hIdxVec[i]]->AddRightChild(m_pNodes[i]);

	for (int i = (int)hIdxVec.size() - 1; i !=  -1; --i)
		if (hIdxVec[i] == -1)
		{
			m_pRoot = m_pNodes[i];
			m_pRoot->m_iHead = _TNODE::ROOT_IDX;
		}
		else if (hIdxVec[i] > i)
			m_pNodes[hIdxVec[i]]->AddLeftChild(m_pNodes[i]);
}

// shallow copy
CLTree::CLTree(CLTree &r)
{
	m_pSen   = r.m_pSen;
	int len  = m_pSen->Length();
	m_pNodes = new SDTreeNode *[len];
	memcpy(m_pNodes, r.m_pNodes, sizeof(CLTree *) * len);
	for (int i = 0; i < len; ++i)
		m_pNodes[i]->m_pNodes = m_pNodes;
}


CLTree::~CLTree()
{
	assert(m_pNodes != NULL);
	for (int i = 0; i < m_pSen->Length(); ++i)
		delete m_pNodes[i];
	delete[] m_pNodes;
	m_pSen  = NULL;
	m_pRoot = NULL;
}


void CLTree::RemovePuncs()
{
	assert(m_pSen != NULL && m_pNodes != NULL);
	for (int i = 0; i < m_pSen->Length(); ++i)
		if (CConfig::IsPunc(m_pSen->Tag(i)) == true) 
			m_pNodes[m_pNodes[i]->m_iHead]->RemoveChild(i);
}

void CLTree::
RepairPunc()
{
	assert(m_pSen != NULL && m_pNodes != NULL);
	int last = -1;
	for (int i = 0; i < m_pSen->Length(); ++i)
	{
		if (CConfig::IsPunc(m_pSen->Tag(i)) && 
				m_pNodes[i]->m_iHead == _TNODE::s_IDX_NULL)
		{
			if (last != -1 && 
					(m_pNodes[last]->GetRightMostChild() == NULL||
					 m_pNodes[last]->GetRightMostChild()->Index() < i))
				m_pNodes[last]->AddRightChild(m_pNodes[i]);
		}
		else
			last = i;
	}
	
	last = -1;
	for (int i = m_pSen->Length() - 1; i >= 0; --i)
	{
		if (CConfig::IsPunc(m_pSen->Tag(i)) && 
				m_pNodes[i]->m_iHead == _TNODE::s_IDX_NULL)
		{
			if (last != -1 && 
					(m_pNodes[last]->GetLeftMostChild() == NULL||
					 m_pNodes[last]->GetLeftMostChild()->Index() > i))
				m_pNodes[last]->AddLeftChild(m_pNodes[i]);
		}
		else
			last = i;
	}
}


void SDTreeNode::
DisplayTreeStructure(FILE *fp, int depth, int dir, SSentence *pSen)
{
	SDTreeNode *pNode = GetLeftMostChild();
	while (pNode != NULL)
	{
		pNode->DisplayTreeStructure(fp, depth + 1, 1, pSen);
		pNode = pNode->GetRightSlibing();
	}

	string space(depth * 5, ' ');
	if (depth >0)
		dir == 1 ? (space += string("/--")):(space += string("\\--"));

	fprintf(fp, "%-3d%s[%s %s %d]\n", m_idx, space.c_str(),
					wstr2utf8(pSen->Word(m_idx)).c_str(), 
					wstr2utf8(pSen->Tag(m_idx)).c_str(), 
					m_iHead == ROOT_IDX ? 0: m_iHead);

	pNode = GetRightMostChild();
	vector<SDTreeNode *> rcs;		// right children
	while (pNode != NULL)
	{
		rcs.push_back(pNode);
		pNode = pNode->GetLeftSlibing();
	}

	for (vector<SDTreeNode *>::reverse_iterator iter = rcs.rbegin(); iter != rcs.rend(); ++iter)
		(*iter)->DisplayTreeStructure(fp, depth + 1, 2, pSen);
}


void CLTree::
PrintTree(FILE *fp)
{
	for (int i = 0; i < m_pSen->Length(); ++i)
		fprintf(fp, "%s\t%s\t%d\t NULL\n",
				wstr2utf8(m_pSen->Word(i)).c_str(), 
				wstr2utf8(m_pSen->Tag(i)).c_str(),
				m_pNodes[i]->m_iHead == _TNODE::ROOT_IDX ? 0:m_pNodes[i]->m_iHead + 1);

	fprintf(fp, "\n");
}
//------------------------------------------------------------

