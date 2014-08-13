#ifndef __SPARSE_SCORER_H__
#define __SPARSE_SCORER_H__
#include <vector>
#include <cstdlib>
#include <string>
#include "Pool.h"

using std::string;
using std::vector;
struct SEvent
{
	int 	m_nFID;
	int 	m_oID;
	int 	*m_pFID;
	SEvent *m_pNext;

	int FNum()				{return m_nFID;}
	int OID()					{return m_oID;}
	int *FIDS()				{return m_pFID;}
};


class CAccEventCreator
{
public:
	CAccEventCreator()						{};
	void ResetMem()								{m_Pool.Recycle();}

	SEvent * CreateEvent()
	{
		SEvent *pEvent = (SEvent *)m_Pool.Allocate(sizeof(SEvent));
		memset(pEvent, 0, sizeof(*pEvent));
		return pEvent;
	}

	bool UpdateEventVec(SEvent **pHead, const vector<int> &rFIdVec, int outcomeId)
	{
		if (pHead == NULL && rFIdVec.size() == 0)
			return false;
		
		// 1. initialize outcome id 
		SEvent *pSEvent = CreateEvent();
		pSEvent->m_oID  = outcomeId;
		
		// 2. initialize feature id array 
		pSEvent->m_nFID = (int)rFIdVec.size();
		pSEvent->m_pFID = (int*)m_Pool.Allocate(sizeof(int) * pSEvent->m_nFID);
		memcpy(pSEvent->m_pFID, &rFIdVec[0], sizeof(int) * pSEvent->m_nFID);

		// 3. insert at head 
		pSEvent->m_pNext = *pHead;
		*pHead = pSEvent;
		
		return true;
	}

	
private:
	CPool m_Pool;
};


class CSparseScorer
{
	const static size_t s_initSize = 1024 * 1024 * 64;
	const static int unkId = -1;
	const static int verbose = 0;

	typedef unsigned short OID_TYPE;  
	typedef double		   FLOAT_TYPE;


	struct ValAry
	{
		static const int initSize = 2; // this value should be larger than 1
		static const int growFactor = 2;

		struct Item
		{
			OID_TYPE oid;
			FLOAT_TYPE  val;
			FLOAT_TYPE	sumVal;
			static Item * MakeItem(CPool &rPool, OID_TYPE oid, FLOAT_TYPE v, FLOAT_TYPE sumV)
			{
				Item *it = (Item *)rPool.Allocate(sizeof(Item));
				it->oid = oid;
				it->val = v;
				it->sumVal = sumV;

				return it;
			}
		};
		
		unsigned short eleNum;
		unsigned short size;

		OID_TYPE	*pOids;
		FLOAT_TYPE  *pVals;
		FLOAT_TYPE  *pSumVals;

		Item ** pItemAry;

		

		static ValAry* MakeValAry(CPool &rPool, OID_TYPE oid, FLOAT_TYPE v, FLOAT_TYPE sumV)
		{
			ValAry *pVal = (ValAry *)rPool.Allocate(sizeof(ValAry));
			pVal->eleNum = 1;
			pVal->size = initSize;
			pVal->pOids = (OID_TYPE *) rPool.Allocate((sizeof(OID_TYPE) + 2 * sizeof(FLOAT_TYPE)) * pVal->size);
			pVal->pVals = (FLOAT_TYPE *)(pVal->pOids + pVal->size);
			pVal->pSumVals = pVal->pVals + pVal->size;

			pVal->pOids[0]		= oid;
			pVal->pVals[0]		= v;
			pVal->pSumVals[0]	= sumV;
			return pVal;
		}

		int Find(OID_TYPE oid)
		{
			for (int i = 0; i < eleNum; ++i)
				if (pOids[i] == oid)
					return i;

			return -1;
		}

		void UpdateVal(CPool &rPool, OID_TYPE oid, 
									FLOAT_TYPE v, FLOAT_TYPE sumV, 
									bool insertMode = true)
		{	
			for (int i = 0; i < eleNum; ++i)
			{
				if (pOids[i] == oid)
				{
					pVals[i] += v;
					pSumVals[i] += sumV;
					return ;
				}
			}

			if (insertMode == false)
				return ;

#if 1
			if (eleNum + 1 == size)
			{
				size *= growFactor;

				OID_TYPE *pNew = (OID_TYPE *) rPool.Allocate((sizeof(OID_TYPE) + 2 * sizeof(FLOAT_TYPE)) * size);
				memcpy(pNew, pOids, sizeof(OID_TYPE) * eleNum);
				memcpy(pNew + size, pVals, sizeof(FLOAT_TYPE) * eleNum);
				memcpy((FLOAT_TYPE *)(pNew + size) + size, pSumVals, sizeof(FLOAT_TYPE) * eleNum);
				
				pOids = pNew;
				pVals = (FLOAT_TYPE *)(pOids + size);
				pSumVals = pVals + size;
			}
#endif

			pOids[eleNum] = oid;
			pVals[eleNum] = v;
			pSumVals[eleNum] = sumV;
			++eleNum;
		}

	};

