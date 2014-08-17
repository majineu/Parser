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


CSRParser::
CSRParser(size_t bs, bool insertMode, CFeatureCollector *pCF):
m_nBeamSize(bs), m_pCf(pCF)
{
	m_verbose = false;
}


CSRParser::~CSRParser(void)
{
}


CSRParser::OUTCOME_ID CSRParser::
updateGoldState(CDepTree *pGoldTree,   
								CState *pGoldState,   
								vector<CDepTree*> &nodes)
{
	if (pGoldState->GetStackLen() >= 2)
	{
		CDepTree *trees[4];
		pGoldState->GetTopThreeElements(trees);
		int idx0 = trees[0]->Index(), idx1 = trees[1]->Index();
		
		if (nodes[idx1]->HIndex() == nodes[idx0]->Index()) 
		{
			pGoldState->ReduceLeft(nodes[idx1]->GetDepId());
			return CIDMap::GetOutcomeId(CIDMap::LEFT_REDUCE, nodes[idx1]->GetDepId());
		}

		CDepTree *pRChild0 = nodes[idx0]->GetRC();
		if (nodes[idx0]->HIndex() == nodes[idx1]->Index()) 
			if (pRChild0 == NULL || pRChild0->Index() < pGoldState->GetQIndex())
			{
				pGoldState->ReduceRight(nodes[idx0]->GetDepId());
				return CIDMap::GetOutcomeId(CIDMap::RIGHT_REDUCE, nodes[idx0]->GetDepId());
			}
	}

	if (pGoldState->GetQIndex() < pGoldTree->GetSen()->Length()) 
	{
		pGoldState->Shift(m_pool, pGoldTree->GetSen());
		return CIDMap::GetOutcomeId(CIDMap::SHIFT, 0);
	}
	else
	{
		fprintf(stderr, "Error: Gold action not found\nGold tree\n");
		pGoldTree->DisplayTreeStructure(stderr);
		fprintf(stderr, "Gold State\n");
		pGoldState->PrintState();
		exit(0);
	}
}



CDepTree *InitState(CPool &pool, 
										_SENTENCE *pSen, 
										CState *pState)
{
	if (pSen->Length() == 1)
	{
		vector<int> hIdx(1, -1);
		return CDepTree::BuildTree(hIdx, pSen, pool);
	}
	else
	{
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
Training(_SENTENCE *pSen, CDepTree *pRef)
{
	SetIMode(true);
	m_Scorer.SetAvgMode(false);
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
	for (int step = 0; step < stepNum; ++ step)
	{	
		// collect gold related features and actions 
		if (m_verbose == true)
			fprintf(stderr, "\n\n\n----------step %d----------\n", step);
		
		m_pCf->SetState(&CandState);
		m_pCf->GetFeatures(featureIdVec);	
		
		OUTCOME_ID goldOId = updateGoldState(pRef, &CandState, treeNodeVec);
		m_AccCreator.UpdateEventVec(CandState.GetAccEventPtr(), 
																featureIdVec, goldOId);	
		if (m_verbose == true)
		{
			int depLabel = 0;
			CIDMap::ACTION_TYPE action = CIDMap::Interprate(goldOId, depLabel);
			fprintf(stderr, "oid %d gold Action %d, depLabel %s\n", goldOId, action, 
							action == 0 ? "NULL": wstr2utf8(CIDMap::GetDepLabel(depLabel)).c_str());
		}

			
		updateBeam(keepMaxEvent, m_verbose);
		if (isGoldSurvive(CandState, step == stepNum-1) == false)
		{	
			m_Scorer.TrainingWithAccEvent(CandState.GetAccEvent(),	
																		CandState.GetLastCorrEvent(),
										  						  m_Beam.back()->GetAccEvent(),  
																		m_Beam.back()->GetLastCorrEvent(), 1.0);

			m_statis.m_nUp ++;//++ totalUpdate;
			m_statis.m_nEarly += step < stepNum - 1;
			success = false;
			break;
		}
	}
		
	freeBeam();
	m_Scorer.IncIt();
	return success;
}

CDepTree *CSRParser::
Parsing(_SENTENCE *pSen)
{
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

