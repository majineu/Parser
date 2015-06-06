#include <sstream>
#include <cassert>
#include <set>
#include <iostream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include "SRParser.h"
#include "DepTree.h"
#include "Macros.h"
#include "util.h"
#include "DynamicOracle.h"


bool HaveRightChildInQ(const CState & state, const vector<CDepTree *> &nodes) {
  assert(state.GetStackLen() > 0);
  CDepTree * s0_right_most = nodes[state.GetTopStack()->Index()]->GetRC();
  if (s0_right_most == NULL) return false;
  return s0_right_most->Index() >= state.GetQIndex();
}

size_t StaticOracle(const CState & state,
                    const vector<CDepTree *> &nodes) {
  if (state.GetStackLen() > 1) {
    CDepTree *s0 = nodes[state.GetTopStack()->Index()];
    CDepTree *s1 = nodes[state.GetStack1()->Index()];
    if (s1->HIndex() == s0->Index())
			return CIDMap::GetOutcomeId(CIDMap::LEFT_REDUCE, s1->GetDepId());

    if (!HaveRightChildInQ(state, nodes) && s0->HIndex() == s1->Index())
      return CIDMap::GetOutcomeId(CIDMap::RIGHT_REDUCE, s0->GetDepId());
  }

  if (state.GetQIndex() < nodes.size())
    return CIDMap::GetOutcomeId(CIDMap::SHIFT, 0);
		
  fprintf(stderr, "Error: Gold action not found\nGold tree\n");
  const CDepTree *p_gold_tree = NULL;
  for (auto tree: nodes) {
    if (tree->HIndex() < 0) {
      p_gold_tree = tree;
      break;
    }
  }
  p_gold_tree->DisplayTreeStructure(stderr);
	fprintf(stderr, "Gold State\n");
	state.PrintState();
	exit(0);
}

CSRParser::
CSRParser(size_t bs, bool insertMode, CFeatureCollector *pCF):
m_nBeamSize(bs), m_pCf(pCF) {
	m_verbose = false;
}

CSRParser::~CSRParser(void) {}

CSRParser::OUTCOME_ID CSRParser::
updateGoldState(CDepTree *pGoldTree,   
								CState *pGoldState,   
								vector<CDepTree*> &nodes) {
	if (pGoldState->GetStackLen() >= 2) {
		CDepTree *trees[4];
		pGoldState->GetTopThreeElements(trees);
		int idx0 = trees[0]->Index(), idx1 = trees[1]->Index();
		
		if (nodes[idx1]->HIndex() == nodes[idx0]->Index()) {
			pGoldState->ReduceLeft(nodes[idx1]->GetDepId());
			return CIDMap::GetOutcomeId(CIDMap::LEFT_REDUCE, nodes[idx1]->GetDepId());
		}

		CDepTree *pRChild0 = nodes[idx0]->GetRC();
		if (nodes[idx0]->HIndex() == nodes[idx1]->Index()) 
			if (pRChild0 == NULL || pRChild0->Index() < pGoldState->GetQIndex()) {
				pGoldState->ReduceRight(nodes[idx0]->GetDepId());
				return CIDMap::GetOutcomeId(CIDMap::RIGHT_REDUCE, nodes[idx0]->GetDepId());
			}
	}

	if (pGoldState->GetQIndex() < pGoldTree->GetSen()->Length()) {
		pGoldState->Shift(m_pool, pGoldTree->GetSen());
		return CIDMap::GetOutcomeId(CIDMap::SHIFT, 0);
	} else {
		fprintf(stderr, "Error: Gold action not found\nGold tree\n");
		pGoldTree->DisplayTreeStructure(stderr);
		fprintf(stderr, "Gold State\n");
		pGoldState->PrintState();
		exit(0);
	}
}



CDepTree *InitState(CPool &pool, 
										_SENTENCE *pSen, 
										CState *pState) {
	if (pSen->Length() == 1) {
		vector<int> hIdx(1, -1);
		return CDepTree::BuildTree(hIdx, pSen, pool);
	} else {
		pState->Shift(pool, pSen);
		pState->Shift(pool, pSen);
		return NULL;
	}
}

