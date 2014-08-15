#ifndef __STATE_H__
#define __STATE_H__
#define _MAX_STACK_LEN 256
//#include "LinearTree.h"
#include "DepTree.h"
#include "SparseScorer.h"
//#include "Scorer.h"
class CState
{
public:
	static CState * CopyState(CState *pState, CPool &rPool, bool cpEvent)
	{
		CState *pNew = (CState *)rPool.Allocate(sizeof(CState));
		new (pNew)CState(pState, rPool, cpEvent);
		return pNew;
	}

	static CState * MakeState(CPool &rPool)
	{
		CState *pNew = (CState *)rPool.Allocate(sizeof(CState));
		new (pNew)CState();
		return pNew;
	}
	CState (CState *pState, CPool &rPool, bool cpEvent);
	CState()																								{memset(this, 0, sizeof(*this));}
	~CState()																								{memset(this, 0, sizeof(*this));}

	void DisAction(FILE *fp = stderr);	
	void PrintState(vector<wstring *> *pVLabels = NULL,
									FILE *fp = stderr);
	
	
	SEvent * GetAccEvent()																	{return pAccHead;}
	SEvent * GetLastCorrEvent()															{return pAccTail;}
	SEvent ** GetAccEventPtr()															{return &pAccHead;}
	CDepTree * GetTopStack()																{return m_stack[m_stackSize - 1];}
	int GetQIndex()																					{return m_idxQ;}
	int	GetStackLen()																				{return m_stackSize;}
	double GetScore()																				{return m_fScore;}
	double IncreaseScore(double s)													{return m_fScore += s;}
	bool StackEmpty()																				{return m_stackSize == 0;}
	bool IsPossible()																				{return m_bMaybeGold;}
	bool operator() (const CState* l, const CState *r)			{return l->m_fScore > r->m_fScore;}
	void SetPossible(bool possible)													{m_bMaybeGold = possible;}
	void ClearStack()																				{m_stackSize = 0, memset(m_stack, 0, sizeof(m_stack));}
	void AddAccEvent(SEvent *pEvent)												{pAccTail = pAccHead = pEvent;}
	bool IsGold(CState *pState, bool ignoreDLabel = false);
	

	void Shift (CPool &rPool, SSentence *pSen)
	{
		CDepTree * pTree = CDepTree::MakeTree(rPool); 
		pTree->SetIndex(m_idxQ);
		pTree->SetSen(pSen);
		pTree->SetPuncID(pSen->PuncID(m_idxQ));
		pTree->SetPuncNum(pSen->SepNum(m_idxQ++));
		m_stack[m_stackSize ++] = pTree;
	}

	void ReduceLeft(int depLabelId)
	{
		if (m_stackSize < 2)
			throw("Error: reduce not allowed\n");
		
		CDepTree *p0 = m_stack[--m_stackSize];
		CDepTree *p1 = m_stack[--m_stackSize];
		
		p0->AddLeftChild(p1);		//note here the new tree should be inserted at the head
		p1->SetDepId(depLabelId);
		m_stack[m_stackSize ++] = p0;

		int nSep1 = p1->PuncNum();
		int nSep0 = p0->PuncNum();
		
		if (SSenNode::IsBSep(p1->PuncID()))
		{
			if (SSenNode::IsBSep(p0->PuncID()))
				p0->SetPuncNum(nSep1 + nSep0);
			else if (SSenNode::IsESep(p0->PuncID()))
			{
				if (nSep1 > nSep0)
				{
					p0->SetPuncID(p1->PuncID());
					p0->SetPuncNum(nSep1 - nSep0);
				}
				else if (nSep1 < nSep0)
					p0->SetPuncNum(nSep0 - nSep1);
				else
				{
					p0->SetPuncNum(0);		// actually, separator num
					p0->SetPuncID(SSenNode::EMap(p0->PuncID()));
				}
			}
			else
			{
				// just transfer directly
				p0->SetPuncID(p1->PuncID());
				p0->SetPuncNum(nSep1);
			}
		}
#if 0
		int puncMapID = SSenNode::BMap(p1->PuncID());
		if (puncMapID != -1)
		{
			if (puncMapID == SSenNode::EMap(p0->PuncID()))
				p0->SetPuncID(puncMapID);
			else
				p0->SetPuncID(p1->PuncID());
		}
#endif
	}


	void ReduceRight(int depLabelId)
	{
		if (m_stackSize < 2)
			throw("Error: reduce not allowed\n");
		CDepTree *p0 = m_stack[--m_stackSize];
		CDepTree *p1 = m_stack[--m_stackSize];

		p1->AddRightChild(p0);
		p0->SetDepId(depLabelId);
		m_stack[m_stackSize ++] = p1;
		
		int nSep1 = p1->PuncNum();
		int nSep0 = p0->PuncNum();
		
		if (SSenNode::IsESep(p0->PuncID()))
		{
			if (SSenNode::IsESep(p1->PuncID()))
				p1->SetPuncNum(nSep1 + nSep0);
			else if (SSenNode::IsBSep(p1->PuncID()))
			{
				if (nSep1 < nSep0)
				{
					p1->SetPuncID(p0->PuncID());
					p1->SetPuncNum(nSep0 - nSep1);
				}
				else if (nSep1 > nSep0)
					p1->SetPuncNum(nSep1 - nSep0);
				else
				{
					p1->SetPuncNum(0);		// actually, separator num
					p1->SetPuncID(SSenNode::BMap(p1->PuncID()));
				}
			}
			else
			{
				// p1 contains no punct, just transfer directly
				p1->SetPuncID(p0->PuncID());
				p1->SetPuncNum(nSep0);
			}
		}
		else if (SSenNode::IsPunc(p0->PuncID()))
		{
			if (nSep1 == 0)
				p1->SetPuncID(p0->PuncID());
		}
		
#if 0
		int puncMapID = SSenNode::BMap(p1->PuncID());
		if (puncMapID != -1 && puncMapID == SSenNode::EMap(p0->PuncID()))
			p1->SetPuncID(puncMapID);
		else if (puncMapID == -1)	// either no punc attached or the punc is not bPunc
			p1->SetPuncID(p0->PuncID());
#endif
	}

	void GetTopThreeElements (CDepTree ** ppTree0)
	{
		memset(ppTree0, 0, sizeof(CDepTree*) * 3);
		for (int i = 0; m_stackSize - i - 1 >=0 && i < 3; ++i)
			ppTree0[i] = m_stack[m_stackSize - i - 1];
	}

private:
	CDepTree* m_stack[128];
	int 			m_stackSize;
	int 			m_idxQ;
	bool 			m_bMaybeGold;
	double 		m_fScore;
	SEvent 		*pAccHead;
	SEvent		*pAccTail;
};

#endif  /*__STATE_H__*/
