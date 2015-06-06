#include "DynamicOracle.h"

using std::max;
using std::min;


// Get the highest score action from oracles.
// When REDUCE action builds a gold arc, we choose the gold dep label.
// Otherwise, we select the label with the highest model score.
int CDynamicOracleSearcher::
BestAction(const set<CIDMap::ACTION_TYPE> &actions, 
           const vector<double> &scores) {
  if (actions.size() == 0) {
    fprintf(stderr, "Error: empty action set.\n");
    exit(-1);
  }
  int bestID = -1;
  for (auto action: actions) {
    if (action == CIDMap::SHIFT) {
      if (bestID == -1 || scores[bestID] < scores[CIDMap::SHIFT]) 
        bestID = CIDMap::SHIFT;
    } else {
      const CDepTree * s0 = nodes_[state_.GetTopStack()->Index()];
      const CDepTree * s1 = nodes_[state_.GetStack1()->Index()];
      
      const CDepTree * head = s0;
      const CDepTree * modifier = s1;
      if (action == CIDMap::RIGHT_REDUCE) 
        std::swap(head, modifier);
      
      // When REDUCE action builds a gold arc, we choose the gold dep label.
      // Otherwise, we select the label with the highest model score.
      if (modifier->HIndex() == head->Index()) {
        int ID = CIDMap::GetOutcomeId(action, modifier->GetDepId());
        if (bestID == -1 || scores[bestID] < scores[ID])
          bestID = ID;
      } else {
        for (int label = 0; label < CIDMap::GetDepLabelNum(); ++label) {
          int ID = CIDMap::GetOutcomeId(action, label);
          if (bestID == -1 || scores[bestID] < scores[ID])
            bestID = ID;
        }
      }
    }
  }
  return bestID;
}

// Return the loss of oracle action.
int CDynamicOracleSearcher::
//Oracle(const int loss_last_step, set<CIDMap::ACTION_TYPE> *actions) {
Oracle(set<CIDMap::ACTION_TYPE> *actions) {
  assert(actions);
  actions->clear();
  int min_loss = INT_MAX;
  int loss_shift = INT_MAX;
  int loss_reduce_left = INT_MAX;
  int loss_reduce_right = INT_MAX;
  
  if (state_.AllowShift(nodes_.size())) {
    loss_shift = ComputeLoss(CIDMap::SHIFT);
    min_loss = min(min_loss, loss_shift);
  }

  if (state_.AllowReduce()) {
    loss_reduce_left = ComputeLoss(CIDMap::LEFT_REDUCE);
    min_loss = min(min_loss, loss_reduce_left);

    loss_reduce_right = ComputeLoss(CIDMap::RIGHT_REDUCE);
    min_loss = min(min_loss, loss_reduce_right);
  }

  if (min_loss == loss_shift) actions->insert(CIDMap::SHIFT);
  if (min_loss == loss_reduce_right) actions->insert(CIDMap::RIGHT_REDUCE);
  if (min_loss == loss_reduce_left) actions->insert(CIDMap::LEFT_REDUCE);
 
//  if (min_loss != loss_last_step) {
//    fprintf(stderr, "Error: min_loss %d, loss_last_step %d\n",
//        min_loss, loss_last_step);
//    exit(0);
//  }
  assert(actions->size() > 0);
  return min_loss;
}


