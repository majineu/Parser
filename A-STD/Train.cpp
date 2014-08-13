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
 	random_shuffle(resVec.begin(), resVec.end());
  
	return resVec;
}

string BuildPath()
{
	string path = CConfig::strPrefix;
	string trainSubPath = CConfig::strTrainPath.substr(CConfig::strTrainPath.rfind("/") + 1);
	
	if (trainSubPath.find("verysmall") != string::npos)
		path += string("vSmall_");
	else if (trainSubPath.find("small") != string::npos)
		path += string("small_");
	else
		path += string("full_");

	char buf[100];
	string tempSubPath = CConfig::strTempPath.substr(CConfig::strTempPath.rfind("/") + 1);
	path += tempSubPath + string(".");

	for (size_t i = 0; i < CConfig::vBoundaryPairs.size(); ++i)
		path += CConfig::vBoundaryPairs[i];
	
	if (CConfig::bAddNonSepPunc)
		path += "_addNonSep";
	if (CConfig::bIgnoreLastPunc)
		path += "_ignoreLastPunc";
	if (CConfig::bRmNonPunc)
		path += "_rmNonPuncFeat";
	
	sprintf(buf, "_bs%d_rt%d", CConfig::nBS, CConfig::nRetry);
	path += buf;
	return path;
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
		throw("Reading config file failed\n");


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
	{
		fprintf(stderr, "Reading test set\n");
		vtDev   = CReader::ReadTrees(CConfig::strDevPath.c_str(), goldPool, &vsDev);
	}

	if (CConfig::strTestPath != CConfig::strNull)
	{
		fprintf(stderr, "Reading dev set\n");
		vtTest  = CReader::ReadTrees(CConfig::strTestPath.c_str(), goldPool, &vsTest);
	}

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
		throw ("Error: reading feature template failed\n");
	
	double bestDevAcc = 0, bestTestAccByDev	= 0, bestTestAcc = 0; 
	int	bestDevIter = 0,   bestTestIter = 0;
	
	string path		= BuildPath();
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
//		parser.SetIMode(true);
//		parser.GetScorer()->SetAvgMode(false);
		int nFail = 0;
		for (size_t i = 0; i < randomIds.size(); ++ i)
		{
			//	parser.SetVerbose(true);
			int senId	= randomIds[i];
			bool succ = parser.Training(vsTrain[senId], vtTrain[senId]);
//			bool succ = parser.Checking(vsTrain[senId], vtTrain[senId]);
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
		fprintf(stderr, "%lu sen, ups %d, early ups %d, pu ups %d,  failed %d\r", 
										randomIds.size(),		statis.m_nUp, 	statis.m_nEarly,  statis.m_nPuUps,  nFail);

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

	// save model file 
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
