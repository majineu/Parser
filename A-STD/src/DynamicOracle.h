#ifndef __DYNAMIC_ORACLE_H__
#define __DYNAMIC_ORACLE_H__
// Implements dynamic oracle related functions.
#include <set>
#include "State.h"
#include "Maps.h"
using std::set;

class CDynamicOracleSearcher {
  // Usage:
  // 1 InitStackCosts
  // 2 Oracle    to get the action set
  // 3 If the predicted action is not in Oracle, then Call BestAction.
 public:
  CDynamicOracleSearcher(const CState & state, const vector<CDepTree *> &nodes):
    state_(state), nodes_(nodes), top_stack_cost_(0), total_stack_cost_(0) {}

  // Build left and right stacks.
  // Note that left_stack[0] and right_stack[0] points to the same element.
  void BuildStacks(const CIDMap::ACTION_TYPE action);

  // Return the total stack cost after perform action.
  // Also update top stack cost according to action.
  int ComputeStackCost(const CIDMap::ACTION_TYPE action);

  // Initialize costs_ and total_stack_cost_ with costs of current state.
  void InitStackCosts();

  // Get the highest score action from oracles.
  // When REDUCE action builds a gold arc, we choose the gold dep label.
  // Otherwise, we select the label with the highest model score.
  int BestAction(const set<CIDMap::ACTION_TYPE> &actions, 
           const vector<double> &scores);

  // Compute oracle actions.
  int Oracle(const int loss_last_step, set<CIDMap::ACTION_TYPE> *actions);
  int Oracle(set<CIDMap::ACTION_TYPE> *actions);

  // Compute loss after perform action.
  int ComputeLoss(const CIDMap::ACTION_TYPE action);

  // The DP algorithm.
  int LossLinearTree();

  const vector<int> &LeftStack()  const              {return L_;};
  const vector<int> &RightStack() const              {return R_;}
  int TopStackCost() const                           {return top_stack_cost_;}
  int TotalStackCost() const                         {return total_stack_cost_;}
  int LeftStack(int idx) const                       {return L_[idx];}
  int RightStack(int idx) const                      {return R_[idx];}
  int CostsNum() const                               {return costs_.size();}
  int Cost(int idx) const                            {return costs_[idx];}
  int TotalCost() const                              {return total_stack_cost_;}
 private:
  vector<int> L_; // left stack.
  vector<int> R_; // right stack.
  vector<int> costs_;  // left stack costs;
  
  const CState &state_;
  
  // Reference nodes.
  const vector<CDepTree *> &nodes_;

  // Top L stack cost of the current state.
  int top_stack_cost_;

  // Sum of costs in left stack.
  int total_stack_cost_; 
};

#if 0
int Oracle(const CState & state, const vector<CDepTree*> &nodes,
           const int loss_last_step, set<CIDMap::ACTION_TYPE> *actions);

// Given the action, compute the corresponding loss.
int ComputeLoss(const vector<CDepTree *> &nodes,
                const CIDMap::ACTION_TYPE action,
                const CState &state,
                const vector<int> &element_costs,
                const int cost_sum);

// Compute the loss of the best linear tree, according to gold standard nodes.
int LossLinearTree(const vector<CDepTree *> &nodes, const vector<int> &L,
                   const vector<int> &R, int top_stack_cost);
  
// Goldberg et al., 2014 (Dynamic oracle).
// Cost of each element on the stack, from top to bottum.
int StackCosts(const vector<CDepTree *> &gold_nodes, 
                 vector<int> * costs);

// Update cost after an action, here labels are ignored.
void UpdateCost(const vector<CDepTree *> &gold_nodes,
                  const CIDMap::ACTION_TYPE action,
                  const vector<int> & costs,
                  const int old_cost_sum,
                  int * cost_sum,
                  int * top_stack_cost);

// Build left and right stacks.
// Note that left_stack[0] and right_stack[0] points to the same element.
void BuildStacks(const vector<CDepTree *> &nodes,
                   CIDMap::ACTION_TYPE action,
                   vector<int> *left_stack);
#endif


#endif  /*__DYNAMIC_ORACLE_H__*/