void PrintFeatScore(vector<_TPLT*> & vTemps, 
										CSparseScorer &scorer,
										int *pFIDs, int *kernels, 
										int oRef, FILE *fp)//, int oPred) 
{
typedef	vector<std::pair<string, double> > _V_TEMP_SCORE;
typedef std::pair<string, double> _STR_SCORE;
typedef	vector<std::pair<_TPLT *, double> > _V_TEMP_SCORE2;
typedef std::pair<_TPLT*, double> _TEMP_SCORE2;
	_V_TEMP_SCORE2 ts1;
	struct comper
	{
		bool operator() (const _STR_SCORE &l, const _STR_SCORE &r)			{return l.second > r.second;}
	};
	
	struct comper2
	{
		bool operator() (const _TEMP_SCORE2 &l, const _TEMP_SCORE2 &r)			{return l.second > r.second;}
	};
	
	double totalScore = 0;
	for (size_t i = 0; i < vTemps.size(); ++i)
	{
		if (pFIDs[i] == -1)
			continue;
		ts1.push_back(_TEMP_SCORE2(vTemps[i], scorer.GetVal(pFIDs[i], oRef)));
		totalScore += scorer.GetVal(pFIDs[i], oRef);
	}

	std::sort(ts1.begin(), ts1.end(), comper2());
	fprintf(fp, "total score:%.2f\n", totalScore);
	int nPrint = 10;
	for (size_t i = 0; i < ts1.size() && (int)i < nPrint; ++i)
	{
		fprintf(fp, "%s|", ts1[i].first->m_strName.c_str());//, 	ts1[i].second);
		
		bool negid = false;
		for (int k = 0; k < ts1[i].first->m_nKernel; ++k)
			if (kernels[ts1[i].first->m_pKidxes[k]] < 0)
				negid = true;
		
		for (int k = 0; negid == false && k < ts1[i].first->m_nKernel; ++k)
			fprintf(fp, "_%s", wstr2utf8(SSenNode::GetKey(kernels[ts1[i].first->m_pKidxes[k]])).c_str());
		fprintf(fp, ":%.2f  ", ts1[i].second);
	}
	fprintf(fp, "\n");
	
	for (int i = (int)ts1.size() -1; i >=0 && (int)ts1.size() - i <= nPrint; --i)
	{
		fprintf(fp, "%s|", ts1[i].first->m_strName.c_str());//, 	ts1[i].second);
		
		bool negid = false;
		for (int k = 0; k < ts1[i].first->m_nKernel; ++k)
			if (kernels[ts1[i].first->m_pKidxes[k]] < 0)
				negid = true;
		
		for (int k = 0; negid == false && k < ts1[i].first->m_nKernel; ++k)
			fprintf(fp, "_%s", wstr2utf8(SSenNode::GetKey(kernels[ts1[i].first->m_pKidxes[k]])).c_str());
		fprintf(fp, ":%.2f  ", ts1[i].second);
	}
	fprintf(fp, "\n\n");
}

