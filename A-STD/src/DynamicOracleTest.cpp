#include "gtest/gtest.h"
#include "DynamicOracle.h"

class DynamicOracleTest: public ::testing::Test {
 public:
 protected:
  virtual void SetUp() {
    const wstring tokens[]= {
      L"Rolls-Royce	NNP	4	NMOD",
    L"Motor	NNP	4	NMOD",
    L"Cars	NNPS	4	NMOD",
    L"Inc.	NNP	5	SUB",
    L"said	VBD	0	ROOT",
    L"it	PRP	7	SUB",
    L"expects	VBZ	5	VMOD",
    L"its	PRP$	10	NMOD",
    L"U.S.	NNP	10	NMOD",
    L"sales	NNS	12	SUB",
    L"to	TO	12	VMOD",
    L"remain	VB	7	VMOD",
    L"steady	JJ	12	PRD",
    L"at	IN	12	VMOD",
    L"about	IN	17	NMOD",
    L"1,200	CD	15	AMOD",
    L"cars	NNS	14	PMOD",
    L"in	IN	12	VMOD",
    L"1990	CD	18	PMOD",
    L".	.	5	P"};
    int len = 20;
    vector<wstring> lines(tokens, tokens + len);
    vector<int> indexes;

    List<CDepTree*>::SetPool(&pool_);
    sentence_ = SSentence::CreateSentence(pool_, lines, true, &indexes);
    sentence_->AppendPuncs(pool_, indexes);
    
    for (int i = 0; i < sentence_->Length(); ++i)
      CIDMap::AddDependencyLabel(sentence_->Label(i));
    
    tree_ = CDepTree::BuildTree(indexes, sentence_, pool_);
    tree_->CollectTreeNodes(nodes_);
    state_ = CState::MakeState(pool_);
    CIDMap::BuildOutcomeIds();
  }

  virtual void TearDown() {
    tree_ = nullptr;
    sentence_ = nullptr;
    delete oracle_;
    oracle_ = nullptr;
    state_ = nullptr;
    pool_.Recycle();
  }

 public:
  SSentence *sentence_;
  CPool pool_;
  CDepTree *tree_;
  CDynamicOracleSearcher *oracle_;
  CState *state_;
  vector<CDepTree *> nodes_;
};
                   
TEST_F(DynamicOracleTest, BuildStacks) {
  state_->Shift(pool_, sentence_);
  CDynamicOracleSearcher oracle1(*state_, nodes_);
  oracle1.BuildStacks(CIDMap::SHIFT);
  ASSERT_EQ(oracle1.LeftStack().size(), 2);
  // Right stack: Inc, said
  EXPECT_EQ(oracle1.RightStack().size(), 3);
  EXPECT_EQ(oracle1.LeftStack(0), 1);
  EXPECT_EQ(oracle1.LeftStack(1), 0);
  EXPECT_EQ(oracle1.RightStack(0), 1);
  EXPECT_EQ(oracle1.RightStack(1), 3);
  EXPECT_EQ(oracle1.RightStack(2), 4);

  state_->Shift(pool_, sentence_);
  CDynamicOracleSearcher oracle2(*state_, nodes_);
  oracle2.BuildStacks(CIDMap::SHIFT);
  ASSERT_EQ(oracle2.LeftStack().size(), 3);
  EXPECT_EQ(oracle2.RightStack().size(), 3);
  EXPECT_EQ(oracle2.LeftStack(0), 2);
  EXPECT_EQ(oracle2.LeftStack(1), 1);
  EXPECT_EQ(oracle2.LeftStack(2), 0);
  EXPECT_EQ(oracle2.RightStack(0), 2);
  EXPECT_EQ(oracle2.RightStack(1), 3);
  EXPECT_EQ(oracle2.RightStack(2), 4);
  
  // Build an arc (1, 0), and 1 becomes top of stack
  oracle2.BuildStacks(CIDMap::LEFT_REDUCE);
  ASSERT_EQ(oracle2.LeftStack().size(), 1);
  EXPECT_EQ(oracle2.RightStack().size(), 3);
  EXPECT_EQ(oracle2.LeftStack(0), 1);
  EXPECT_EQ(oracle2.RightStack(0), 1);
  EXPECT_EQ(oracle2.RightStack(1), 3);
  EXPECT_EQ(oracle2.RightStack(2), 4);
  
  // Build an arc (0, 1), and 0 becomes top of stack
  oracle2.BuildStacks(CIDMap::RIGHT_REDUCE);
  ASSERT_EQ(oracle2.LeftStack().size(), 1);
  ASSERT_EQ(oracle2.RightStack().size(), 3);
  EXPECT_EQ(oracle2.LeftStack(0), 0);
  EXPECT_EQ(oracle2.RightStack(0), 0);
  EXPECT_EQ(oracle2.RightStack(1), 3);
  EXPECT_EQ(oracle2.RightStack(2), 4);

  state_->Shift(pool_, sentence_);
  // stack top Inc 4
  state_->Shift(pool_, sentence_);
  CDynamicOracleSearcher oracle3(*state_, nodes_);
  oracle3.BuildStacks(CIDMap::SHIFT);
  ASSERT_EQ(5, oracle3.LeftStack().size());
  ASSERT_EQ(3, oracle3.RightStack().size());
  EXPECT_EQ(4, oracle3.LeftStack(0));
  EXPECT_EQ(3, oracle3.LeftStack(1));
  EXPECT_EQ(2, oracle3.LeftStack(2));
  EXPECT_EQ(1, oracle3.LeftStack(3));
  EXPECT_EQ(0, oracle3.LeftStack(4));
  EXPECT_EQ(4, oracle3.RightStack(0));
  EXPECT_EQ(6, oracle3.RightStack(1));
  EXPECT_EQ(19, oracle3.RightStack(2));

  oracle3.BuildStacks(CIDMap::LEFT_REDUCE);
  ASSERT_EQ(3, oracle3.LeftStack().size());
  ASSERT_EQ(2, oracle3.RightStack().size());
  EXPECT_EQ(3, oracle3.LeftStack(0));
  EXPECT_EQ(1, oracle3.LeftStack(1));
  EXPECT_EQ(0, oracle3.LeftStack(2));
  EXPECT_EQ(3, oracle3.RightStack(0));
  EXPECT_EQ(4, oracle3.RightStack(1));
  
  oracle3.BuildStacks(CIDMap::RIGHT_REDUCE);
  ASSERT_EQ(3, oracle3.LeftStack().size());
  ASSERT_EQ(2, oracle3.RightStack().size());
  EXPECT_EQ(2, oracle3.LeftStack(0));
  EXPECT_EQ(1, oracle3.LeftStack(1));
  EXPECT_EQ(0, oracle3.LeftStack(2));
  EXPECT_EQ(2, oracle3.RightStack(0));
  EXPECT_EQ(4, oracle3.RightStack(1));
}

