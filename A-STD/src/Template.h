#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__
#include "Maps.h"

struct SKernels
{
	int *vals;
	int len;
	static CPool s_pool;
	SKernels()						{vals = NULL, len = 0;}
	~SKernels()						{vals = NULL, len = 0;}
	
	SKernels(int *pVal, int l)
	{
		len = l;
		vals = (int *)s_pool.Allocate(sizeof(int) * l);
		assert(vals);
		memcpy(vals, pVal, sizeof(int) * l);
	}

	SKernels(const SKernels & r)
	{
		len = r.len;
		vals = (int *)s_pool.Allocate(sizeof(int) * r.len);
		assert(vals);
		memcpy(vals, r.vals, sizeof(int) * len);
	}


	bool operator == (const SKernels &r)const
	{
		for (int i = 0; i < r.len; ++i)
			if (vals[i] != r.vals[i])
				return false;

		return true;
	}
};

class KernelsHasher
{
public:
	size_t operator()(const SKernels & pval) const 
	{
		int *buf = pval.vals;
		unsigned int index = 1315423911;
		int i = 0; 
		while(i++ < pval.len)
			index^=((index<<5) + (index>>2) + *buf++);// 5.92

		return (index & 0x7FFFFFFF);
	}
};

typedef CCompactHashMap<SKernels, int, KernelsHasher> _K2ID_MAP;
struct STemplate
{

#if 0
	void Display (const map<wstring, int> &refKernelMap, 
								const vector<wstring> &kernelVec,
			 					FILE *fp = stderr)
	{
		fprintf(fp, "%s :", wstr2utf8(name).c_str());
		fprintf(fp, "<dis %d, pp %d> ", m_useDistance, m_involvePP);
		if (m_useDistance == true)
		fprintf(fp, "<%s%d %s%d>:", m_disPending1 <= 0 ? "l":"r", 
									m_disPending1, m_disPending2 <= 0 ? "l":"r", 
									m_disPending2);
		

		map<PENDING_INDEX_TYPE, SPend_Kernel>::iterator iter = m_pendKernel.begin();
		
		for (; iter != m_pendKernel.end(); ++iter)
		for (size_t i = 0; i < iter->second.vecKernelInd.size(); ++i)
			fprintf(fp, "<%s%d %s> ", iter->first <= 0 ? "l":"r", 
				iter->first, wstr2utf8(kernelVec[iter->second.vecKernelInd[i]]).c_str());
	}
#endif

	STemplate()									{m_pKidxes = NULL, m_nKernel = 0;}
	STemplate(const string &name, 
						int *pKIdxes, int nIdx)
	{
		assert(pKIdxes != NULL && nIdx > 0);
		m_strName = name;
		m_pKidxes = new int[nIdx];
		m_nKernel = nIdx;
//		fprintf(stderr, "temp name %s, len %d\n", m_strName.c_str(), m_nKernel);
		memcpy(m_pKidxes, pKIdxes, sizeof(int) * nIdx);
	}

	~STemplate()
	{
		if (m_pKidxes != NULL)
			delete m_pKidxes;
		m_strName  = "";
		m_nKernel	 = 0;
		m_pKidxes	 = NULL;
	}
	
	int GetFID(int *kernelIds)
	{
		SKernels temp;
		temp.vals = kernelIds, temp.len = m_nKernel;
		_K2ID_MAP::iterator findIter = m_mapK2FID.Find(temp);
		temp.vals = NULL;
		return findIter == m_mapK2FID.end() ? -1 : findIter->second;
	}

	int Insert(int *kernelIds, int gID)///*, int nKernels*/
	{
		SKernels temp;
		temp.vals = kernelIds, temp.len = m_nKernel;
		_K2ID_MAP::iterator iter = m_mapK2FID.Insert(temp, gID);
		temp.vals = NULL;
		return iter->second;
	}

	bool Save(FILE *fp);
	bool Load(FILE *fp);
	
	_K2ID_MAP		m_mapK2FID;
	string 			m_strName;
	bool 				m_useDistance;
	int 				m_nKernel;
	int 				*m_pKidxes;
};

typedef STemplate _TPLT;
#endif  /*__TEMPLATE_H__*/