bool CSRParser::
ParsingHis(_SENTENCE *pSen, CDepTree *pRef, FILE *fpRes)
{
	SetIMode(false);
	m_Scorer.SetAvgMode(true);
	bool keepMaxEvent = true;//m_verbose = true;
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2;// remove the first 2 shift 
	m_AccCreator.ResetMem();
	m_pool.Recycle();
	List<CDepTree *>::RecyclePool();
	CState CandState, *pState = CState::MakeState(m_pool);// CState();

	CDepTree *res = InitState(m_pool, pSen, &CandState);
	InitState(m_pool, pSen, pState);
	if (res != NULL)
		return true;
	
	CandState.AddAccEvent(m_AccCreator.CreateEvent());

	// initialize beam with pState 
	pState->AddAccEvent(m_AccCreator.CreateEvent());
	pState->SetPossible(true);
	m_Beam.push_back(pState);

	vector<CDepTree *> treeNodeVec;
	vector<int> featureIdVec;
	pRef->CollectTreeNodes(treeNodeVec);

	m_pCf->ResetMem();
	m_pCf->SetSen(pSen);
	if (m_verbose == true)
	{
		fprintf(stderr, "Gold Tree:\n");
		pRef->DisplayTreeStructure(stderr);
		fprintf(stderr, "--------------------------\n");
	}

	// here we go ...
	bool success = true;
	static int SenID = 0;
	SenID ++;
//	m_verbose = true;
	for (int step = 0; step < stepNum; ++ step)
	{	
		// collect gold related features and actions 
		if (m_verbose == true)
			fprintf(stderr, "\n\n\n----------step %d----------\n", step);
		
		vector<int> kernels;
		m_pCf->SetState(&CandState);
		m_pCf->ExtractKernels(kernels);
		m_pCf->GetFeatures(featureIdVec);	
		
		OUTCOME_ID goldOId = updateGoldState(pRef, &CandState, treeNodeVec);
		m_AccCreator.UpdateEventVec(CandState.GetAccEventPtr(), 
																featureIdVec, goldOId);	
			
		updateBeam(keepMaxEvent, m_verbose);
		// update number of total iteration in order to compute the averaged weight 
		if (isGoldSurvive(CandState, step == stepNum-1, true) == false)
		{
			fprintf(fpRes, "\nsentence %d:\n", SenID);
			pRef->DisplayTreeStructure(fpRes);
			fprintf(fpRes, "\n\nGold State\n");
			CandState.PrintState(&CIDMap::GetDepLabelVec(), fpRes);
			int *fids = CandState.GetAccEvent()->FIDS();
			PrintFeatScore(m_pCf->GetTemps(), m_Scorer, fids, &kernels[0], goldOId, fpRes);

			fprintf(fpRes, "\n\nPred State\n");
			m_Beam.back()->PrintState(&CIDMap::GetDepLabelVec(), fpRes);
			fids = m_Beam.back()->GetAccEvent()->FIDS();
			PrintFeatScore(m_pCf->GetTemps(), m_Scorer, fids, &kernels[0],
											m_Beam.back()->GetAccEvent()->OID(), fpRes);
			success = false;
			break;
		}
	}
		
	freeBeam();
	return success;
}


bool CSRParser::
Training(_SENTENCE *pSen, CDepTree *pRef) {
	SetIMode(true);
	m_Scorer.SetAvgMode(false);
	m_pCf->ResetMem();
	m_pCf->SetSen(pSen);
	m_AccCreator.ResetMem();
	m_pool.Recycle();
	List<CDepTree *>::RecyclePool();

  if (CConfig::strTrainingMethod == "standard") {
    return StandardTraining(pSen, pRef);
  } else if (CConfig::strTrainingMethod == "dynamic_oracle_greedy") {
    return DynamicOracleGreedyTraining(pSen, pRef);
  } else if (CConfig::strTrainingMethod == "dynamic_oracle_explore") {
    return DynamicOracleExploreTraining(pSen, pRef);
  } else {
    fprintf(stderr, "Error: unknown training method %s\n", 
        CConfig::strTrainingMethod.c_str());
    exit(0);
  }
}