	vector<ValAry*> m_vecValPtrs;


private:
	/* newly modified */
	CPool			m_pool;
	bool			m_avgMode;
	bool			m_insertMode;
	int 			m_It;
	size_t			m_nOutcomeNum;
public:
	enum VAL_TYPE {POS_ONLY, NEG_ONLY, BOTH, ALL_ZERO};
	~CSparseScorer()										{}
	CSparseScorer()											{m_avgMode = false; m_It = 0;m_insertMode = true;} 
	void PrintParameter()								{}
	void Print(int gid)									{}
	void SetInsertMode(bool m)					{m_insertMode = m;}
	void SetAvgMode(bool m)							{m_avgMode = m;}
	void ResetIt()											{m_It = 0;}
	void IncIt()												{++m_It;}
	void SetOutcomeNum(size_t n)				{m_nOutcomeNum = n;}
	size_t GetParamNum()								{return m_vecValPtrs.size();}
	int GetItNum()											{return m_It;}


	bool operator == (const CSparseScorer &r)const
	{
		if (m_It != r.m_It)
		{
			fwprintf(stderr, L"Iteration number %d vs %d\n", m_It, r.m_It);
			return false;
		}

		if (m_nOutcomeNum != r.m_nOutcomeNum)
		{
			fwprintf(stderr,L"outcomeNum %d vs %d\n", m_nOutcomeNum, r.m_nOutcomeNum);
			return false;
		}


		if (m_vecValPtrs.size() != r.m_vecValPtrs.size())
		{
			fwprintf(stderr, L"inconsistent Size");
			return false;
		}

		for (size_t i = 0; i < m_vecValPtrs.size(); ++ i)
		{
			bool thisNull = m_vecValPtrs[i] == NULL;
			bool tgtNull  = r.m_vecValPtrs[i] == NULL;
			if (tgtNull != thisNull)
			{
				fwprintf(stderr, L"%s is not null\n", tgtNull ? L"this": L"target");
				return false;
			}

			if (!tgtNull)
			{
				if (m_vecValPtrs[i]->eleNum != r.m_vecValPtrs[i]->eleNum)
				{
					fwprintf(stderr, L"entry %d inconsistent element number <%d vs %d>\n", i, 
						m_vecValPtrs[i]->eleNum, r.m_vecValPtrs[i]->eleNum);

					return false;
				}

				for (int k = 0; k < m_vecValPtrs[i]->eleNum; ++k)
				{
					if (m_vecValPtrs[i]->pOids[k] != r.m_vecValPtrs[i]->pOids[k])
					{
						fwprintf(stderr, L"inconsistent oid\n");
						return false;
					}

					if (m_vecValPtrs[i]->pVals[k] != r.m_vecValPtrs[i]->pVals[k])
					{
						fwprintf(stderr, L"inconsistent val\n");
						return false;
					}

					if (m_vecValPtrs[i]->pSumVals[k] != r.m_vecValPtrs[i]->pSumVals[k])
					{
						fwprintf(stderr, L"inconsistent sumVal\n");
						return false;
					}
				}
			}	//
		}	// for for (size_t i = 0; i < m_v

		return true;
	}

	bool Init()
	{
		m_vecValPtrs.reserve(s_initSize);
		return true;
	}

	bool EmptyFeatrue(int gfid)
	{
		if (gfid < (int)m_vecValPtrs.size())
			return m_vecValPtrs[gfid] == NULL;

		return false;
	}
	
	bool AllLess(int gfid, FLOAT_TYPE thres)
	{
		fprintf(stderr, "Warning, AllLesss unfinished\n");
		return true;
	}