int CDynamicOracleSearcher::
LossLinearTree() {
  vector<vector<int *>> T(L_.size(), vector<int *>(R_.size(), nullptr));
  // We use offset in the 3rd demonsion inside of real head index.
  int num_heads = L_.size() + R_.size() - 1;
  vector<int> scoreBuf(L_.size() * R_.size() * num_heads, nodes_.size());
  for (size_t i = 0; i < L_.size(); ++i)
    for (size_t k = 0; k < R_.size(); ++k)
      T[i][k] = &scoreBuf[0] + (i * R_.size() + k) * num_heads;


//  T[0][0][0] = top_stack_cost_;
  T[0][0][0] = 0;
  // There are two offets conversions:
  // 1. h_offset -> offsets in L/R stacks
  //    0 ~ i correponds to nodes in L[0] ~ L[i]
  //    i + 1 to i + j correponds to nodes in R[1] ~ R[j]
  // 2. h_offset -> offsets in table T
  //    0 ~ L.size() - 1 correpondes to head in L[0] ~ L[L.size() - 1]
  //    L.size() ~ L.size() + R.size() - 1 -> R[1] ~ R[R.size() - 1]
  for (int d = 1; d < num_heads; ++d) {
    for (int j = max(0, d - (int)L_.size()); 
         j <= min(d - 1, (int)R_.size() - 1); ++j) {
      int i = d - j - 1;
      // expand left
      if (i < L_.size() - 1) {
        // h_offset: offset of h in L or R, ranges from 0 to i + j - 1
        for (int h_offset = 0; h_offset <= i + j; ++h_offset) {
          // head node is either nodes[L[i]] or nodes[R[h_offset - i]];
          int h_idx = h_offset <= i ? nodes_[L_[h_offset]]->Index():
                                      nodes_[R_[h_offset-i]]->Index();
          // whether h is the head of L[i+1]
          int cost_h = 1 - (nodes_[L_[i + 1]]->HIndex() == h_idx);
          // whether L[i + 1] is the head of h
          int cost_i = 1 - (nodes_[h_idx]->HIndex() == nodes_[L_[i + 1]]->Index());
          // t_offset: offset of head node in T[i][j]
          int t_offset = h_offset <= i ? h_offset : 
                                         (L_.size() + h_offset - i - 1);
          T[i+1][j][t_offset] = min(T[i+1][j][t_offset], 
                                    T[i][j][t_offset] + cost_h);
          T[i+1][j][i + 1] = min(T[i+1][j][i + 1], T[i][j][t_offset] + cost_i);
        }
      }

      // expand right
      if (j < R_.size() - 1) {
        // h_offset: offset of h in L or R
        for (int h_offset = 0; h_offset <= i + j; ++h_offset) {
          int h_idx = h_offset <= i ? nodes_[L_[h_offset]]->Index():
                                      nodes_[R_[h_offset-i]]->Index();
          // whether head of R[j+1] is h
          int cost_h = 1 - (nodes_[R_[j + 1]]->HIndex() == h_idx);
          // whether head of h is R[j + 1]
          int cost_j = 1 - (nodes_[h_idx]->HIndex() == nodes_[R_[j + 1]]->Index());
          int t_offset = h_offset <=i ? h_offset : 
                                        (L_.size() + h_offset - i - 1);

          T[i][j+1][t_offset] = min(T[i][j+1][t_offset], 
                                    T[i][j][t_offset] + cost_h);
          T[i][j+1][L_.size() + j] = min(T[i][j + 1][L_.size() + j], 
                                        T[i][j][t_offset] + cost_j);
        }
      }
    }
  }

  int loss = nodes_.size();
  for (int h_offset = 0; h_offset < L_.size() + R_.size() - 1; ++h_offset) {
    int h_idx = (h_offset <= L_.size()-1) ? nodes_[L_[h_offset]]->Index():
                                nodes_[R_[h_offset - L_.size() + 1]]->Index();
    int cost = nodes_[h_idx]->HIndex() < 0 ? 0 : 1;
//    fprintf(stderr, "offset: %-2d, h_idx: %-2d, cost: %-2d\n", h_offset, h_idx, cost);
    int t_offset = h_offset;
    T[L_.size() - 1][R_.size() - 1][t_offset] += cost;
    loss = min(loss, T[L_.size() - 1][R_.size() - 1][t_offset]);
  }
 
#if 0
  for (size_t i = 0; i < L_.size(); ++i)
    for (size_t j = 0; j < R_.size(); ++j) {
      for (size_t m = 0; m < L_.size() + R_.size() - 1; ++m) {
        fprintf(stderr, "T[%d][%d][%d]: %-2d, ", i, j, m, T[i][j][m]);
      }
      fprintf(stderr, "\n");
    }
#endif
  return loss;
}