bool CSRParser::
StandardTraining(_SENTENCE *pSen, CDepTree *pRef) {
	bool keepMaxEvent = true;//m_verbose = true;
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2;// remove the first 2 shift 
	CState CandState, *pState = CState::MakeState(m_pool);// CState();
	CDepTree *res = InitState(m_pool, pSen, &CandState);
	InitState(m_pool, pSen, pState);
	if (res != NULL)
		return true;
	
	CandState.AddAccEvent(m_AccCreator.CreateEvent());

	// initialize beam with pState 
	pState->AddAccEvent(m_AccCreator.CreateEvent());
	pState->SetPossible(true);
	m_Beam.push_back(pState);

	vector<CDepTree *> treeNodeVec;
	vector<int> featureIdVec;
	pRef->CollectTreeNodes(treeNodeVec);

	if (m_verbose == true) {
		fprintf(stderr, "Gold Tree:\n");
		pRef->DisplayTreeStructure(stderr);
		fprintf(stderr, "--------------------------\n");
	}

	// here we go ...
	bool success = true;
	for (int step = 0; step < stepNum; ++ step) {	
		// collect gold related features and actions 
		if (m_verbose == true)
			fprintf(stderr, "\n\n\n----------step %d----------\n", step);
		
		m_pCf->SetState(&CandState);
		m_pCf->GetFeatures(featureIdVec);	
		
		OUTCOME_ID goldOId = updateGoldState(pRef, &CandState, treeNodeVec);
		m_AccCreator.UpdateEventVec(CandState.GetAccEventPtr(), 
																featureIdVec, goldOId);	
		if (m_verbose == true) {
			int depLabel = 0;
			CIDMap::ACTION_TYPE action = CIDMap::Interprate(goldOId, depLabel);
			fprintf(stderr, "oid %d gold Action %d, depLabel %s\n", goldOId, action, 
							action == 0 ? "NULL": wstr2utf8(CIDMap::GetDepLabel(depLabel)).c_str());
		}

			
		updateBeam(keepMaxEvent, m_verbose);
		if (isGoldSurvive(CandState, step == stepNum-1) == false) {	
			m_Scorer.TrainingWithAccEvent(CandState.GetAccEvent(),	
																		CandState.GetLastCorrEvent(),
										  						  m_Beam.back()->GetAccEvent(),  
																		m_Beam.back()->GetLastCorrEvent(), 1.0);

			m_statis.m_nUp ++;
			m_statis.m_nEarly += step < (stepNum - 1);
			success = false;
			break;
		}
	}
		
	freeBeam();
	m_Scorer.IncIt();
	return success;
}

size_t AllowedGuessLabel(const CState & state, const vector<double> &scores,
                        const size_t label, const int sentence_length) {
  CIDMap::ACTION_TYPE action = CIDMap::ClassLabel2Action(label);
  if (state.AllowAction(action, sentence_length)) 
    return label;
  if (state.AllowShift(sentence_length)) 
    return 0;
  size_t best_id = 1;
  for (int i = 2; i < scores.size(); ++i) {
    if (scores[best_id] < scores[i]) 
      best_id = i;
  }
  return best_id;
}

bool CSRParser::
DynamicOracleExploreTraining(_SENTENCE *pSen, CDepTree *pRef) {
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2;// remove the first 2 shift 
	CState state;
	CDepTree *res = InitState(m_pool, pSen, &state);
	if (res != NULL) return true;
	
	vector<CDepTree *> nodes;
	pRef->CollectTreeNodes(nodes);
	vector<int> features;

	if (m_verbose == true) {
		fprintf(stderr, "Gold Tree:\n");
		pRef->DisplayTreeStructure(stderr);
		fprintf(stderr, "--------------------------\n");
	}

	// here we go ...
	bool success = true;
  vector<double> scores;
  bool on_gold_track = true;
	for (int step = 0; step < stepNum; ++ step) {	
		// collect gold related features and actions 
    if (state.AllowReduce() == false) {
      updateState(&state, pSen, 0, 0);
      continue;
    }
		m_pCf->SetState(&state);
		m_pCf->GetFeatures(features);	
		size_t label = m_Scorer.PredictLabel(features, &scores);
		label = AllowedGuessLabel(state, scores, label, nodes.size());
    if (m_verbose == true) {
			fprintf(stderr, "\n\n\n----------step %d----------\n", step);
      CIDMap::PrintClassLabel(label);
    }

    if (on_gold_track == true) {
      if (IsGoldLabel(state, label, nodes) == false) {
        on_gold_track = false;
        size_t best_gold_label = BestGoldLabel(state, scores, nodes);
        if (m_verbose == true) CIDMap::PrintClassLabel(best_gold_label);
        m_Scorer.UpdateParameter(features, best_gold_label, label);
			  m_statis.m_nUp ++;
        if (m_verbose) {
          fprintf(stderr, "\nGold action:\n");
          CIDMap::PrintClassLabel(best_gold_label);
          fprintf(stderr, "\n");
        }
      }
      updateState(&state, pSen, label, scores[label]);
    } else {
      CDynamicOracleSearcher oracle(state, nodes);
      oracle.InitStackCosts();
      std::set<CIDMap::ACTION_TYPE> actions;
      oracle.Oracle(&actions);
      if (actions.find(CIDMap::ClassLabel2Action(label)) != actions.end()) {
        updateState(&state, pSen, label, scores[label]);
      } else {
        int best_id = oracle.BestAction(actions, scores);
        if (m_verbose) {
          fprintf(stderr, "\nOracle action:\n");
          CIDMap::PrintClassLabel(best_id);
          fprintf(stderr, "\n");
        }
        m_Scorer.UpdateParameter(features, best_id, label);
        updateState(&state, pSen, best_id, scores[label]);
			  m_statis.m_nUp ++;
      }
    }
	}
		
	m_Scorer.IncIt();
	return success;
}