TEST_F(DynamicOracleTest, CostTest) {
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  CDynamicOracleSearcher oracle1(*state_, nodes_);
  oracle1.InitStackCosts();
  ASSERT_EQ(2, oracle1.CostsNum());
  EXPECT_EQ(0, oracle1.Cost(0));
  EXPECT_EQ(0, oracle1.Cost(1));
  int total_cost = oracle1.ComputeStackCost(CIDMap::SHIFT);
  EXPECT_EQ(0, total_cost);
  EXPECT_EQ(0, oracle1.TopStackCost());

  total_cost = oracle1.ComputeStackCost(CIDMap::LEFT_REDUCE);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(1, oracle1.TopStackCost());
  
  total_cost = oracle1.ComputeStackCost(CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(1, oracle1.TopStackCost());

  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  // Wrong dependency label.
  state_->ReduceLeft(nodes_[5]->GetDepId());
  CDynamicOracleSearcher oracle2(*state_, nodes_);
  oracle2.InitStackCosts();
  ASSERT_EQ(3, oracle2.CostsNum());
  EXPECT_EQ(1, oracle2.TotalCost());
  EXPECT_EQ(1, oracle2.Cost(0));
  EXPECT_EQ(0, oracle2.Cost(1));
  EXPECT_EQ(0, oracle2.Cost(2));
  total_cost = oracle2.ComputeStackCost(CIDMap::SHIFT);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(0, oracle2.TopStackCost());

  state_ = CState::MakeState(pool_);
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  state_->ReduceLeft(nodes_[2]->GetDepId());

  CDynamicOracleSearcher oracle3(*state_, nodes_);
  oracle3.InitStackCosts();
  ASSERT_EQ(3, oracle3.CostsNum());
  EXPECT_EQ(0, oracle3.TotalCost());
  EXPECT_EQ(0, oracle3.Cost(0));
  EXPECT_EQ(0, oracle3.Cost(1));
  EXPECT_EQ(0, oracle3.Cost(2));
  total_cost = oracle3.ComputeStackCost(CIDMap::SHIFT);
  EXPECT_EQ(0, total_cost);
  EXPECT_EQ(0, oracle3.TopStackCost());
  total_cost = oracle3.ComputeStackCost(CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, total_cost);
  EXPECT_EQ(0, oracle3.TopStackCost());
  total_cost = oracle3.ComputeStackCost(CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(1, oracle3.TopStackCost());
  
  state_->ReduceLeft(nodes_[1]->GetDepId());
  CDynamicOracleSearcher oracle4(*state_, nodes_);
  oracle4.InitStackCosts();
  ASSERT_EQ(2, oracle4.CostsNum());
  EXPECT_EQ(0, oracle4.TotalCost());
  EXPECT_EQ(0, oracle4.Cost(0));
  EXPECT_EQ(0, oracle4.TopStackCost());
  total_cost = oracle4.ComputeStackCost(CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, total_cost);
  EXPECT_EQ(0, oracle4.TopStackCost());
  total_cost = oracle4.ComputeStackCost(CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(1, oracle4.TopStackCost());

  state_->Shift(pool_, sentence_);
  state_->ReduceLeft(nodes_[3]->GetDepId());
  
  CDynamicOracleSearcher oracle5(*state_, nodes_);
  oracle5.InitStackCosts();
  ASSERT_EQ(2, oracle5.CostsNum());
  EXPECT_EQ(0, oracle5.TotalCost());
  EXPECT_EQ(0, oracle5.TopStackCost());
  total_cost = oracle5.ComputeStackCost(CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(1, total_cost);
  EXPECT_EQ(1, total_cost);
  
  state_->ReduceLeft(nodes_[1]->GetDepId());
  CDynamicOracleSearcher oracle6(*state_, nodes_);
  oracle6.InitStackCosts();
  ASSERT_EQ(1, oracle6.CostsNum());
  EXPECT_EQ(1, oracle6.TotalCost());
  EXPECT_EQ(1, oracle5.TopStackCost());
}

int ComputeLinearTreeLoss(const CState & state,
                          const vector<CDepTree *> & nodes,
                          const CIDMap::ACTION_TYPE action) {
  CDynamicOracleSearcher oracle(state, nodes);
  oracle.InitStackCosts();
  oracle.BuildStacks(action);
  oracle.ComputeStackCost(action);
  return oracle.LossLinearTree();
}

int ComputeActionLoss(const CState & state, const vector<CDepTree *> & nodes,
                      const CIDMap::ACTION_TYPE action) {
  CDynamicOracleSearcher oracle(state, nodes);
  oracle.InitStackCosts();
  return oracle.ComputeLoss(action);
}

TEST_F(DynamicOracleTest, LinearTreeLoss) {
  state_->Shift(pool_, sentence_);
  int loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(0, loss);
  
  state_->Shift(pool_, sentence_);
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(0, loss);
  
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, loss);
  
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(2, loss);
  
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, loss);
  
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(3, loss);

  loss = ComputeActionLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(2, loss);
  loss = ComputeActionLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, loss);
  loss = ComputeActionLoss(*state_, nodes_, CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(4, loss);

  state_->ReduceLeft(nodes_[state_->GetStack1()->Index()]->GetDepId());
  state_->ReduceLeft(nodes_[state_->GetStack1()->Index()]->GetDepId());
  state_->ReduceLeft(nodes_[state_->GetStack1()->Index()]->GetDepId());
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(0, loss);
  
  state_->Shift(pool_, sentence_);
  state_->Shift(pool_, sentence_);
  EXPECT_EQ(5, state_->GetTopStack()->Index());
  EXPECT_EQ(4, state_->GetStack1()->Index());
  //                   2    1    0        
  // stack and queue: Inc. said it   |   expects
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(0, loss);
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(3, loss);
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::RIGHT_REDUCE);
  EXPECT_EQ(0, loss);
  
  // Add arc (said, it) which is a wrong action.
  //                   1    0        
  // stack and queue: Inc. said   |   expects
  state_->ReduceRight(state_->GetTopStack()->GetDepId());
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::SHIFT);
  EXPECT_EQ(0, loss);

  CDynamicOracleSearcher oracle(*state_, nodes_);
  oracle.InitStackCosts();
  EXPECT_EQ(1, oracle.TopStackCost());
  EXPECT_EQ(1, oracle.TotalStackCost());
  
  oracle.ComputeStackCost(CIDMap::LEFT_REDUCE);
  EXPECT_EQ(1, oracle.TopStackCost());
  EXPECT_EQ(1, oracle.TotalStackCost());
  
  loss = ComputeLinearTreeLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(0, loss);
  
  loss = ComputeActionLoss(*state_, nodes_, CIDMap::LEFT_REDUCE);
  EXPECT_EQ(1, loss);
  
//  loss = ComputeActionLoss(*state_, nodes_, CIDMap::RIGHT_REDUCE);
//  EXPECT_EQ(4, loss);
}

