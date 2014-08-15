#include "FeatureCollector.h"

const wchar_t *CFeatureCollector::pNull = L"NUL";
const wchar_t *CFeatureCollector::pSigEnd = L"</s>";

CFeatureCollector::
CFeatureCollector(bool insertMode)//, bool useDis):
:m_insertMode(insertMode), m_nKernel(0), m_gID(10)
{
	m_verbose = false;
}

CFeatureCollector::~CFeatureCollector()
{
	for (size_t i = 0; i < m_vTemps.size(); ++i)
		delete m_vTemps[i];
}


bool CFeatureCollector::
ReadTemplate(const string & strFile)
{
	FILE * fp = fopen(strFile.c_str(), "r");
	CHK_OPEN_FAILED(fp, strFile.c_str());
	char buf[CFeatureCollector::MAX_BUF_LEN];
	bool kernelFeature = true;
	int  kFeatureId = 0;
	fprintf(stderr, "Reading feature template from %s\n", strFile.c_str());
	while (fgets(buf,  CFeatureCollector::MAX_BUF_LEN, fp))
	{
		removeNewLine(buf);
		if (strlen(buf) == 0 || strstr(buf, "::")) 
			continue;
	
		for (int i = strlen(buf) - 1; i >= 0; --i)
			if (buf[i] == ' ' || buf[i] == '\t')
				buf[i] = '\0';
			else
				break;
		
		//m_templateStrings.push_back(buf);
		if (strcmp(buf, "#ComplexFeatures") == 0)
		{	
			kernelFeature = false;
			m_nKernel = (int)m_mapK2idx.size();
//			fprintf(stderr, "Kernels: \n");
//			for (size_t i = 0; i < m_kerVec.size(); ++i)
//				fprintf(stderr, "%s  ", m_kerVec[i].c_str());
//			fprintf(stderr, "\n--------------------------\n");
		}
		else if (kernelFeature == true)
		{	
			if (m_mapK2idx.find(buf) == m_mapK2idx.end())
			{
				m_kerVec.push_back(buf);
				m_mapK2idx[buf] = kFeatureId ++ ;
			}
		}
		else
		{
			string name(buf);
			vector<char *> tokens = Split(buf, " \t\r\n");
			const int MAX_K_NUM = 128;
			int kAry[MAX_K_NUM] = {0}, numComponent = 0;
			
			for (size_t i = 0; i < tokens.size(); ++i)
			{
				if (m_mapK2idx.find(tokens[i]) == m_mapK2idx.end())
				{
					fprintf(stderr, "unknown kernel %s\n", tokens[i]);
					throw("Error: unknown feature component");
				}
				kAry[numComponent ++] = m_mapK2idx[tokens[i]];
			}
			m_vTemps.push_back(new _TPLT(name, kAry, numComponent));
		}
	}

	fprintf(stderr, "Total %lu templates read\n", m_vTemps.size());
	fclose(fp);
	return true;
}

bool CFeatureCollector::
SaveTemplateFile(const string &strPath)
{
	FILE *fp = fopen(strPath.c_str(), "w");
	assert(fp != NULL);
	// kernels
	for (size_t i = 0; i < m_kerVec.size(); ++i)
		fprintf(fp, "%s\n", m_kerVec[i].c_str());
	
	// templates
	fprintf(fp, "#ComplexFeatures\n");
	for (size_t i = 0; i < m_vTemps.size(); ++i)
		fprintf(fp, "%s\n", m_vTemps[i]->m_strName.c_str());

	fclose(fp);
	return true;
}

bool CFeatureCollector::
SaveFeatures(const string &strPath)
{
	FILE *fp = fopen(strPath.c_str(), "w");
	assert(fp != NULL);
	for (size_t i = 0; i < m_vTemps.size(); ++i)
		m_vTemps[i]->Save(fp);
	fclose(fp);
	return true;
}

bool CFeatureCollector::
LoadFeatures(const string &strPath)
{
	FILE *fp = fopen(strPath.c_str(), "r");
	assert(fp != NULL);
	fprintf(stderr, "Loading features...");
	for (size_t i = 0; i < m_vTemps.size(); ++i)
		m_vTemps[i]->Load(fp);
	fclose(fp);
	fprintf(stderr, "Done");
	return true;
}


void CFeatureCollector::
GetFeatures(vector<int> &vFIDs)
{
	vector<int> vKernels;
	ExtractKernels(vKernels);
	
	vFIDs.resize(m_vTemps.size());
	memset(&vFIDs[0], -1, vFIDs.size() * sizeof(int));
	
	static int buf[128], k = 0;
	for (size_t i = 0; i < m_vTemps.size(); ++i)
	{
		_TPLT *pTemp = m_vTemps[i];
		
		for (k = 0; k < pTemp->m_nKernel; ++k)
			if ((buf[k] = vKernels[pTemp->m_pKidxes[k]]) == -1)
				break;

		if (k == pTemp->m_nKernel)
		{
			vFIDs[i] = pTemp->GetFID(buf);
			if (vFIDs[i] == -1 && m_insertMode == true)
				vFIDs[i] = pTemp->Insert(buf, m_gID++);
			if (m_verbose == true)
			{
				fprintf(stderr, "temp %s: ", pTemp->m_strName.c_str());
				int idx = 0;
				if (pTemp->m_strName.find("d_") != pTemp->m_strName.npos)
					fprintf(stderr, "%d-", buf[idx++]);
				for (; idx < pTemp->m_nKernel; ++idx)
					fprintf(stderr, "%s-", 
									wstr2utf8(SSenNode::GetKey(buf[idx])).c_str());
				fprintf(stderr, "\n");
			}
		}
	}

	if (m_verbose == true)
		fprintf(stderr, "\n\n");
}