bool CSRParser::IsGoldLabel(const CState & state, const size_t label,
                            const vector<CDepTree*> & nodes) {
  size_t static_oracle_label = StaticOracle(state, nodes);
  if (static_oracle_label == label) 
    return true;

  CIDMap::ACTION_TYPE oracle_action = CIDMap::ClassLabel2Action(static_oracle_label);
  CIDMap::ACTION_TYPE guess_action = CIDMap::ClassLabel2Action(label);
  if (oracle_action == CIDMap::LEFT_REDUCE)
    return guess_action == CIDMap::SHIFT && HaveRightChildInQ(state, nodes);

  return false;
}

bool CSRParser::
DynamicOracleGreedyTraining(_SENTENCE *pSen, CDepTree *pRef) {
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2; // remove the first 2 shift 
	CState state;
	CDepTree *res = InitState(m_pool, pSen, &state);
	if (res != NULL)
		return true;
	
	vector<CDepTree *> nodes;
	pRef->CollectTreeNodes(nodes);
	vector<int> features;

	if (m_verbose == true) {
		fprintf(stderr, "Gold Tree:\n");
		pRef->DisplayTreeStructure(stderr);
		fprintf(stderr, "--------------------------\n");
	}

	// here we go ...
	bool success = true;
  vector<double> scores;
	for (int step = 0; step < stepNum; ++ step) {	
    if (state.AllowReduce() == false) {
      updateState(&state, pSen, 0, 0);
      continue;
    }
		m_pCf->SetState(&state);
		m_pCf->GetFeatures(features);	
		size_t label = m_Scorer.PredictLabel(features, &scores);
    label = AllowedGuessLabel(state, scores, label, nodes.size());
		if (m_verbose == true) {
			fprintf(stderr, "\n\n\n----------step %d----------\n", step);
      fprintf(stderr, "Guess Action\n");
      CIDMap::PrintClassLabel(label);
    }

#if 1
    size_t static_oracle = StaticOracle(state, nodes);
    if (label == static_oracle) {
      updateState(&state, pSen, label, scores[label]);
      continue;
    } 
  
    CIDMap::ACTION_TYPE oracle_action = CIDMap::ClassLabel2Action(static_oracle);
    CIDMap::ACTION_TYPE guess_action = CIDMap::ClassLabel2Action(label);
    if (oracle_action == CIDMap::LEFT_REDUCE && 
        guess_action == CIDMap::SHIFT &&
        HaveRightChildInQ(state, nodes)) {
      updateState(&state, pSen, label, scores[label]);
      continue;
    }
    
    size_t best_gold_label = static_oracle;
    if (oracle_action == CIDMap::LEFT_REDUCE && 
        HaveRightChildInQ(state, nodes)) {
      best_gold_label = scores[best_gold_label] > scores[0] ? 
                        best_gold_label : 0;
    }
    m_Scorer.UpdateParameter(features, best_gold_label, label);
    updateState(&state, pSen, best_gold_label, scores[best_gold_label]);
	  m_statis.m_nUp ++;
		m_statis.m_nEarly += step < (stepNum - 1);
#else
    // Parameter update.
    if (IsGoldLabel(state, label, nodes) == false) {
      size_t best_gold_label = BestGoldLabel(state, scores, nodes);
      if (m_verbose == true) {
        fprintf(stderr, "\nGold action\n");
        CIDMap::PrintClassLabel(label);
      }
      m_Scorer.UpdateParameter(features, best_gold_label, label);
			m_statis.m_nUp ++;
			m_statis.m_nEarly += step < (stepNum - 1);
			success = false;
      updateState(&state, pSen, best_gold_label, scores[best_gold_label]);
    } else {
      updateState(&state, pSen, label, scores[label]); 
    }
#endif
	}

	m_Scorer.IncIt();
	return success;
}

