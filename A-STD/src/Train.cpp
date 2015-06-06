#include <cstdlib>
#include <algorithm>
#include <random>
#include "SRParser.h"
#include "Eval.h"
#include "IO.h"
typedef CDepTree _TREE;


vector<int> Shuffel(int nElems)
{
	vector<int> resVec;
	if (nElems == 0)
		return resVec;

	for (int i = 0; i < nElems; ++i)
		resVec.push_back(i);
 
	random_shuffle(resVec.begin(), resVec.end());
 //	random_shuffle(resVec.begin(), resVec.end());
	return resVec;
}



void DumpNoPuncTree(const char *path, vector<CDepTree *> &vTree)
{
	FILE *fp = fopen(path, "w");
	for (CDepTree *pTree: vTree)
	{
		pTree->PrintTree(fp, &CIDMap::GetDepLabelVec());
		fprintf(fp, "\n");
	}
	fclose(fp);
}



void Training(const char * pszCfg)
{
	if (CConfig::ReadConfig(pszCfg) == false)
	{
		fprintf(stderr, "Error: config file open failed\n");
		exit(0);
	}

	vector<SSentence *> vsTrain, vsDev, vsTest, vsTrainSub;
	vector<_TREE  *> vtTrain, vtDev, vtTest, vtTrainSub;

	CPool goldPool, trainingPool;
	List<CDepTree *>::SetPool(&goldPool);

	fprintf(stderr, "Reading training set\n");
	SSenNode::InitPuncIDSet();
	vtTrain = CReader::ReadTrees(CConfig::strTrainPath.c_str(),  goldPool,  &vsTrain); 
	for (size_t i = 0; i < vtTrain.size(); ++i)
	{
		fprintf(stdout, "\n\nsentence %d\n", (int)i + 1);
		vtTrain[i]->DisplayTreeStructure(stdout, 0, &CIDMap::GetDepLabelVec());
	}

	if (CConfig::strDevPath != CConfig::strNull)
		vtDev   = CReader::ReadTrees(CConfig::strDevPath.c_str(), goldPool, &vsDev);

	if (CConfig::strTestPath != CConfig::strNull)
		vtTest  = CReader::ReadTrees(CConfig::strTestPath.c_str(), goldPool, &vsTest);

	// extract training sub-set
	for (size_t i = 0; i < vsTrain.size(); i += 25)
	{
		vsTrainSub.push_back(vsTrain[i]);
		vtTrainSub.push_back(vtTrain[i]);
	}

//	DumpNoPuncTree("wsj02-21.malt.auto.nopunc", vtTrain);
//	DumpNoPuncTree("wsj22.malt.auto.nopunc", vtDev);
//	DumpNoPuncTree("wsj23.malt.auto.nopunc", vtTest);

	fprintf(stderr, "parsing verbose %d, feature verbose %d\n",
					CConfig::bPVerbose, CConfig::bFVerbose);
	
	bool iMode	 = false;
	CFeatureCollector fCollector(iMode);
	fCollector.SetVerbose(CConfig::bFVerbose);
	if (fCollector.ReadTemplate(CConfig::strTempPath) == false)
	{
		fprintf(stderr, "Error: Reading template failed\n");
		exit(0);
	}
	
	double bestDevAcc = 0, bestTestAccByDev	= 0, bestTestAcc = 0; 
	int	bestDevIter = 0,   bestTestIter = 0;
	
	string path		= CConfig::BuildPath();
  string logPath	= CConfig::strLogDir + "/" + path+ ".log";
	fprintf(stderr, "\npath: %s\n", path.c_str());
  fprintf(stderr, "logPath: %s\n", logPath.c_str());
 
	CSRParser parser(CConfig::nBS, true, &fCollector);
	parser.SetVerbose(CConfig::bPVerbose);
	parser.GetScorer()->SetOutcomeNum(CIDMap::GetOutcomeNum());
	FILE *fpLog = fopen(logPath.c_str(), "w");
	if (fpLog == NULL)
		fprintf(stderr, "Warning: Log file open failed\n");
	
	// OK, here we go...
	clock_t totalStart = 0;
//	CDepTree::SetPool(&trainingPool);
	List<CDepTree *>::SetPool(&trainingPool);
	for (int nIter = 0; nIter < CConfig::nRound; ++ nIter)
	{
		fprintf(stderr, "\n-----------------------Training %d round----------------------\n", nIter);
		vector<int> randomIds = Shuffel((int)vtTrain.size());
		
		clock_t start = clock();
		parser.ResetStatis();
		int nFail = 0;
		for (size_t i = 0; i < randomIds.size(); ++ i)
		{
			int senId	= randomIds[i];
			bool succ = parser.Training(vsTrain[senId], vtTrain[senId]);
			int nRetry = 0;
			while (succ == false && nRetry++ < CConfig::nRetry)
				succ = parser.Training(vsTrain[senId], vtTrain[senId]);
			if (i != 0 && i % 300 == 0)
			{
				SStatis statis = parser.Statis();
				fprintf(stderr, "%lu sen, ups %d, early ups %d, pu ups %d,  failed %d\r", 
								i,		statis.m_nUp, 	statis.m_nEarly,  statis.m_nPuUps,  nFail);
			}
		}

		double secs = 1.0 * (clock() - start)/CLOCKS_PER_SEC;
		fprintf(stderr, "%d sen, %.2f secs, ", (int)randomIds.size(), secs);

		SStatis statis = parser.Statis();
		fprintf(stderr, "%lu sen, ups %d, early ups %d, conflicts %d, conflict_shift %d,  failed %d\r", 
										randomIds.size(),		statis.m_nUp, 	statis.m_nEarly,  
                    statis.m_num_conflicts, statis.m_num_shift,  nFail);

		// evaluate gold sentences
		fprintf(stderr, "\nEvaluating on training set ....\n");
		SEvalRec recTrain = ParsingAndEval(parser, vsTrainSub, vtTrainSub);
		
		if (fpLog != NULL)
		{
			fprintf(fpLog, "Iteration %d:\nTraining Set:\n", nIter);
			recTrain.Display(fpLog);
		}

		// evaluate dev set
		if (CConfig::strDevPath != CConfig::strNull)
		{
			fprintf(stderr, "\nEvaluating on dev set ....\n");
			SEvalRec eRecDev = ParsingAndEval(parser, vsDev, vtDev);
			if (fpLog != NULL)
			{
				fprintf(fpLog, "Dev set:\n");
				eRecDev.Display(fpLog);
			}
			
			if (nIter > 10 && eRecDev.accuracy > bestDevAcc)
			{
				// save model file 
				bestDevAcc = eRecDev.accuracy;
				bestDevIter = nIter;
				string modelPath = CConfig::strModelPath + path + ".model"; 
				string confPath  = CConfig::strModelPath + path + ".confg";
				string tempPath  = CConfig::strModelPath + path + ".temp";
				string featPath  = CConfig::strModelPath + path + ".feat";
				string nbPath    = CConfig::strModelPath + path + ".nb";
				string dLabel    = CConfig::strModelPath + path + ".dlabel";
				CConfig::SaveConfig(confPath);
				parser.GetScorer()->Save(modelPath);
				fCollector.SaveTemplateFile(tempPath);
				fCollector.SaveFeatures(featPath);
				SSenNode::SaveNumberer(nbPath);
				CIDMap::SaveDepLabel(dLabel);
			}
		}
		

		// evaluate test set
		if (CConfig::strTestPath != CConfig::strNull)
		{
			fprintf(stderr, "\nEvaluating on test set ....\n");
			SEvalRec eRecTest = ParsingAndEval(parser, vsTest, vtTest);
			if (fpLog != NULL)
			{
				fprintf(fpLog, "Test set:\n");
				eRecTest.Display(fpLog);
				fprintf(fpLog, "\n");
				fflush(fpLog);
			}

			if (bestDevIter == nIter)
				bestTestAccByDev = eRecTest.accuracy;

			if (eRecTest.accuracy > bestTestAcc)
			{
				bestTestAcc = eRecTest.accuracy;
				bestTestIter= nIter;
			}
		}
	}

	fprintf(stderr, "\n-----------------------------------------\n");
	fprintf(stderr, "config path %s\n", path.c_str());
	fprintf(stderr, "\nbestDevIter %d, bestDevAcc %.2f%%, bestTestAccByDev %.2f%%\nbestTestIter %d, bestTestAcc %.2f%%\n",
					bestDevIter, bestDevAcc, bestTestAccByDev, bestTestIter, bestTestAcc);
	fprintf(stderr, "Training done, cost %.2f secs\n\n\n", 1.0*(clock() - totalStart)/CLOCKS_PER_SEC);


	double secs = (double)(clock() - totalStart)/CLOCKS_PER_SEC;
	if (fpLog != NULL)
	{
		fprintf(fpLog, "\nbestDevIter %d, bestDevAcc %.2f%%, bestTestAccByDev %.2f%%\nbestTestIter %d, bestTestAcc %.2f%%\n",
			bestDevIter, bestDevAcc, bestTestAccByDev, bestTestIter, bestTestAcc);
		fprintf(fpLog, "%.2f secs or %.2f hours eclipsed\n", secs, secs/3600);
		fclose(fpLog);
	}
}