	bool AllZero(int gfid)
	{
		if (gfid == -1 || gfid >= (int)m_vecValPtrs.size() || m_vecValPtrs[gfid] == NULL)
			return true;

		FLOAT_TYPE *pVal	= m_vecValPtrs[gfid]->pVals;
		FLOAT_TYPE *pSumVal = m_vecValPtrs[gfid]->pSumVals;
		OID_TYPE *pOid		= m_vecValPtrs[gfid]->pOids;
			
		for (int k = 0; k < m_vecValPtrs[gfid]->eleNum; ++k)
		{
			if ((*pVal) - (*pSumVal)/m_It != 0)
				return false;

			++pOid;
			++pVal;
			++pSumVal;
		}
		
		return true;
	};



	void Reset()
	{
		// only values are reset
		for (size_t i = 0; i < m_vecValPtrs.size(); ++i)
		{
			if (m_vecValPtrs[i] == NULL)
				continue;

			for (int k = 0; k < m_vecValPtrs[i]->eleNum; ++k)
			{
				m_vecValPtrs[i]->pVals[k] 	 = 0;
				m_vecValPtrs[i]->pSumVals[k] = 0;
			}
		}
		m_It = 0;
	}


	/* binary file format values only
	 * fid_1, oid_1, val, sumVal
	 * fid_1, oid_2, val, sumVal
	 * ....
	 */
	CSparseScorer(const CSparseScorer &r)
	{
		m_It			= r.m_It;
		m_avgMode		= r.m_avgMode;
		m_nOutcomeNum	= r.m_nOutcomeNum;
		m_insertMode	= r.m_insertMode;

		m_vecValPtrs.resize(r.m_vecValPtrs.size());
		for (size_t i = 0; i < r.m_vecValPtrs.size(); ++i)
		{
			if (r.m_vecValPtrs[i] != NULL)
			{
				OID_TYPE	*tmpOids = r.m_vecValPtrs[i]->pOids;
				FLOAT_TYPE	*pVs = r.m_vecValPtrs[i]->pVals;
				FLOAT_TYPE	*psumVs = r.m_vecValPtrs[i]->pSumVals;

				for (int k = 0; k < r.m_vecValPtrs[i]->eleNum; ++k)
				{
					if (k == 0)
						m_vecValPtrs[i] = ValAry:: MakeValAry(m_pool,  tmpOids[k], pVs[k], psumVs[k]);
					else
						m_vecValPtrs[i]->UpdateVal(m_pool, tmpOids[k], pVs[k], psumVs[k], true);
				}
			}
		}
	}



	bool CleanAndCopy(const CSparseScorer & r)
	{
		m_vecValPtrs.clear();
		m_pool.Recycle();

		m_It			= r.m_It;
		m_avgMode		= r.m_avgMode;
		m_nOutcomeNum	= r.m_nOutcomeNum; 
		
		m_vecValPtrs.resize(r.m_vecValPtrs.size(), NULL);

		for (size_t i = 0; i < r.m_vecValPtrs.size(); ++i)
		{
			if (r.m_vecValPtrs[i] != NULL)
			{
				OID_TYPE	*tmpOids = r.m_vecValPtrs[i]->pOids;
				FLOAT_TYPE	*pVs = r.m_vecValPtrs[i]->pVals;
				FLOAT_TYPE	*psumVs = r.m_vecValPtrs[i]->pSumVals;

				for (int k = 0; k < r.m_vecValPtrs[i]->eleNum; ++k)
				{
					if (k == 0)
						m_vecValPtrs[i] = ValAry::MakeValAry(m_pool,  tmpOids[k], pVs[k], psumVs[k]);
					else
						m_vecValPtrs[i]->UpdateVal(m_pool, tmpOids[k], pVs[k], psumVs[k], true);
				}
			}
		}
		return true;
	}



	bool Save(const string &strFile)
	{
		fprintf(stderr,"Saving model file %s...\n", strFile.c_str());
		FILE *fp = fopen(strFile.c_str(), "wb");
		CHK_OPEN_FAILED(fp, strFile.c_str());

		
		/* 1. write id map */
		size_t mapSize = m_vecValPtrs.size();
		fwrite(&mapSize,	sizeof(mapSize), 1, fp);
		fwrite(&m_It,		sizeof(m_It), 1, fp);
		fwrite(&m_nOutcomeNum,	sizeof(m_nOutcomeNum), 1, fp);
		
		
		/* 2. wirte values */
		const unsigned short zero = 0;
		for (size_t i = 0; i < mapSize; ++i)
		{
			if (m_vecValPtrs[i] == NULL)
				fwrite(&zero, sizeof(zero), 1, fp);
			else
			{
				unsigned short eleNum = m_vecValPtrs[i]->eleNum;

				fwrite(&m_vecValPtrs[i]->eleNum,	sizeof(m_vecValPtrs[i]->eleNum), 1, fp);
				fwrite(m_vecValPtrs[i]->pOids,		sizeof(OID_TYPE),	eleNum, fp);
				fwrite(m_vecValPtrs[i]->pVals,		sizeof(FLOAT_TYPE),	eleNum, fp);
				fwrite(m_vecValPtrs[i]->pSumVals,	sizeof(FLOAT_TYPE), eleNum, fp);
			}
		}

		fclose(fp);
		return true;
	}



