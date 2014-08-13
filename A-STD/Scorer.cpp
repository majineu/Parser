#include <cmath>
#include <cfloat>
#include <iostream>
#include <fstream>
#include <cassert>
#include "Scorer.h"
#include "Macros.h"
#include "code_conversion.h"
inline bool CheckedRead(size_t count, void * buf, size_t size, FILE *fp, const char * msg)
{
	if (fread(buf, size, count, fp) != count)
	{
		printf("%s\n", msg);
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Perceptron
{


CPerceptronScorer::~CPerceptronScorer()
{
	if (m_pParams != NULL)
		delete[] m_pParams;
	m_pParams = NULL;

	if (m_pSumParams != NULL)
		delete[] m_pSumParams;
	m_pSumParams = NULL;

	if (m_nUps != NULL)
		delete[] m_nUps;
	m_nUps = NULL;
}


CPerceptronScorer::CPerceptronScorer()
{
	m_pSumParams = m_pParams = NULL; 
	m_nUps = NULL;
	m_nParamNum = m_nOutcomeNum = m_nPredNum = 0; 
	correctNum = totalNum = 0;
	m_It = 1.0;
	m_nDelay = 0;
}
bool CPerceptronScorer::Load(FILE *fp)
{
	assert(fp != NULL);
	
	/* load feature vector */
	fread((void*)&m_nParamNum,  sizeof(size_t),  1, fp);
	if (m_nParamNum != m_nOutcomeNum * m_nPredNum)
	{
		fprintf(stderr, "Error: loading model failed\n");
		return false;
	}
	m_pParams = new double[m_nParamNum];
	CHK_NEW_FAILED(m_pParams);
	if (fread(m_pParams,  sizeof(double),  m_nParamNum,  fp) != m_nParamNum)
	{
		fprintf(stderr, "Error: loading model failed\n");
		return false;
	}
	std::cerr<< "predNum: "<< m_nPredNum<< " outcomeNum: "<<m_nOutcomeNum<<" paraNum: "<<m_nParamNum<<std::endl;
	return true;
}

bool CPerceptronScorer::Load(const char * pszModel)
{
	FILE *fp = fopen(pszModel, "rb");
	CHK_OPEN_FAILED(fp, pszModel);


	/* load feature vector */
	fread((void*)&m_nParamNum,  sizeof(size_t),  1, fp);
	if (m_nParamNum != m_nOutcomeNum * m_nPredNum)
	{
		fprintf(stderr, "Error: loading model failed\n");
		return false;
	}

	m_pParams = new double[m_nParamNum];
	CHK_NEW_FAILED(m_pParams);
	if (fread(m_pParams,  sizeof(double),  m_nParamNum,  fp) != m_nParamNum)
	{
		fprintf(stderr, "Error: loading model failed\n");
		return false;
	}
	fclose(fp);
	std::cerr<< "predNum: "<< m_nPredNum<< " outcomeNum: "<<m_nOutcomeNum<<" paraNum: "<<m_nParamNum<<std::endl;
	return true;
}


bool CPerceptronScorer::Save(FILE *fp)
{
	
	/* write parameters */
	fwrite((void*)&m_nParamNum, sizeof(size_t), 1, fp);
	fwrite((void*)m_pParams, sizeof(double), m_nParamNum, fp);

	return true;
}



bool CPerceptronScorer::Save(const char * pszModel)
{
	FILE *fp = fopen(pszModel, "wb");
	CHK_OPEN_FAILED(fp, pszModel);


	/* write parameters */
	fwrite((void*)&m_nParamNum, sizeof(size_t), 1, fp);
	fwrite((void*)m_pParams, sizeof(double), m_nParamNum, fp);

	fclose(fp);
	return true;
}


bool CPerceptronScorer::
TrainingWithAccEvent(	SSimpleEvent *pGoldBeg,   
											SSimpleEvent *pGoldEnd,
					  					SSimpleEvent *pWinnerBeg, 
											SSimpleEvent *pWinnerEnd, double rate)
{
	if (pGoldBeg == NULL || pWinnerBeg == NULL)
		return false;
	
	size_t featureId = 0, baseId = 0;
	SSimpleEvent *pGoldEvent = pGoldBeg, *pWinnerEvent = pWinnerBeg;
	
	for (; pWinnerEvent != pWinnerEnd ;)
	{
		baseId = pGoldEvent->oId * m_nPredNum;
		for (size_t k = 0; k < pGoldEvent->size; ++k)
		{
			featureId = baseId + pGoldEvent->pPred[k];

			if (m_nUps[featureId] < m_nDelay)
				++ m_nUps[featureId];
			else
			{
				m_pParams[featureId] += rate;
				m_pSumParams[featureId] += m_It * rate;
			}
		}

		baseId = pWinnerEvent->oId * m_nPredNum;
		for(size_t k = 0; k < pWinnerEvent->size; ++k)
		{
			featureId = baseId + pWinnerEvent->pPred[k];

			if (m_nUps[featureId] < m_nDelay)
				++m_nUps[featureId];
			else
			{
				m_pParams[featureId] -= rate;
				m_pSumParams[featureId] -= m_It * rate;
			}
		}

		pGoldEvent = pGoldEvent->pNext;
		pWinnerEvent = pWinnerEvent->pNext;
	}
	return true;
}





size_t CPerceptronScorer::
Eval(int *predIds, int nPred, vector<double> &OutcomeScoreVec)
{
	if (OutcomeScoreVec.size() != m_nOutcomeNum)
		OutcomeScoreVec.resize(m_nOutcomeNum);

	std::fill(OutcomeScoreVec.begin(), OutcomeScoreVec.end(), 0);

	bool verbose = false;

	size_t baseFId = 0;
	size_t bestId = size_t(-1);
	double maxScore = MIN_SCORE;
	
	for (size_t oId = 0; oId < m_nOutcomeNum; ++oId)
	{	
		/* 1. compute score for the current label */
		double curScore = 0;
		for (int k = 0; k < nPred; ++ k)
		{
			size_t featureId = baseFId + predIds[k];
			curScore += m_pParams[featureId];

			if (verbose == true)
			{
				std::cerr<<"baseId "<<baseFId<<" predId "<<predIds[k] << " featureId "<<
					featureId<<" fValue "<<m_pParams[featureId]<<std::endl;
			}
		}

		/* 2. update maxScore */
		if (maxScore < (OutcomeScoreVec[oId] = curScore))
		{
			maxScore = OutcomeScoreVec[oId];
			bestId = oId;
		}

		/* 3. update base feature Id, reset curScore */
		baseFId += m_nPredNum;
	}

	if (verbose == true)
	{
		for(size_t i = 0; i < OutcomeScoreVec.size(); ++i)
			fprintf(stderr, "%f  ", OutcomeScoreVec[i]);
		fprintf(stderr, "max %f\n", maxScore);
	}

	return bestId;
}

size_t CPerceptronScorer::
eval(const vector<size_t> & predIdVec, vector<double> & OutcomeScoreVec)
{
	size_t predNum = predIdVec.size();
	double maxScore = -1.7e308, curScore = 0.0;
	size_t baseFId = 0, bestOutcomeId = (size_t)-1, featureId;
	bool verbose = false;
	if(OutcomeScoreVec.size() != m_nOutcomeNum)
		OutcomeScoreVec.resize(m_nOutcomeNum);
	std::fill(OutcomeScoreVec.begin(), OutcomeScoreVec.end(), 0.0);

	for(size_t oId = 0; oId < m_nOutcomeNum; ++oId)
	{	/* 1. compute score for the current label */
		for(size_t it = 0; it < predNum; ++ it)
		{
			featureId = baseFId + predIdVec[ it ];
			curScore += m_pParams[ featureId ];

			if(verbose == true)
			{
				std::cerr<<"baseId "<<baseFId<<" predId "<<predIdVec[it] << " featureId "<<
					featureId<<" fValue "<<m_pParams[featureId]<<std::endl;
			}
		}

		/* 2. update maxScore */
		if(maxScore < (OutcomeScoreVec[oId] = curScore))
		{
			maxScore = OutcomeScoreVec[oId];
			bestOutcomeId = oId;
		}

		/* 3. update base feature Id, reset curScore */
		baseFId += m_nPredNum;
		curScore = 0.0;
	}

	if(verbose == true)
	{
		for(size_t i = 0; i < OutcomeScoreVec.size(); ++i)
			fprintf(stderr, "%f  ", OutcomeScoreVec[i]);
		fprintf(stderr, "max %f\n", maxScore);
	}

	return bestOutcomeId;
}




//----------------------------------------------------------------
bool CCompactor::
CompressingAll(CPerceptronScorer &rScorer, const char * pszModel,
				bool isAvg,		vector<std::string> & templates,  NCLP_INT32  beamSize)
{
	FILE *fp = fopen(pszModel, "wb");
	CHK_OPEN_FAILED(fp, pszModel);


	typedef unsigned int _OFFSET_TYPE;
	struct SHeader
	{
		_OFFSET_TYPE	m_bsOffset;
		_OFFSET_TYPE	m_depLabelOffset;
		_OFFSET_TYPE	m_templateOffset;
		_OFFSET_TYPE	m_valCountOffset;
	}head;


	struct VAL_TYPES
	{
		unsigned char	oId;
		float			val;
		VAL_TYPES(unsigned char o, float v)
		{
			oId = o;
			val  = v;
		}

		VAL_TYPES()
		{
			oId = 0;
			val = 0;
		}
	};

	typedef std::pair<unsigned char, double> PARAM_T;
	int nPred = CIDMap::GetPredNum();
	int newPredId = 0;
	vector<VAL_TYPES> nonZeroVec;

	char pszBuf[100]; 
	/* 1 count depLabel size */

	size_t offset = sizeof(SHeader);
	head.m_bsOffset = offset;
	
	offset += sizeof(NCLP_INT32);
	head.m_depLabelOffset = offset;


	vector<wstring *> depLabels = CIDMap::GetDepLabelVec();
	size_t depLabelSize = 0;
	for (size_t i = 0; i < depLabels.size(); ++i)
		depLabelSize += depLabels[i]->length() + 1;

	offset += depLabelSize;
	head.m_templateOffset = offset;

	/* 2 count template size */
	size_t templateSize = 0;
	for (size_t i = 0; i < templates.size(); ++i)
		templateSize += templates[i].length() +1;

	offset += templateSize;
	head.m_valCountOffset = offset;

	
	/* 1. count newPredId */
	vector<unsigned char> counts;
	for (int i = 0; i < nPred; ++i)
	{
		int nVal = 0;
		for (int o = 0; o < (int)CIDMap::GetOutcomeNum(); ++o)
		{
			double val = rScorer.GetVal(i, o, isAvg);
			if (val < -1.0e-4 || val > 1.0e-4)
			{
				++ newPredId;
				++nVal;
			}
		}

		if (nVal != 0)
			counts.push_back(nVal);
	}

	/* write head */
	size_t headSize = sizeof(head);
	fwrite(&head, headSize, 1, fp);
	fwrite(&beamSize, sizeof(beamSize), 1, fp);

	/* write depLabel */
	char buf[1000];
	for (size_t i = 0; i < depLabels.size(); ++i)
	{
		memset(buf, 0, sizeof(buf));

		string str = CCodeConversion::UnicodeToUTF8(*depLabels[i]);
		fwrite(str.c_str(), str.size() + 1, 1, fp);

		//wcstombs(buf, depLabels[i]->c_str(), depLabels[i]->size() * 2);  
		//fwrite(buf, strlen(buf) + 1, 1, fp);
	}


	/* write templates */
	for (size_t i = 0; i < templates.size(); ++i)
		fwrite(templates[i].c_str(), templates[i].length() + 1, 1, fp);
	

	/* write feature counts */
	NCLP_INT32  nFeatureNum = (NCLP_INT32)counts.size();
	fwrite(&nFeatureNum, sizeof(nFeatureNum), 1, fp);

	NCLP_INT32  nOutcomeNum = (NCLP_INT32)CIDMap::GetOutcomeNum();
	fwrite(&nOutcomeNum, sizeof(nOutcomeNum), 1, fp);

	if (nFeatureNum == 0)
	{
		fprintf(stderr, "Error: all features removed?\n");
		system("pause");
		exit(0);
	}

	fwrite(&counts[0], sizeof(counts[0]), counts.size(), fp);


	int nWrite = 0;
	for (int i = 0; i < nPred; ++i)
	{
		nonZeroVec.clear();
		for (int o = 0; o < (int)CIDMap::GetOutcomeNum(); ++o)
		{
			double val = rScorer.GetVal(i, o, isAvg);
			if (val < -1.0e-4 || val > 1.0e-4)
				nonZeroVec.push_back(VAL_TYPES(o, val));
		}

		if (nonZeroVec.size() > 0)
		{
			if (nonZeroVec.size() != counts[nWrite++])
			{
				fprintf(stderr, "Error: number of parameters inconsistent!\n");
				system("pause");
				exit(0);
			}

			const wchar_t * pwzFeature = CIDMap::GetPredict(i); 
			
			memset(pszBuf, 0, 100);
			string str = CCodeConversion::UnicodeToUTF8(wstring(pwzFeature));
			fwrite(str.c_str(), str.size() + 1, 1, fp);

			//wcstombs(pszBuf, pwzFeature, wcslen(pwzFeature) * 2);
			//fwrite(pszBuf, sizeof(char), strlen(pszBuf) + 1, fp);
			
			fwrite(&nonZeroVec[0],  sizeof(VAL_TYPES),  nonZeroVec.size(),  fp);
		}
	}

	fclose(fp);
	return true;
}


bool CCompactor::
Compressing(CPerceptronScorer &rScorer, const char *pszModel, bool isAvg)
{
	FILE *fp = fopen(pszModel, "wb");
	CHK_OPEN_FAILED(fp, pszModel);

	typedef std::pair<unsigned char, double> PARAM_T;
	int nPred = CIDMap::GetPredNum();
	int newPredId = 0;
	vector<PARAM_T> nonZeroVec;

	char pszBuf[100]; 
	double sizeReduction = 0;

	/* 1. count newPredId */
	vector<unsigned char> counts;
	for (int i = 0; i < nPred; ++i)
	{
		int nVal = 0;
		for (int o = 0; o < (int)CIDMap::GetOutcomeNum(); ++o)
		{
			double val = rScorer.GetVal(i, o, isAvg);
			if (val < -1.0e-4 || val > 1.0e-4)
				++nVal;
		}

		if (nVal != 0)
			counts.push_back(nVal);
	}

	/* write pred number */
	fwrite(&newPredId, sizeof(int), 1, fp);
	fwprintf(stderr , L"Old predNum %d, New predNum %d, pruned %d\n", 
					(int)CIDMap::GetPredNum(),  newPredId,  (int)CIDMap::GetPredNum()-newPredId);

	/* 2. do the real jop */
	for (int i = 0; i < nPred; ++i)
	{
		nonZeroVec.clear();
		for (int o = 0; o < (int)CIDMap::GetOutcomeNum(); ++o)
		{
			double val = rScorer.GetVal(i, o, isAvg);
			if (val < -1.0e-4 || val > 1.0e-4)
				nonZeroVec.push_back(PARAM_T(o, val));
		}

		sizeReduction += sizeof(float) * (CIDMap::GetOutcomeNum() - nonZeroVec.size());
		sizeReduction -= sizeof(unsigned char) * nonZeroVec.size();

		if (nonZeroVec.size() > 0)
		{
			const wchar_t * pwzFeature = CIDMap::GetPredict(i); 
			
			memset(pszBuf, 0, 100);
			wcstombs(pszBuf, pwzFeature, wcslen(pwzFeature) * 2);

			/* writing features */
			int len = strlen(pszBuf);
			fwrite(&len, sizeof(int), 1, fp);
			fwrite(pszBuf, sizeof(char), len, fp);
			
			unsigned char nVal = (unsigned char)nonZeroVec.size();
			fwrite(&nVal, sizeof(unsigned char), 1, fp);

			for (int k = 0; k < nVal; ++k)
			{
				unsigned char oid = nonZeroVec[k].first;
				float val = (float)nonZeroVec[k].second;

				fwrite(&oid, sizeof(unsigned char), 1, fp);
				fwrite(&val, sizeof(float), 1, fp);
			}
		}
	}

	sizeReduction -= newPredId * sizeof(unsigned char);
	size_t oldSize = CIDMap::GetOutcomeNum() * CIDMap::GetPredNum() * sizeof(float);

	fwprintf(stderr, L"oldSize %.2fm, total size reduction %.2fm  %.3f\n", oldSize/(float)(1024*1024),
			sizeReduction/(float)(1024 * 1024),  sizeReduction/(float)oldSize);
	fclose(fp);
	return true;
}
}