CDepTree *CSRParser::
ParsingGreedy(_SENTENCE *pSen) {
	SetIMode(false);
	m_Scorer.SetAvgMode(true);
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2; 
	m_AccCreator.ResetMem();
	m_pool.Recycle();
	List<CDepTree *>::RecyclePool();
	CState *pState = CState::MakeState(m_pool);// CState();
	CDepTree *res = InitState(m_pool, pSen, pState);
	if (res != NULL)
		return res;
	
	// initialize beam with pState 
	m_pCf->ResetMem();
	m_pCf->SetSen(pSen);
  vector<int> features;
  vector<double> scores;
	for (int step = 0; step < stepNum; ++ step) {
    if (pState->AllowReduce() == false) {
      updateState(pState, pSen, 0, 0);
    } else {
		  m_pCf->SetState(pState);
		  m_pCf->GetFeatures(features);	
		  size_t label = m_Scorer.PredictLabel(features, &scores);
      label = AllowedGuessLabel(*pState, scores, label, pSen->Length());
      updateState(pState, pSen, label, 0);
    }
  }
	
	CDepTree *tree = pState->GetTopStack();
	tree->SetHIdx(CDepTree::ROOT_IDX);
	pState->ClearStack();
	return tree;
}

void PrintGoldTree(const vector<CDepTree *> &nodes) {
  for (const CDepTree * node: nodes) {
    if (node->HIndex() < 0) {
      node->DisplayTreeStructure(stderr);
      break;
    }
  }
}

size_t CSRParser::BestGoldLabel(const CState &state, 
                                const vector<double> &scores,
                                const vector<CDepTree *> &nodes) {
  int result = -1;
  int stack_size = state.GetStackLen();
  CDepTree *trees[4];
	state.GetTopThreeElements(trees);
  bool left_reduce_allowed = false;
  if (stack_size >= 2) {
		int idx0 = trees[0]->Index(), idx1 = trees[1]->Index();
    // Check whether LEFT_REDUCE is allowed
    if (nodes[idx1]->HIndex() == nodes[idx0]->Index()) {
      left_reduce_allowed = true;
      int id = CIDMap::GetOutcomeId(CIDMap::LEFT_REDUCE, 
                                    nodes[idx1]->GetDepId());
      result = id;
    } else if (nodes[idx0]->HIndex() == nodes[idx1]->Index()) {
      // Check whether RIGHT_REDUCE is allowed
      if (HaveRightChildInQ(state, nodes) == false) {
        int id = CIDMap::GetOutcomeId(CIDMap::RIGHT_REDUCE, 
                                      nodes[idx0]->GetDepId());
        if (result == -1 || scores[result] > scores[id])
          result = id;
      }
    }
  }
  
  // Check whether SHIFT is allowed
  if (result == -1 || stack_size < 2 || HaveRightChildInQ(state, nodes)) {
		int id = CIDMap::GetOutcomeId(CIDMap::SHIFT, 0);
    if (result == -1 || scores[result] < scores[id]) result = id;
    
    // Update statistics
    if (left_reduce_allowed) {
      m_statis.m_num_conflicts++;
      m_statis.m_num_shift += result == id;
    }
  }

  if (result == -1) {
    fprintf(stderr, "Error: best gold label not found!\n");
    fprintf(stderr, "Gold Tree:\n");
    PrintGoldTree(nodes);
    fprintf(stderr, "\n\n------------state----------\n");
    state.PrintState(NULL, stderr);
    exit(0);
  }
  return result;
}