void CDynamicOracleSearcher::
BuildStacks(const CIDMap::ACTION_TYPE action) {
  int stack_size = state_.GetStackLen();
  L_.resize(action == CIDMap::SHIFT ? (stack_size + 1) : (stack_size - 1));
  R_.clear();
  int idx_q_new = state_.GetQIndex();

  const CDepTree ** stack = state_.GetStack();
  // Build left stack.
  if (action == CIDMap::SHIFT) {
    L_[0] = idx_q_new ++;
    for (int i = 0; i < stack_size; ++i)
      L_[i + 1] = stack[stack_size - i - 1]->Index();
  } else {
    for (int i = 0, k = 0; i < stack_size; ++i) {
      if (i == 0 && action == CIDMap::RIGHT_REDUCE) continue;
      if (i == 1 && action == CIDMap::LEFT_REDUCE) continue;
      L_[k++] = stack[stack_size - i - 1]->Index();
    }
  }

  // Build right stack.
  R_.push_back(L_[0]); 
  for (size_t k = idx_q_new; k < nodes_.size(); ++k) {
    // Note, for shift, the condition is "<="
    if (nodes_[k]->HIndex() < idx_q_new) {
      R_.push_back(k);
    } else if (nodes_[k]->GetLC() != NULL && 
               nodes_[k]->GetLC()->Index() < idx_q_new) {
      R_.push_back(k);
    } else {
      // Check if any dependents of k is already in right_stack.
      for (auto idx: R_) {
        if (nodes_[idx]->HIndex() == k) {
          R_.push_back(k);
          break;
        }
      }
    }
  }
}

void CDynamicOracleSearcher::
InitStackCosts() {
  costs_.clear();
  if (state_.GetStackLen() == 0) return;
  costs_.resize(state_.GetStackLen(), 0);
  total_stack_cost_ = 0;
  const CDepTree ** stack = state_.GetStack();
  vector<const CDepTree *> guess_nodes;

  for (size_t i = 0; i < state_.GetStackLen(); ++i) {
    // Cost of each element on the stack, from top to bottum.
    const CDepTree * tree = stack[state_.GetStackLen() - i - 1];
    tree->CollectTreeNodes(guess_nodes);
    int tree_cost = 0;

    // Compute cost of this element.
    for (const CDepTree * node: guess_nodes) {
      if (node->HIndex() != -1)
        tree_cost += 1 - (nodes_[node->Index()]->HIndex() == node->HIndex() &&
          nodes_[node->Index()]->GetDepId() == node->GetDepId()); 
    }
    costs_[i] = tree_cost;
    total_stack_cost_ += tree_cost;
  }
  top_stack_cost_ = costs_[0];
}

int CDynamicOracleSearcher::
ComputeStackCost(const CIDMap::ACTION_TYPE action) {
  if (action == CIDMap::SHIFT) {
    top_stack_cost_ = 0;
    return total_stack_cost_;
  }

  assert(state_.GetStackLen() > 1);
  int idx0 = state_.GetTopStack()->Index();
  int idx1 = state_.GetStack1()->Index();
    
  int stack_cost = total_stack_cost_;
  if (action == CIDMap::LEFT_REDUCE) {
    top_stack_cost_ = costs_[0];
    if (nodes_[idx1]->HIndex() != idx0) {
      stack_cost += 1 ;
      top_stack_cost_ += 1;
    }
  } else {
    top_stack_cost_ = costs_[1];
    if (nodes_[idx0]->HIndex() != idx1) {
      stack_cost += 1;
      top_stack_cost_ +=1;
    }
  }
  return stack_cost;
}

int CDynamicOracleSearcher::
ComputeLoss(const CIDMap::ACTION_TYPE action) {
  int stack_cost = ComputeStackCost(action);
  BuildStacks(action);
  return stack_cost + LossLinearTree();
}
