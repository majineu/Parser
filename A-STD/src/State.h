#ifndef __STATE_H__
#define __STATE_H__
#include "DepTree.h"
#include "SparseScorer.h"


#define MAX_STACK_LEN 256
class CState
{
public:
	CState (CState *pState, CPool &rPool, bool cpEvent);
	CState()																								{memset(this, 0, sizeof(*this));}
	~CState()																								{memset(this, 0, sizeof(*this));}
	
	
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


  void MatchBEPunc(CDepTree * p0, CDepTree *p1, CDepTree * pOut) {
		int nSep1 = p1->PuncNum();
    int id1 = p1->PuncID();
    int nSep0 = p0->PuncNum();
    int id0 = p0->PuncID();

    if ((SSenNode::IsBSep(id0) && SSenNode::IsBSep(id1)) ||
        (SSenNode::IsESep(id0) && SSenNode::IsESep(id1))) {
      pOut->SetPuncNum(nSep1 + nSep0);
      pOut->SetPuncID(pOut == p0 ? id1 : id0);
    } else {
      if (nSep1 == nSep0) {
			  pOut->SetPuncNum(0);
        pOut->SetPuncID(SSenNode::EMap(id0));
      } else {
  	    pOut->SetPuncNum(abs(nSep1 - nSep0));
        pOut->SetPuncID(nSep1 > nSep0 ? id1 : id0);
      }
    }
  }

	void ReduceLeft(int depLabelId) {
		if (m_stackSize < 2) {
			fprintf(stderr, "Error: reduce not allowed\n");
			exit(0);
		}
		CDepTree *p0 = m_stack[--m_stackSize];
		CDepTree *p1 = m_stack[--m_stackSize];
		
		p0->AddLeftChild(p1);		//note here the new tree should be inserted at the head
		p1->SetDepId(depLabelId);
		m_stack[m_stackSize ++] = p0;

    //	Processing punctuation properties.
		int nSep1 = p1->PuncNum();
    int id1 = p1->PuncID(), id0 = p0->PuncID();
		if (SSenNode::IsBSep(id1)) {
      if (SSenNode::IsBSep(id0) || SSenNode::IsESep(id0)) {
        MatchBEPunc(p0, p1, p0);
			} else {
				// Rewrite the original punc property in s0.
				p0->SetPuncNum(nSep1);
				p0->SetPuncID(id1);
			}
		} else if (!SSenNode::IsNonBEPunc(id0) && !SSenNode::IsESep(id0)) {
      // When s1 does not take any BPunc, and s0 does not contain
      // nonBEPunc nor EPunc, then just clear s0's punc property.
      p0->SetPuncNum(0);
      p0->SetPuncID(SSenNode::s_iPuncNul);
    }
	}


	void ReduceRight(int depLabelId) {
		if (m_stackSize < 2) {
			fprintf(stderr, "Error: reduce not allowed\n");
			exit(0);
		}
		CDepTree *p0 = m_stack[--m_stackSize];
		CDepTree *p1 = m_stack[--m_stackSize];

		p1->AddRightChild(p0);
		p0->SetDepId(depLabelId);
		m_stack[m_stackSize ++] = p1;
		
		int nSep1 = p1->PuncNum();
		int nSep0 = p0->PuncNum();
		
		if (SSenNode::IsESep(p0->PuncID())) {
			if (SSenNode::IsESep(p1->PuncID())) {
				p1->SetPuncNum(nSep1 + nSep0);
        // using the right most punc property.
        p1->SetPuncID(p0->PuncID());
      } else if (SSenNode::IsBSep(p1->PuncID())) {
				if (nSep1 < nSep0) {
					p1->SetPuncID(p0->PuncID());
					p1->SetPuncNum(nSep0 - nSep1);
				} else if (nSep1 > nSep0) {
					p1->SetPuncNum(nSep1 - nSep0);
        } else {
					p1->SetPuncNum(0);		
					p1->SetPuncID(SSenNode::BMap(p1->PuncID()));
				}
			} else {
				// When p1 contains no punc, paired-punc or nonBEPunc,
        // then rewrite with epunc from s0.
				p1->SetPuncID(p0->PuncID());
				p1->SetPuncNum(nSep0);
			}
		} else if (SSenNode::IsNonBEPunc(p0->PuncID())) {
      // nonBEPunc are also propagated from right to left.
			if (!SSenNode::IsBSep(p1->PuncID())) {
        // Only BPunc is preserved, others are overwrite.
        p1->SetPuncNum(0);
				p1->SetPuncID(p0->PuncID());
      }
		} else {
      // When p0 contains BPunc or no punc at all, we only need to
      // clear p1's punc properties.
      // The only exception is when s1 contains a BPunc, then that
      // BPunc will be reserved.
      if (!SSenNode::IsBSep(p1->PuncID())) {
        p1->SetPuncNum(0);
        p1->SetPuncID(SSenNode::s_iPuncNul);
      }
    }
	}

	void GetTopThreeElements (CDepTree ** ppTree0) {
		memset(ppTree0, 0, sizeof(CDepTree*) * 3);
		for (int i = 0; m_stackSize - i - 1 >=0 && i < 3; ++i)
			ppTree0[i] = m_stack[m_stackSize - i - 1];
	}

private:
	CDepTree* m_stack[MAX_STACK_LEN];
	int 			m_stackSize;
	int 			m_idxQ;
	bool 			m_bMaybeGold;
	double 		m_fScore;
	SEvent 		*pAccHead;
	SEvent		*pAccTail;
};

#endif  /*__STATE_H__*/