#if 0
bool CSRParser::IsGoldLabel(const CState & state, const size_t label,
                            const vector<CDepTree*> & nodes) {
  
	int dependency_label = 0;
	CIDMap::ACTION_TYPE action = CIDMap::Interprate(label, dependency_label);
  int stack_size = state.GetStackLen();
  CDepTree *trees[4];
	state.GetTopThreeElements(trees);
  
  if (action == CIDMap::SHIFT) {
    if (state.GetQIndex() >= nodes[0]->GetSen()->Length()) return false;
    if (stack_size < 2) return true; 
    if (HaveRightChildInQ(state, nodes)) return true;
		
    int idx0 = trees[0]->Index();
    int idx1 = trees[1]->Index();
    return (nodes[idx1]->HIndex() != nodes[idx0]->Index() && 
            nodes[idx0]->HIndex() != nodes[idx1]->Index());
  } else {
    if (stack_size < 2)
      return false;
		int idx0 = trees[0]->Index();
    int idx1 = trees[1]->Index();
    
    if (action == CIDMap::LEFT_REDUCE) {
      return nodes[idx1]->HIndex() == nodes[idx0]->Index() && 
             dependency_label == nodes[idx1]->GetDepId();
    } else {
      // Stack 0's right children are not yet collected.
      if (HaveRightChildInQ(state, nodes))
        return false;

      return nodes[idx0]->HIndex() == nodes[idx1]->Index() &&
             dependency_label == nodes[idx0]->GetDepId();
    }
  }
}
#endif


CDepTree *CSRParser::
Parsing(_SENTENCE *pSen)
{
  if (m_nBeamSize == 1) 
    return ParsingGreedy(pSen);

	SetIMode(false);
	m_Scorer.SetAvgMode(true);
	int senLen  = pSen->Length();
	int stepNum = 2 * senLen - 1 - 2; 
	m_AccCreator.ResetMem();
	m_pool.Recycle();
	List<CDepTree *>::RecyclePool();
	CState *pState = CState::MakeState(m_pool);// CState();

	CDepTree *res = InitState(m_pool, pSen, pState);
	if (res != NULL)
		return res;
	
	// initialize beam with pState 
	m_Beam.push_back(pState);
	m_pCf->ResetMem();
	m_pCf->SetSen(pSen);

	for (int step = 0; step < stepNum; ++ step)
		updateBeam(false, m_verbose);
	
	CState *bestState = m_Beam.back();//.front();
	CDepTree *tree = bestState->GetTopStack();
	
	tree->SetHIdx(CDepTree::ROOT_IDX);
	bestState->ClearStack();
	freeBeam();
	return tree;
}


