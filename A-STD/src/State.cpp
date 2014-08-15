#include "State.h"
CState::
CState(CState *pState, CPool &rPool, bool cpEvent)
{
	m_bMaybeGold 	= pState->m_bMaybeGold;
	m_fScore 			= pState->m_fScore;
	m_idxQ 				= pState->m_idxQ;
	m_stackSize	  = pState->m_stackSize;

		/* copy accEvent */
	if (cpEvent && pState->pAccHead != NULL)
	{
		assert(pState->pAccTail != NULL);
		pAccHead = pState->pAccHead;
		pAccTail = pState->pAccTail;
	}
	else
		pAccTail = pAccHead = NULL;

	for (int i = 0; i < pState->m_stackSize; ++i)
		m_stack[i] = CDepTree::CopyTree(pState->m_stack[i], rPool);
}

void CState::
DisAction(FILE *fp)
{
	if(pAccHead == NULL)
		return; 
		
	SEvent *pit = pAccHead;
	while(pit != NULL)
	{
		fprintf(fp, "%s  ",
				pit->OID() == 0? "SFHIT":
				pit->OID() <= (int)CIDMap::GetDepLabelNum()? 
											"LEFT_REDUCE" : "RIGHT_REDUCE");
		pit = pit->m_pNext;
	}
	fprintf(fp,"\n");
}
	
bool CState::
IsGold(CState *pState, bool ignoreDLabel)
{	
		/* we only need to compare the last action */
	if (ignoreDLabel == true)
	{
		int label = 0;
		int a1 = CIDMap::Interprate(pAccHead->OID(), label);
		int a2 = CIDMap::Interprate(pState->pAccHead->OID(), label);
		bool equal = a1 == a2;

		if (equal == true)
			pState->pAccTail = pState->pAccHead;
		return equal;
	}

	bool equal =  pAccHead->OID() == pState->pAccHead->OID() ;
	if (equal == true)
		pState->pAccTail = pState->pAccHead;
	return equal;
}

	
void CState::
PrintState(vector<wstring *> *pDLabelVec, FILE *fp)
{
	fprintf(fp, "################state info##############\n");
	fprintf(fp, "stackSize %d, qIdx %d, score %lf\n",
					m_stackSize, m_idxQ, m_fScore);
	for (int i = 0; i < m_stackSize; ++i)
	{
		fprintf(fp, "s%d:\n", m_stackSize - i - 1);
		m_stack[i]->DisplayTreeStructure(fp, 0, &CIDMap::GetDepLabelVec());
		fprintf(fp,"\n");
	}

	DisAction(fp);
	fprintf(fp, "#######################################\n\n");
}

#if 0
void CState::
Append(SSentence *pSen, CPool &rPool)
{
	if (m_idxQ >= pSen->Length())
		return;

	int TID = pSen->TID(m_idxQ);
	while (m_idxQ < pSen->Length() && SSenNode::IsPunc(TID))
	{
		if (SSenNode::IsBPunc(TID))
		{
			while (m_idxQ < pSen->Length() && 
						 SSentence::IsPunc(pSen->TID(m_idxQ)))
				m_idxQ ++;
			if (m_idxQ < pSen->Length())
			{
				Shift(rPool, pSen);
				m_stack[m_stackSize]->AddPunc(TID);
			}
		}
		else
			m_stack[m_stackSize]->AddPunc(TID); 
	}
}
#endif

