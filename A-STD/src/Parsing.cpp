#include "IO.h"
#include "Eval.h"
#include "SRParser.h"
bool LoadingModel(const char *pszModel, 
          				CFeatureCollector &fExtor,
				  				CSparseScorer &scorer)
{
	if (pszModel == NULL)
		return false;

	string modelPath	= pszModel;
	string modelFile	= modelPath + ".model";
	string featureFile	= modelPath + ".feat";
	string templateFile = modelPath + ".temp";
	string nbFile   = modelPath + ".nb";
	string dLabel   = modelPath + ".dlabel";

	fprintf(stderr, "Initializing....");
	CIDMap::ReadDepLabelFile(dLabel);

	// loading model
	if (scorer.Load(modelFile.c_str()) == false)
		return false;

	// loading template
	if (fExtor.ReadTemplate(templateFile.c_str()) == false)	// load template file
		return false;

	// loading features
	if (fExtor.LoadFeatures(featureFile.c_str()) == false)
		return false;

	SSenNode::LoadNumberer(nbFile);
	SSenNode::InitPuncIDSet();
	return true;
}



SEvalRec Eval(vector<SSentence *> &vSen,  
							vector<CDepTree *> &vTree,  
							vector<vector<int> > &hIdxVec);


bool Parsing(CSRParser &rPar,  
						 vector<SSentence *> &vSen, 
						 vector<vector<int> > &hidxVec) {
	bool verbose = false;
	if (vSen.size() == 0)
		return false;

	hidxVec.resize(vSen.size());
	clock_t start = clock();
	int totalLen = 0;
	int senId = 0;

	for (SSentence *pSen: vSen) {
		totalLen += pSen->Length();
    CDepTree *pTree = nullptr;
    if (CConfig::nBS == 1) {
      pTree = rPar.ParsingGreedy(pSen);
    } else {
      pTree = rPar.Parsing(pSen);
    }

		HIndexes(pTree, hidxVec[senId ++]);
		if (verbose == true) {
			pTree->DisplayTreeStructure(stderr, 0);
			for (int i = 0; i < (int)hidxVec[senId-1].size(); ++i)
				fprintf(stderr, "%d, %d\n", i, hidxVec[senId - 1][i]);
		}
		if (senId % 100 == 0)
			fprintf(stderr, "Parsing %d sen\r", senId);
	}

	double avgLen	= 1.0 * totalLen / senId;
	double secs		= 1.0 * (clock() - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "Parsing %d sen, average length %.2f, cost %.2f second, %.2f sens/sec\n",
			senId,  avgLen,  secs,  senId/secs);

	return true;
}

SEvalRec ParsingAndEval(CSRParser &rPar, 
												vector<SSentence *> &vSen,  
												vector<CDepTree *> &vTree)
{
	vector<vector<int>> resHidx;
	if (Parsing(rPar, vSen, resHidx) == true)
		return Eval(vSen, vTree, resHidx);
	else
	{
		fprintf(stderr, "Parsing failed\n");
		exit(0);
	}
}