void CSRParser::
updateBeam(bool training, bool verbose)
{	
	// ensure valid beam width
	assert(m_nBeamSize > 0);
	if (m_Beam.size() == 0)
	{
		fprintf(stderr, "Error: Updating empty beam\n");
		exit(0);
	}

	//	1. the feature collector has already been initialized with current sentence
	static vector<int> featureIdVec;
	static vector<double> scoreVec;
	_SENTENCE * pSen		= m_pCf->GetSen();
	double topHeapScore	= MAX_SCORE;
	int senLen	= pSen->Length();


	if (m_verbose == true)
		fprintf(stderr, "\n------------------update beam------------------\n");

	for (size_t i = 0; i < m_Beam.size(); ++i)
	{
		CState *pCurState = m_Beam[i], *newState;
		int 	 index = pCurState->GetQIndex();
		double score = pCurState->GetScore();

		// 1. Collect feature
		m_pCf->SetState(pCurState);
		m_pCf->GetFeatures(featureIdVec);

		// 2. Scoring 
		size_t bestId = m_Scorer.Eval(&featureIdVec[0], (int)featureIdVec.size(), scoreVec);
		assert(bestId != (size_t)-1);

		if (m_verbose == true)
		{
			fprintf(stderr, "\ncurrent state:\n");
			pCurState->PrintState(&CIDMap::GetDepLabelVec());
		}


		// first ensure that at least one element is in the beam
		// so that the topHeapScore is correctly initialized
		if ((int)m_heap.size() == m_nBeamSize && 
				score + scoreVec[bestId] < topHeapScore)
			continue;

		// 3. create new state and update heap
		for (size_t i = 0; i < CIDMap::GetOutcomeNum(); ++i)
		{
			if ((int)m_heap.size() == m_nBeamSize && 
					score + scoreVec[i] < topHeapScore)
				continue;
			
			// skip invalid action 
			int dLabel = 0;
			CIDMap::ACTION_TYPE action = CIDMap::Interprate(i, dLabel);


			if (action == CIDMap::SHIFT && index >= senLen)
				continue;
			 
			if (action != CIDMap::SHIFT && pCurState->GetStackLen() < 2)
				continue;

			// create new state
			newState = CState::CopyState(pCurState, m_pool, training); 
			updateState(newState, pSen, i, scoreVec[i]);
			topHeapScore = updateHeap(newState);

			if (m_verbose == true)
			{
				fprintf(stderr, "new State :\n");
				newState->PrintState(&CIDMap::GetDepLabelVec());
				fprintf(stderr, "\n---------------------\n");
			}

			// 2012 08 07
			if (training == true)
				m_AccCreator.UpdateEventVec(newState->GetAccEventPtr(), featureIdVec, i);
		}
	}

	dumpHeap(false);
}


bool CSRParser:: 
updateState(CState *pState, _SENTENCE *pSen , 
						int classId, double outcomeScore)
{
	int depLabelId = 0;
	CIDMap::ACTION_TYPE action = CIDMap::Interprate(classId, depLabelId); 
	
	if (action == CIDMap::SHIFT)
		pState->Shift(m_pool, pSen);
	
	else if(action == CIDMap::LEFT_REDUCE)
		pState->ReduceLeft(depLabelId);
	
	else if(action == CIDMap::RIGHT_REDUCE)
		pState->ReduceRight(depLabelId);
	
	else
	{
		fprintf(stderr, "ERROR: unknown action in goldTree\n");
		return false;
	}

	pState->IncreaseScore(outcomeScore);
	return true;
}


bool CSRParser::
isGoldSurvive(CState & CandState, bool lastStep, bool ignoreDLabel) 
{
	bool survive = false;
	vector<CState*> ::iterator it  = m_Beam.begin();
	if (lastStep)	// only check the top score element
		it += m_Beam.size() - 1;

	if (m_verbose == true)
	{
		fprintf(stderr, "\n\ncheck gold state survive\n\ngold state:\n");
		CandState.PrintState(&CIDMap::GetDepLabelVec());
		fprintf(stderr, "\nState in the beam\n");
	}

	for(; it!= m_Beam.end(); ++it)
	{
		if (m_verbose == true)
			(*it)->PrintState(&CIDMap::GetDepLabelVec());

		if ((*it)->IsPossible() == false)
			continue;

		if (CandState.IsGold(*it, ignoreDLabel) == true)
			survive = true;
		else
			(*it)->SetPossible(false);
	}
		
	return survive;
}

