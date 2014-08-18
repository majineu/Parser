#include "Macros.h"
#include "Config.h"
#include "util.h"

/* static members of class config */
int		CConfig::nRound				= 26;
int   CConfig::nDisThres    = 1000;
int		CConfig::nRareWord			= 30;
int		CConfig::nCutoff			= 6;
int		CConfig::nBS				= 1;			// by default, deterministic tagger
int		CConfig::nRetry				= 3;
int		CConfig::nGoldBS			= 1;
int		CConfig::nLowCount			= -1;			// by default, now low frequency feature
int		CConfig::nRandFactor		= 10;			// default
int		CConfig::nStandIter			= 4;			// when searn is set on, the first n iterations still performs standard training


bool	CConfig::bGoldBeam			= false;		// by default, we do not us gold beam
bool	CConfig::bRareFlag			= false;		// zhang's template should set false
bool	CConfig::bLen1WordEndWord	= true;			// zhang's template should set false
bool	CConfig::bDisHis			= false;		// for debuging purpose
bool	CConfig::bDyOracle			= false;		// dynamic oracle
bool	CConfig::bSearn				= false;		// dynamic searn
bool	CConfig::bMergeEq			= true;
bool	CConfig::bResetGold			= true;
bool	CConfig::bRandGold			= false;
bool  CConfig::bPunc        = false;
bool  CConfig::bEng           = true;
bool  CConfig::bInsertParentRelate = false;
bool  CConfig::bBackoffDis    = false;
bool  CConfig::bFVerbose    = false;
bool  CConfig::bPVerbose				= false;
bool  CConfig::bDropout				= false;
bool	CConfig::bNN						= false;
bool	CConfig::bAvgNN					= false;
bool	CConfig::bAddNonSepPunc	= false;
bool	CConfig::bRmNonPunc			= false;

bool	CConfig::bIgnoreLastPunc= false;
double CConfig::fMargin				= 1.0;
double CConfig::fRate					= 0.1;

string	CConfig:: strNull			= "null";
string	CConfig:: strTrainPath		= strNull;
string	CConfig:: strDevPath			= strNull;
string	CConfig:: strTestPath			= strNull;
string	CConfig:: strTempPath			= strNull;
string	CConfig:: strModelPath		= strNull;
string	CConfig:: strPrefix				= strNull;
string	CConfig:: strTagLabelPath	= strNull;
string  CConfig:: strLogDir     	= strNull;
string  CConfig:: strDropoutType	= strNull;

double 	CConfig:: fSchedule  = 0.5; 
vector<string> CConfig::vHiddenType;
vector<string> CConfig::vBoundaryPairs;
vector<int> CConfig::vHiddenSize;


wstring puncs[] = {L",", L".", L":",  L"``", L"''", L"-LRB-", L"-RRB-", L"PU"};
std::set<wstring> CConfig::puncsSet(puncs, puncs + sizeof(puncs)/sizeof(*puncs));

CConfig::UPDATE_STRATEGY CConfig::us		= CConfig::ALL;
CConfig::DY_ORACLE_NEXT_BEAM CConfig::dyNextBeam	= CConfig::GOLD_TOP;
CConfig::UPDATE_T CConfig::ut = CConfig::PER_SEN;

bool CConfig::SaveConfig(const string &strPath)
{
	FILE *fp = fopen(strPath.c_str(), "w");
	CHK_OPEN_FAILED(fp, strPath.c_str());

  fprintf(fp, "%d %d %d %d\n", nBS, 
					bAddNonSepPunc,
					bIgnoreLastPunc,
					bRmNonPunc);
	for (size_t i = 0; i < CConfig::vBoundaryPairs.size(); ++i)
		fprintf(fp, "%s\n", vBoundaryPairs[i].c_str());

	fclose(fp);
	return true;
}