bool Parsing(const char *pszModel, 
						 const char *pszIn, 
						 const char *pszOut)
{
	string debugPath = string(pszOut) + ".dbg";
	FILE *fpDBG = fopen(debugPath.c_str(), "w");
	FILE *fpRes = fopen(pszOut, "w");
	if (fpDBG == NULL || fpRes == NULL)
	{
		fprintf(stderr, "Error: open %s failed\n", pszOut);
		return false;
	}
	
	string configFile   = string(pszModel) + ".confg";
	CConfig::LoadConfig(configFile);
	
	// initializing
	bool insertMode	= false;
	CFeatureCollector fExtor(insertMode);
	CSRParser parser(CConfig::nBS, insertMode, &fExtor);	
	if (LoadingModel(pszModel, fExtor, *parser.GetScorer()) == false)
		return false;
	
	// loading sentences
	CPool senPool;
	List<CDepTree*>::SetPool(&senPool);
	vector<SSentence *> senVec;// = CReader::ReadSentences(pszIn, senPool);
	vector<CDepTree *> vtTest = CReader::ReadTrees(pszIn,	senPool,  
																									&senVec, false); 

	// parsing
	CPool treePool;
	List<CDepTree *>::SetPool(&treePool);
	clock_t start = clock();
	vector<bool> wrong;
	vector<int> hPred, hRef;
	int nTotal = 0, nCorr = 0, nIn = 0, nInAcc = 0;
	int nBound = 0, nBoundCorr = 0;
	for(size_t i = 0; i < senVec.size(); ++i)
	{
		if (i != 0 && i % 100 == 0)
			fprintf(stderr, "Parsing %lu sen\r", i);

		wrong.clear();
		wrong.resize(senVec[i]->Length(), false);
		CDepTree *pRes = parser.Parsing(senVec[i]);
		HIndexes(vtTest[i], hRef);
		HIndexes(pRes, hPred);

		int nCorrCurr = 0;
		for (size_t k = 0; k < hPred.size(); ++k)
		{
			if (!SSenNode::IsPunc(senVec[i]->TID(k)))
			{
				wrong[k] = hRef[k] != hPred[k];
				nCorrCurr += hRef[k] == hPred[k];
				nTotal++;
			}
		}
		nCorr += nCorrCurr;
		fprintf(fpDBG, "sentence %lu %d\nRef:\n", i + 1, nCorrCurr);
		vtTest[i]->DisplayTreeStructure(fpDBG, 0, &CIDMap::GetDepLabelVec());

		// compute in boundary accuracy
		int nInTotal = 0, nInCorr = 0;
		InBoundaryAcc(senVec[i], vtTest[i], pRes, nInTotal, nInCorr, fpDBG);
		nIn += nInTotal;
		nInAcc += nInCorr;
		std::pair<int, int> bound = BoundarySenAcc(senVec[i], vtTest[i], pRes);
		nBound += bound.first, nBoundCorr += bound.second;
		
		fprintf(fpDBG, "\nPred:\n");
		pRes->DisplayTreeStructure(fpDBG, 0, 
																&CIDMap::GetDepLabelVec(), 0, &wrong);
		fprintf(fpDBG ,"\n\n");
		pRes->PrintTree(fpRes, &CIDMap::GetDepLabelVec()); 
		fprintf(fpRes, "\n\n");
	};

	double secs = 1.0*(clock() - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "Parsing %d sen, %.2f sen/sec\n", (int)senVec.size(), senVec.size()/secs);
	fprintf(stderr, "Total %d, corr %d, acc %.2f\n",
									nTotal, nCorr, 100.0*nCorr/nTotal);
	
	fprintf(stderr, "InTotal %d, Incorr %d, acc %.2f\n",
									 nIn, nInAcc, 100.0*nInAcc/nIn);
	
	fprintf(stderr, "BTotal %d, Bcorr %d, acc %.2f\n",
									 nBound, nBoundCorr, 
									 100.0*nBoundCorr/nBound);
	fclose(fpRes);
	fclose(fpDBG);
	return true;
}

bool ParsingHis(const char *pszModel, 
						 		const char *pszIn, 
						 		const char *pszOut)
{
	FILE *fpRes = fopen(pszOut, "w");
	if (fpRes == NULL)
	{
		fprintf(stderr, "Error: open %s failed\n", pszOut);
		return false;
	}
	
	string configFile   = string(pszModel) + ".confg";
	CConfig::LoadConfig(configFile);
	
	// initializing
	bool insertMode	= false;
	CFeatureCollector fExtor(insertMode);
	CSRParser parser(CConfig::nBS, insertMode, &fExtor);	
	if (LoadingModel(pszModel, fExtor, *parser.GetScorer()) == false)
		return false;

	// loading sentences
	CPool senPool;
	List<CDepTree*>::SetPool(&senPool);
	vector<SSentence *> senVec;
	vector<CDepTree *> vtTest = CReader::ReadTrees(pszIn,	senPool,  
																									&senVec, false); 

	// parsing
	CPool treePool;
	List<CDepTree *>::SetPool(&treePool);
	clock_t start = clock();
	for(size_t i = 0; i < senVec.size(); ++i)
	{
		if (i != 0 && i % 100 == 0)
			fprintf(stderr, "Parsing %lu sen\r", i);

		fprintf(fpRes,"\n\n sentence %d\n", (int) i + 1);
		vtTest[i]->DisplayTreeStructure(fpRes, 0, &CIDMap::GetDepLabelVec());
		parser.ParsingHis(senVec[i], vtTest[i], fpRes);
	};

	double secs = 1.0*(clock() - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "Parsing %d sen, %.2f sen/sec\n", (int)senVec.size(), senVec.size()/secs);
	return true;
}


