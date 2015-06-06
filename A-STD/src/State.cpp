#include "State.h"
CState::
CState(CState *pState, CPool &rPool, bool cpEvent) {
	m_bMaybeGold 	= pState->m_bMaybeGold;
	m_fScore 			= pState->m_fScore;
	m_idxQ 				= pState->m_idxQ;
	m_stackSize	  = pState->m_stackSize;

		/* copy accEvent */
	if (cpEvent && pState->pAccHead != NULL) {
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
DisAction(FILE *fp) const {
	if(pAccHead == NULL)
		return; 
		
	SEvent *pit = pAccHead;
	while(pit != NULL) {
		fprintf(fp, "%s  ",
				pit->OID() == 0 ? "SFHIT":
				pit->OID() <= (int)CIDMap::GetDepLabelNum()? 
											"LEFT_REDUCE" : "RIGHT_REDUCE");
		pit = pit->m_pNext;
	}
	fprintf(fp,"\n");
}
	

bool CState::
IsGold(CState *pState, bool ignoreDLabel) {	
	/* we only need to compare the last action */
	if (ignoreDLabel == true) {
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

#if 0
int CState::StackCosts(const vector<CDepTree *> & gold_nodes, 
                        vector<int> *costs) const {
  if (costs == nullptr) {
    fprintf(stderr, "Error: null pointer: costs\n");
    exit(0);
  }
  costs->clear();
  costs->resize(m_stackSize, 0);
  vector<CDepTree *> nodes;

  int cost_sum = 0;
  for (size_t i = 0; i < m_stackSize; ++i) {
    // Cost of each element on the stack, from top to bottum.
    const CDepTree * tree = m_stackSize- 1 - i;
    tree->CollectTreeNodes(nodes);
    int tree_cost = 0;

    // Compute cost of this element.
    for (const CDepTree * node: nodes) {
      if (node->HIndex() != -1)
        tree_cost += gold_nodes[node->Index()]->HIndex() == node->HIndex(); 
    }
    (*costs)[i] = tree_cost;
    cost_sum += tree_cost;
  }
  return cost_sum;
}

void CState::BuildStacks(const vector<CDepTree *> &nodes,
                         CIDMap::ACTION_TYPE action,
                         vector<int> *left_stack,
                         vector<int> *right_stack) const {
  assert(left_stack);
  assert(right_stack);
  left_stack->resize(action == SHIFT ? (m_stackSize + 1) : (m_stackSize - 1));
  right_stack->clear();
  int idx_q_new = m_idxQ;

  // Build left stack.
  if (action == SHIFT) {
    (*left_stack)[0] = idx_q_new ++;
    for (int i = 0; i < m_stackSize; ++i)
      (*left_stack)[i + 1] = m_stackSize - i - 1;
  } else {
    for (int i = 0, k = 0; i < m_stackSize; ++i) {
      if (i == 0 && action == CIDMap::RIGHT_REDUCE) continue;
      if (i == 1 && action == CIDMap::LEFT_REDUCE) continue;
      (*left_stack)[k++] = m_stackSize - i - 1;
    }
  }

  // Build right stack.
  right_stack.push_back(left_stack[0]); 
  for (size_t k = idx_q_new; k < nodes.size(); ++i) {
    // Note, for shift, the condition is "<="
    if (nodes[k]->HIndex() < idx_q_new) {
      right_stack->push_back(k);
    } else if (nodes[k]->GetLC() != NULL && 
               nodes[k]->GetLC()->Index() < idx_q_new) {
      right_stack->push_back(k);
    } else {
      // Check if any dependents of k is already in right_stack.
      for (auto idx: *right_stack) {
        if (nodes[idx]->HIndex() == k) {
          right_stack->push_back(k);
          break;
        }
      }
    }
  }
}

void CState::UpdateCost(const vector<CDepTree *> &gold_nodes,
                        const CIDMap::ACTION_TYPE action,
                        const vector<int> & costs,
                        const int old_cost_sum,
                        int * cost_sum,
                        int * top_stack_cost) const {
  assert(cost_sum);
  assert(top_stack_cost);
  *cost_sum = old_cost_sum;
  if (action == CIDMap::SHIFT) {
    // Total cost is not changed.
    *top_stack_cost = 0;
    return ;
  }

  assert(m_stackSize >= 2);
  int idx0 = m_stack[m_stackSize - 1]->Index();
  int idx1 = m_stack[m_stackSize - 2]->Index();
    
  if (action == CIDMap::LEFT_REDUCE) {
    *top_stack_cost = costs[0];
    if (gold_nodes[idx1]->HIndex() != idx0) {
      *cost_sum += 1 ;
      *top_stack_cost += 1;
    }
  } else {
    *top_stack_cost = costs[1];
    if (gold_nodes[idx0]->HIndex() != idx1) {
      *cost_sum += 1;
      *top_stack_cost +=1;
    }
  }
}

#endif
void CState::
PrintState(vector<wstring *> *pDLabelVec, FILE *fp) const {
	fprintf(fp, "################state info##############\n");
	fprintf(fp, "stackSize %d, qIdx %d, score %lf\n",
					m_stackSize, m_idxQ, m_fScore);
	for (int i = 0; i < m_stackSize; ++i) {
		fprintf(fp, "s%d:\n", m_stackSize - i - 1);
		m_stack[i]->DisplayTreeStructure(fp, 0, &CIDMap::GetDepLabelVec());
		fprintf(fp,"\n");
	}

	DisAction(fp);
	fprintf(fp, "#######################################\n\n");
}