bool CConfig::LoadConfig(const string &strPath)
{
	FILE *fp = fopen(strPath.c_str(), "r");
	CHK_OPEN_FAILED(fp, strPath.c_str());
	fprintf(stderr, "\n-----------------config------------------\n");
 	const int BUF_LEN = 1024;
	char buf[BUF_LEN];
	fgets(buf, BUF_LEN, fp);
	vector<char *> tokens = Split(buf, " \r\t\n");
	nBS = atoi(tokens[0]);
	if (tokens.size() > 1)
		bAddNonSepPunc = atoi(tokens[1]) > 0;
	else
		bAddNonSepPunc = false;

	bIgnoreLastPunc = tokens.size() > 2 ? (atoi(tokens[2]) > 0) : false;
	bRmNonPunc = tokens.size() > 3 ? (atoi(tokens[3]) > 0) : false;
	
	fprintf(stderr, "beam size %d, append non sep puncs %s\n", 
					nBS, bAddNonSepPunc ? "true": "false");
	fprintf(stderr, "ignore last punc %s\n",
					bIgnoreLastPunc ? "true":"false");
	
	fprintf(stderr, "rm non punc %s\n",
					bRmNonPunc ? "true":"false");
	
	fprintf(stderr, "boundarys:\n");
	while (fgets(buf, BUF_LEN, fp) != NULL)
	{
		removeNewLine(buf);
		vBoundaryPairs.push_back(buf);
		fprintf(stderr, "%s\n", vBoundaryPairs.back().c_str());
	}
	fprintf(stderr, "-----------------config------------------\n");
	return true;
}