	bool Load(const string &strFile)
	{
		FILE *fp = fopen(strFile.c_str(), "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Error: file %s open failed\n", 
							strFile.c_str());
			return false;
		}
		fprintf(stderr, "Loading scorer...");

		/* 1. read id map */
		size_t mapSize = 0;
		fread(&mapSize,	sizeof(mapSize), 1, fp);
		fread(&m_It,		sizeof(m_It), 1, fp);
		fread(&m_nOutcomeNum,	sizeof(m_nOutcomeNum), 1, fp);
		m_vecValPtrs.resize(mapSize, NULL);
		
		/* 2. read values */
		unsigned short eleNum = 0;
		OID_TYPE	pOids[500];
		FLOAT_TYPE	pVals[500];
		FLOAT_TYPE	pSumVals[500];
		for (size_t i = 0; i < mapSize; ++i)
		{
			fread(&eleNum, sizeof(eleNum), 1, fp);

			if (eleNum != 0)
			{
				fread(pOids,	sizeof(OID_TYPE),	eleNum, fp);
				fread(pVals,	sizeof(FLOAT_TYPE), eleNum, fp);
				fread(pSumVals, sizeof(FLOAT_TYPE), eleNum, fp);
				m_vecValPtrs[i] = ValAry::MakeValAry(m_pool, *pOids, *pVals, *pSumVals); 

				for (int k = 1; k < eleNum; ++k)
					m_vecValPtrs[i]->UpdateVal(m_pool,  pOids[k],  pVals[k],  pSumVals[k], true);
			}

			if (feof(fp))
				break;
		}
		fprintf(stderr, "Done");


