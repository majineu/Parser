#pragma once
#include <string>
#include <set>
#include <vector>
using std::string;
using std::wstring;
using std::vector;

class CConfig
{
public:
	static int		nRound;
	static int		nRetry;		
	static int		nGoldBS;
	static int		nRandFactor;
	static int		nStandIter;

	enum  ACTION_TYPE{LEFT = 0,  RIGHT};
	enum  UPDATE_STRATEGY{TOP_ONE, ALL};
	enum  DY_ORACLE_NEXT_BEAM{GOLD_TOP,  GOLD_SURVIVE,  NON_ZERO};
	enum  UPDATE_T{PER_SEN,  PER_ITER};

	static DY_ORACLE_NEXT_BEAM  dyNextBeam;  
	static UPDATE_STRATEGY	us;
	static UPDATE_T			ut;
	const static int PARSING_ACTION_NUM = 3;
	
	
	static bool		bGoldBeam;
	static bool		bRareFlag;
	static bool		bRandGold;			// randomly generate a gold action
	static bool		bLen1WordEndWord;
	static bool		bDisHis;
	static bool		bDyOracle;
	static bool		bSearn;	
	static bool		bMergeEq;			  // merge eq analysis
	static bool		bResetGold;			// reset gold analysis with beam top
  static bool   bPunc;
  static bool   bEng; 
  static bool   bInsertParentRelate;
  static bool   bBackoffDis;    // whether backoff distance features  
	static bool		bDropout;
	static bool		bPVerbose;
	static bool		bFVerbose;
	static bool		bNN;
	static bool		bAvgNN;
	static bool		bIgnoreLastPunc;
	static bool		bRmNonPunc;
	static bool 	bAddNonSepPunc;

	static string	strNull;
	static string	strTrainPath;
	static string	strDevPath;
	static string	strTestPath;
	static string	strTempPath;
	static string	strModelPath;
	static string	strTagLabelPath;
	static string	strPrefix;
  static string strLogDir;
  static string strDropoutType;			// Base, Annaling and other types in the futher
  static std::set<wstring> puncsSet;

	static double fSchedule;					// for annaling scorer  
	static double fRate;
	static double fMargin;
	// configuration for decoding
	static int		nLowCount;			// low count
	static int		nRareWord;			// rare word
	static int		nCutoff;			// n cutoff
	static int		nBS;				// beam size
  static int    nDisThres;

	static vector<string> vHiddenType;
	static vector<int>		vHiddenSize;
	static vector<string> vBoundaryPairs;
public:
	static bool SaveConfig(const string & strPath);
	static bool LoadConfig(const string & strPath);
	static bool ReadConfig(const char * pszConfigPath);
  static bool IsPunc(const wchar_t *pwzKey);
	static string BuildPath();
};

//--------------------------------------------------------------------------