bool CConfig::
ReadConfig(const char *pszPath)
{
	FILE *fpIn = fopen(pszPath, "r");
	CHK_OPEN_FAILED(fpIn, pszPath);
	
	const int BUF_LEN = 4096;
	char buf[BUF_LEN];
	while (fgets(buf, BUF_LEN, fpIn) != NULL)
	{
		if (buf [0] == ':' && buf [1] == ':')
			continue;
		
		removeNewLine(buf);
		if (strlen(buf) == 0)
			continue;


		char *pKey = strtok(buf, " \r\t\n");
		if (pKey == NULL)
		{
			fprintf(stderr, "Error: config file invalid format %s\n", buf);
			return false;
		}
		
		char *pVal = strtok(NULL, " \r\t\n");
		
		if (pVal == NULL)
		{
			fprintf(stderr, "Error: config file invalid format %s\n", buf);
			return false;
		}

		if (strcmp(pKey, "trainPath") == 0)
			CConfig::strTrainPath = pVal;
		
		else if (strcmp(pKey, "bFVerbose") == 0)
			CConfig::bFVerbose = string(pVal) == string("true");
		
		else if (strcmp(pKey, "bPVerbose") == 0)
			CConfig::bPVerbose = strcmp(pVal, "true") == 0;
		else if (strcmp(pKey, "bAddNonSepPunc") == 0)
		{
			CConfig::bAddNonSepPunc = strcmp(pVal, "true") == 0;
			fprintf(stderr, "Append non-sep punc %s\n", 
					CConfig::bAddNonSepPunc ? "true":"false");
		}
		else if (strcmp(pKey, "bAvgNN") == 0)
		{
			CConfig::bAvgNN = strcmp(pVal, "true") == 0;
			fprintf(stderr, "NN test average mode %s\n", 
					CConfig::bAvgNN ? "true":"false");
		}

		else if (strcmp(pKey, "vBoundaryPairs") == 0)
		{
			char *pToken = pVal;
			vector<char *> pairs(1, pVal);
			while ((pToken = strtok(NULL, " \t\r\n")) != NULL)
				pairs.push_back(pToken);
			vBoundaryPairs.insert(vBoundaryPairs.begin(), 
														pairs.begin(), pairs.end());
			fprintf(stderr, "Boundary punc pairs:");
			for (size_t i = 0; i < vBoundaryPairs.size(); i += 2)
				fprintf(stderr, "%s %s   ", 
						vBoundaryPairs[i].c_str(), vBoundaryPairs[i + 1].c_str());
			fprintf(stderr, "\n");
		}

		else if (strcmp(pKey, "bNN") == 0)
		{
			CConfig::bNN = strcmp(pVal, "true") == 0;
			fprintf(stderr, "NN scorer %s\n", CConfig::bNN ? "true":"false");
		}

		else if (strcmp(pKey, "bRmNonPunc") == 0)
		{
			CConfig::bRmNonPunc = strcmp(pVal, "true") == 0;
			fprintf(stderr, "rm non punc feature %s\n", 
							CConfig::bRmNonPunc ? "true":"false");
		}

		else if (strcmp(pKey, "fMargin") == 0)
		{
			CConfig::fMargin = atof(pVal);
			fprintf(stderr, "margin loss %.2f\n", CConfig::fMargin);
		}

		else if (strcmp(pKey, "fRate") == 0)
		{
			CConfig::fRate = atof(pVal);
			fprintf(stderr, "Learning rate %.2f\n", CConfig::fRate);
		}

		else if (strcmp(pKey, "bIgnoreLastPunc") == 0)
		{
			CConfig::bIgnoreLastPunc = strcmp(pVal, "true") == 0;
			fprintf(stderr, "Ignore last Punc %s\n",
					CConfig::bIgnoreLastPunc?"true":"false");
		}
		
		else if (strcmp(pKey, "HiddenSize") == 0)
		{
			char *pSizes = pVal; 
			CConfig::vHiddenSize.clear();
			while (pSizes != NULL)
			{
				CConfig::vHiddenSize.push_back(atoi(pSizes));
				pSizes = strtok(NULL, " \t\r\n");
			}
		}

		else if (strcmp(pKey, "HiddenType") == 0)
		{
			char *pType = pVal; 
			CConfig::vHiddenType.clear();
			while (pType != NULL)
			{
				CConfig::vHiddenType.push_back(pType);
				pType = strtok(NULL, " \t\r\n");
			}
		}

		else if (strcmp(pKey, "devPath") == 0)
			CConfig::strDevPath	= pVal;

		else if (strcmp(pKey, "updateT") == 0)
		{
			CConfig::ut = strcmp(pVal, "perSen") == 0 ? CConfig::PER_SEN : CConfig::PER_ITER;
			fprintf(stderr, "Inc t: %s\n", CConfig::ut == CConfig::PER_SEN ? "PerSen":"PerIter");
		}
		else if (strcmp(pKey, "modelPath") == 0)
			CConfig::strModelPath = pVal;

		else if (strcmp(pKey, "dropoutType") == 0)
		{
			CConfig::strDropoutType = pVal;
			fprintf(stderr, 	"dropout type: %s\n", 
							CConfig::strDropoutType.c_str());
		}

		else if (strcmp(pKey, "testPath") == 0)
			CConfig::strTestPath  = pVal;
		
		else if (strcmp(pKey, "templatePath") == 0)
			CConfig::strTempPath	= pVal;
		
		else if (strcmp(pKey, "mergeEq") == 0)
			CConfig::bMergeEq		= strcmp(pVal, "true") == 0;

		else if (strcmp(pKey, "resetGold") == 0)
			CConfig::bResetGold		= strcmp(pVal, "true") == 0;

		else if (strcmp(pKey, "fSchedule") == 0)
		{
			CConfig::fSchedule  = atof(pVal);
			fprintf(stderr, "Annaling dropout schedule %.2f\n", 
					CConfig::fSchedule);
		}

    else if (strcmp(pKey, "disThres") == 0)
    {
      CConfig::nDisThres = atoi(pVal);
      fprintf(stderr, "distance threshold :%d\n", CConfig::nDisThres);
    }

		else if (strcmp(pKey, "logDir") == 0)
    {
      CConfig::strLogDir = pVal;
      fprintf(stderr, "logDir: %s\n", CConfig::strLogDir.c_str());
    }

		else if (strcmp(pKey, "bDropout") == 0)
		{
			CConfig::bDropout = string(pVal) == string("true");
			fprintf(stderr, "using dropout %d\n", 
					CConfig::bDropout);
		}

    else if (strcmp(pKey, "backOffDis") == 0)
    {
      CConfig::bBackoffDis = strcmp(pVal, "true") == 0;
      fprintf(stderr, "back off distance features: %d\n", CConfig::bBackoffDis);
    }

    else if (strcmp(pKey, "insertParentRelate") == 0)
    {
      CConfig::bInsertParentRelate = strcmp(pVal, "true") == 0;
      fprintf(stderr, "insert parent Relate: %s", CConfig::bInsertParentRelate? "True":"False");
    }

		else if (strcmp(pKey, "nRound") == 0)
			CConfig::nRound		= atoi(pVal);
    else if (strcmp(pKey, "bPunc") == 0)
      CConfig::bPunc  = string(pVal) == "true";
		
		else if (strcmp(pKey, "nBeamSize") == 0)
			CConfig::nBS		= atoi(pVal);

		else if (strcmp(pKey, "bSearn") == 0)
			CConfig::bSearn		= strcmp(pVal, "true") == 0;

		else if (strcmp(pKey, "bDyOracle") == 0)
		{
			CConfig::bDyOracle	= strcmp(pVal, "true") == 0;
			fprintf(stderr, "%s %d\n", pKey, CConfig::bDyOracle);
		}

		else if (strcmp(pKey, "bGoldBeam") == 0)
			CConfig::bGoldBeam = (strcmp(pVal, "true") == 0);

    else if (strcmp(pKey, "bEng") == 0)
      CConfig::bEng = (strcmp(pVal, "true") == 0);

		else if (strcmp(pKey, "goldBeamSize") == 0)
			CConfig::nGoldBS	= atoi(pVal);

		else if (strcmp(pKey, "nRetry") == 0)
		{
			CConfig::nRetry = atoi(pVal);
			fprintf(stderr, "n retry %d\n", CConfig::nRetry);
		}

		else if (strcmp(pKey, "bRandGold") == 0)
		{
			CConfig::bRandGold = (strcmp(pVal, "true") == 0);
			fprintf(stderr, "random gold action :%s", CConfig::bRandGold?"true": "false");
		}

		else if (strcmp(pKey, "dyNextBeam") == 0)
		{
			if (strcmp(pVal, "GoldTop") == 0)
				CConfig::dyNextBeam = CConfig::GOLD_TOP;
			else if (strcmp(pVal, "GoldSurvive") == 0)
				CConfig::dyNextBeam = CConfig::GOLD_SURVIVE;
			else
			{
				fprintf(stderr, "Error: unknown next beam configure\n");
				exit(0);
			}
		}

		else if (strcmp(pKey, "prefix") == 0)
		{
			CConfig::strPrefix	= pVal;
			fprintf(stderr, "prefix: %s\n", pVal);
		}

		else 
		{
			fprintf(stderr, "Error: unknown configure parameter %s \n", pKey);
			return false;
		}
	}

	if (CConfig::strTrainPath == CConfig::strNull)
	{
		fprintf(stderr, "Error: Training file un-specified\n");
		return false;
	}

	if (CConfig::strModelPath == CConfig::strNull)
	{
		fprintf(stderr, "Error: Model file un-specified\n");
		return false;
	}

	if (CConfig::strTempPath == CConfig::strNull)
	{
		fprintf(stderr, "Error: Template file un-specified\n");
		return false;
	}

	fprintf(stderr, "\n----------------------------------------------------\n");
	fclose(fpIn);
	return true;
}



string CConfig::BuildPath()
{
	string path = CConfig::strPrefix;
	string trainSubPath = CConfig::strTrainPath.substr(CConfig::strTrainPath.rfind("/") + 1);
	
	if (trainSubPath.find("small") != string::npos)
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

//--------------------------------------------------------------------------------------------