inline int
parseDis(const char *p, CDepTree ** pTrees, 
					int idxQ, int senLen)
{
	int oft = p[1] - '0';
	if (p[0] == 's' && pTrees[oft] != NULL)
		return pTrees[oft]->Index();
	else if (p[0] == 'q' && idxQ + oft < senLen)
		return idxQ + oft;
	return -1;
}

inline CDepTree *
GetChild(CDepTree *p, char dir, int cidx)
{
	if (dir == 'l')
		return cidx == 0 ? p->GetLC() : p->GetL2C();
	else
		return cidx == 0 ? p->GetRC() : p->GetR2C();
}

void CFeatureCollector::
ExtractKernels(vector<int> &vKernels)
{
	vKernels.resize(m_mapK2idx.size());
	memset(&vKernels[0],  -1,  vKernels.size() * sizeof(int));

	if (m_verbose == true)
	{
		fprintf(stderr, "extract kernels on state:\n");
		m_pState->PrintState(NULL);
	}

	int idxQ = m_pState->GetQIndex();
	CDepTree *pTrees[3] = {NULL, NULL, NULL};
	m_pState->GetTopThreeElements(pTrees);
	for (size_t i = 0; i < m_kerVec.size(); ++i)
	{
		const char *p = m_kerVec[i].c_str();
		if (m_verbose == true)
			fprintf(stderr, "kernel %-6s: ", p);

		
		if (*p == 'q')
		{
			int idx = idxQ + p[1] - '0';
			if (idx < m_pSen->Length())
				vKernels[i] = p[3] == 'w' ? m_pSen->WID(idx):
											p[3] == 't' ? m_pSen->TID(idx): m_pSen->PuncID(idx);
			else
				vKernels[i] = SSenNode::s_iEnd;
				
			if (m_verbose == true)
				fprintf(stderr, "%s\n", vKernels[i] == -1 ? "":
																wstr2utf8(SSenNode::GetKey(vKernels[i])).c_str());
		}
		else if (*p == 'd')		// e.g. d_s2q1
		{
			int idx1 = parseDis(p + 2, pTrees, idxQ, m_pSen->Length());
			int idx2 = parseDis(p + 4, pTrees, idxQ, m_pSen->Length());
			vKernels[i] = (idx1 != -1 && idx2 != -1) ?
										(idx2 - idx1): -1;
			
			if (m_verbose == true)
				fprintf(stderr, "%d\n", vKernels[i]);
		}
		else
		{
			int idx = p[1] - '0';
			if (pTrees[idx] == NULL)
				continue; 
			
			int len = strlen(p);
			switch (len)
			{
				case 4:
				{
					int wIdx = pTrees[idx]->Index();
					if (p[3] == 'w')
						vKernels[i] = m_pSen->WID(wIdx);
					else if (p[3] == 't')
						vKernels[i] = m_pSen->TID(wIdx);
					else if (p[3] == 'p')
						vKernels[i] = pTrees[idx]->PuncID();
					else if (p[3] == 'f')
						vKernels[i] = pTrees[idx]->IsLeaf();
					else
						fprintf(stderr, "Unsupport kernel %s\n", p);

					if (m_verbose == true)
						fprintf(stderr, "%s\n",
										vKernels[i] == -1 ? "":
										wstr2utf8(SSenNode::GetKey(vKernels[i])).c_str());
					break;
				}
				case 5:
				{
					vKernels[i] = p[4] == 'l' ? pTrees[idx]->GetLCNum(): 
																			pTrees[idx]->GetRCNum();
					if (m_verbose == true)
						fprintf(stderr, "%d\n", vKernels[i]);
					break;
				}
				case 8:
				{
					// kernerls of the form s0.lc0.w
					CDepTree *pChild = GetChild(pTrees[idx], p[3], p[5]-'0');
					if (pChild == NULL)
						continue;

					int idx = pChild->Index();
					if (p[7] == 'w')
						vKernels[i] = m_pSen->WID(idx);
					else if (p[7] == 't')
						vKernels[i] = m_pSen->TID(idx);
					else if (p[7] == 'p')
						vKernels[i] = pChild->PuncID();
					else if (p[7] == 'l')
						vKernels[i] = pChild->GetDepId();
					else
						fprintf(stderr, "Unsupport kernel %s\n", p);
					
					if (m_verbose == true)
						fprintf(stderr, "%s\n",  
										vKernels[i] == -1 ? "":
										wstr2utf8(SSenNode::GetKey(vKernels[i])).c_str());
					break;
				}
			}
		}
	}
}