int BestAction(const CState & state, const vector<CDepTree*> & nodes) {
  set<CIDMap::ACTION_TYPE> actions;
  CDynamicOracleSearcher oracle(state, nodes);
  oracle.InitStackCosts();
//  oracle.Oracle(0, &actions);
  oracle.Oracle(&actions);
  vector<double> scores(CIDMap::GetOutcomeNum(), 0.);
  return oracle.BestAction(actions, scores);
}

TEST_F(DynamicOracleTest, OracleTest) {
  while (state_->GetStackLen() > 1 || state_->GetQIndex() < nodes_.size()) {
    int id = BestAction(*state_, nodes_);
    int label = 0;
    CIDMap::ACTION_TYPE action = CIDMap::Interprate(id, label);
    if (action == CIDMap::SHIFT) {
      state_->Shift(pool_, sentence_);
    } else if (action == CIDMap::LEFT_REDUCE) {
      state_->ReduceLeft(label);
    } else {
      state_->ReduceRight(label);
    }
  }

  vector<CDepTree *> nodes;
  state_->GetTopStack()->CollectTreeNodes(nodes);
  ASSERT_EQ(nodes_.size(), nodes.size());
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes_[i]->HIndex() >= 0) {
      EXPECT_EQ(nodes_[i]->HIndex(), nodes[i]->HIndex());
    } else {
      EXPECT_LT(nodes[i]->HIndex(), 0);
    }
  }
}
