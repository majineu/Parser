#pragma once
#include "DepTree.h"
#include "Scorer.h"
#include "Pool.h"
#include "FeatureCollector.h"
#include "SparseScorer.h"

#include <map>
#include <sstream>
#include <vector>
#include <queue>
#include <algorithm>

class CEvaler
{
public:
	bool static Eval(vector<CDepTree*>  &rGolds,  vector<CDepTree*>  &rGuesses,  double * pPerformance);
};

struct SStatis
{
	int m_nUp;
	int m_nEarly;
	int m_nPuUps;
	void Reset() 							{memset(this, 0, sizeof(*this));}
};

class CSRParser
{
public:
	CSRParser(size_t bs, bool insertMode, CFeatureCollector *pCF);//, bool useDis = false);
	~CSRParser(void);
	CDepTree * Parsing(_SENTENCE *pSen);
	bool Training(_SENTENCE *pSen, CDepTree *pRef);
	bool ParsingHis(_SENTENCE *pSen, CDepTree *pRef, FILE *fpRes = stderr);
//	bool Checking(_SENTENCE *pSen, CDepTree *pRef);
	void ResetStatis()																	{m_statis.Reset();}
	void SetIMode(bool mode)														{m_pCf->SetMode(mode), m_Scorer.SetInsertMode(mode);}
	void SetVerbose(bool v)															{m_verbose = v;}			
	SStatis   Statis()																	{return m_statis;}
	CSparseScorer * GetScorer()													{return &m_Scorer;}
	typedef int ACTION_ID;
	typedef int OUTCOME_ID;

private:
	void freeBeam()																			{m_Beam.clear();}
	CDepTree* parsing(_SENTENCE *pSen);
	
	bool isGoldSurvive(CState & CandState, bool parsingComplete, 
										 bool ignoreLabel = false);
	bool updateBeam(bool keepMaxEvent, bool verbose = false);
	bool updateState(CState *pState, _SENTENCE *pSen , 
									 int classId, double outcomeScore);	
	OUTCOME_ID updateGoldState(CDepTree * pGoldTree, CState * pGoldState, 
														 vector<CDepTree*> & pTreeNodeVec);
	double updateHeap(CState *pState)
	{
		if((int)m_heap.size() == m_nBeamSize)
			m_heap.pop();
		m_heap.push(pState);
		return m_heap.top()->GetScore();
	}

	void dumpHeap(bool verbose)
	{	
		if (verbose)
			fprintf(stderr, "beam candidates:\n");
		freeBeam();
		while (!m_heap.empty())
		{
			if (verbose)
				m_heap.top()->PrintState(&CIDMap::GetDepLabelVec());

			m_Beam.push_back(m_heap.top());
			m_heap.pop();
		}
	}

private:
//	Perceptron::CPerceptronScorer		m_Scorer;
//	Perceptron::CAccEventCreator		m_AccCreator;
	CAccEventCreator		m_AccCreator;
	CSparseScorer     	m_Scorer;
	int									m_nBeamSize;
	vector<CState*>			m_Beam; 
	CStrHashMap					m_dlMap;
	CFeatureCollector		*m_pCf;
	bool 								m_verbose;
	CPool  							m_pool;
	SStatis							m_statis;
	std::priority_queue<CState*, vector<CState*>, CState >	m_heap;
};  