		return true;
	}


	
	bool UpdatePredNum(size_t predNum)
	{
		fprintf(stderr, "Warning: UpdatePredNum unfinished\n");
		return true;
	}
	
	

	double GetVal(int gfid, int oId)
	{
		if (gfid == -1 || m_vecValPtrs[gfid] == NULL)
			return 0;

		ValAry *pAry = m_vecValPtrs[gfid];
		for (int i = 0; i < pAry->eleNum; ++ i)
		{
			if (pAry->pOids[i] == oId)
				return pAry->pVals[i] - pAry->pSumVals[i]/m_It;
		}
		return 0;
	}


	VAL_TYPE GetValType(int gfid)
	{
		fwprintf(stderr, L"GetValType(int gfid) unfinished\n");
		exit(0);
		return POS_ONLY;
	}

	
	FLOAT_TYPE GetAvgEn(int gfid)
	{
		fwprintf(stderr, L"GetAvgEn(int gfid) unfinished\n");
		exit(0);
		return 0;
	}


	
	bool TrainingWithAccEvent(SEvent *pGoldBeg,   
														SEvent *pGoldEnd,
								  					SEvent *pWinnerBeg, 
														SEvent *pWinnerEnd, 
														double rate)
	{
		if (pGoldBeg == NULL || pWinnerBeg == NULL)
			return false;
		
		SEvent *pGoldEvent = pGoldBeg;
		SEvent *pWinnerEvent = pWinnerBeg;
		
		for (; pWinnerEvent != pWinnerEnd ;)
		{
			UpdateParameter(pGoldEvent->FNum(), 	pGoldEvent->FIDS(), 
											pGoldEvent->OID(), 		rate);
			UpdateParameter(pWinnerEvent->FNum(), pWinnerEvent->FIDS(), 
											pWinnerEvent->OID(), 	-rate);
			pWinnerEvent = pWinnerEvent->m_pNext;
			pGoldEvent   = pGoldEvent->m_pNext;
		}
		return true;
	}



	
	


	bool UpdateParameter(int nPred, int *fids, OID_TYPE oid, FLOAT_TYPE learningRate)
	{
		for (int i = 0; i < nPred; ++i)
		{
			int gid = fids[i];
			if (gid == unkId)
				continue;

			if (gid < (int)m_vecValPtrs.size())
			{
				if (m_vecValPtrs[gid] == NULL)
				{
					if(m_insertMode == true)
						m_vecValPtrs[gid] = ValAry::MakeValAry(m_pool, oid, learningRate, m_It * learningRate);
				}
				else
					m_vecValPtrs[gid]->UpdateVal(m_pool, oid, learningRate, m_It * learningRate, m_insertMode);
			}
			else if (m_insertMode == true)
			{
				m_vecValPtrs.resize((gid + 1) * 1.5, NULL);
				m_vecValPtrs[gid] = ValAry::MakeValAry(m_pool, oid, learningRate, m_It * learningRate);
			}
		}
		return true;
	}
	

	
	bool UpdateParameter(int nPred,		int *goldFids,	size_t goldLabel,	int *guessFids, 
						 size_t guessLabel,		FLOAT_TYPE learningRate = 1.0)
	{
		// rewarding gold
		UpdateParameter(nPred,  goldFids, (OID_TYPE)goldLabel, learningRate);
		
		// discounting guess
		UpdateParameter(nPred, guessFids, (OID_TYPE)guessLabel, -learningRate);
		return true;
	}


	
	bool UpdateParameter(int nPredGold, int nPredWrong, int *goldFids, 
							size_t goldLabel, int *guessFids,  size_t guessLabel, FLOAT_TYPE learningRate = 1.0)
	{
		UpdateParameter(nPredGold, goldFids, (OID_TYPE)goldLabel, learningRate);
		UpdateParameter(nPredWrong, guessFids, (OID_TYPE)guessLabel, -learningRate);
		return true;
	}

	
	
	double Eval(int *PredIds, int nPred, int oid)
	{
		if (oid < 0 || oid >= (int)m_nOutcomeNum)
		{
			fprintf(stderr, "Error: outcome id out of range\n");
			exit(0);
		}

		double score = 0;
		for (int i = 0; i < nPred; ++i)
		{
			int gfid = PredIds[i];
			if (gfid == -1 || gfid >= (int)m_vecValPtrs.size() || m_vecValPtrs[gfid] == NULL)
				continue;

			
			int eleNum = m_vecValPtrs[gfid]->eleNum;
			FLOAT_TYPE *pVal	= m_vecValPtrs[gfid]->pVals;
			FLOAT_TYPE *pSumVal = m_vecValPtrs[gfid]->pSumVals;
			OID_TYPE *pOid		= m_vecValPtrs[gfid]->pOids;
			
			for (int k = 0; k < eleNum; ++k)
			{
				if (*pOid == oid)
					score += m_avgMode == true ? ((*pVal) - (*pSumVal)/m_It) : *pVal;


				++pOid;
				++pVal;
				++pSumVal;
			}
		}

		return score;
	}

  

	
	size_t Eval(int *PredIds, int nPred, vector<double> & oScoreVec)
	{
		if (oScoreVec.size() != m_nOutcomeNum)
			oScoreVec.resize(m_nOutcomeNum);

		std::fill(oScoreVec.begin(), oScoreVec.end(), 0.0);

		for (int i = 0; i < nPred; ++i)
		{
			int gfid = PredIds[i];
			if (gfid == -1 || gfid >= (int)m_vecValPtrs.size() || m_vecValPtrs[gfid] == NULL)
				continue;


			int eleNum = m_vecValPtrs[gfid]->eleNum;
			FLOAT_TYPE *pVal	= m_vecValPtrs[gfid]->pVals;
			FLOAT_TYPE *pSumVal = m_vecValPtrs[gfid]->pSumVals;
			OID_TYPE *pOid		= m_vecValPtrs[gfid]->pOids;
			for (int k = 0; k < eleNum; ++k)
			{
				oScoreVec[*pOid++] += m_avgMode == true ? 
										((*pVal++) - (*pSumVal++)/m_It) : *pVal++;
			}
		}

		double maxScore = -1.7e200;
		size_t bestId = (size_t) - 1;
		for (size_t i = 0; i < oScoreVec.size(); ++i)
			if (maxScore < oScoreVec[i])
			{
				maxScore = oScoreVec[i];
				bestId   = i;
			}
		return bestId;
	}
};

#endif  /*__SPARSE_SCORER_H__*/
