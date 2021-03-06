#ifndef __FEATURE_COLLECTOR_H__
#define __FEATURE_COLLECTOR_H__
#include <map>
#include <vector>
#include <algorithm>

#include "DepTree.h"
#include "Pool.h"
#include "State.h"
#include "Template.h"

using std::map;
class CFeatureCollector
{
public:
	CFeatureCollector(bool insertMode);//,  bool useDis);
	~CFeatureCollector();

	bool ReadTemplate(const string & strFile);
	void ExtractKernels(vector<int> &vKernels);
	void GetFeatures(vector<int> &vFIDs);
	bool SaveTemplateFile(const string &strPath);
	bool SaveFeatures(const string &strPath);
	bool LoadFeatures(const string &strPath);
	
	vector<_TPLT *> &GetTemps()									{return m_vTemps;}
	_SENTENCE * GetSen()												{return m_pSen;}
	bool GetMode()															{return m_iMode;}
	bool Verbose()															{return m_verbose;}
	void SetSen(_SENTENCE *pSen)								{m_pSen = pSen, m_nSenLen = pSen->Length();}
	void SetState(CState *pStat)								{m_pState = pStat;}
	void SetMode(bool insertMode)								{m_iMode = insertMode;	}
	void SetVerbose(bool v)											{m_verbose = v;}
	void ResetMem()															{m_pool.Recycle();}


private:
	map<string, int>  m_mapK2idx;  // kernel to kernelID
	vector<string>		m_kerVec;	   // kernel feature vector
	vector<_TPLT*>		m_vTemps;
	
	bool							m_iMode;     // insert mode
	bool							m_verbose;
	int 							m_nSenLen;
	int 							m_nKernel;
	int								m_gID;
	CPool		  				m_pool;
	_SENTENCE					*m_pSen;
	CState		  			*m_pState;

	static const int MAX_BUF_LEN = 256;
	static const wchar_t *pNull, *pSigEnd;
};
#endif  /*__FEATURE_COLLECTOR_H__*/
