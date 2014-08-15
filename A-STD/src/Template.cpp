#include <cassert>
#include "Template.h"
#include "util.h"
CPool SKernels::s_pool;

bool STemplate::Save(FILE *fp)
{
	assert(fp != NULL);
	fprintf(fp, "%s\n", m_strName.c_str());

	_K2ID_MAP::iterator end = m_mapK2FID.end(), iter;
	int nSave = m_mapK2FID.size();
	fprintf(fp, "%d\n", nSave);
	for (iter = m_mapK2FID.begin(); iter != end; ++ iter)
	{
		int gId = iter->second;
		SKernels &refKey = iter->first;
		assert(refKey.len == m_nKernel);
		for (int k = 0; k < refKey.len; ++k)
			fprintf(fp, "%d ", refKey.vals[k]);
		fprintf(fp, "%d\n", gId);
	}
	return true;
}

bool STemplate::Load(FILE *fp)
{
	assert(fp != NULL);
	char buf[4096];
	
	fgets(buf, 4096, fp);
	removeNewLine(buf);
	assert(strcmp(buf, m_strName.c_str()) == 0);

	fgets(buf, 4096, fp);
	int nLoad = 0, i = 0;
	sscanf(buf, "%d", &nLoad);
	SKernels kernel;
	int kids[20];
	while (i++ < nLoad && fgets(buf, 4096, fp))
	{
		removeNewLine(buf);
		vector<char *> tokens = Split(buf, " \r\t\n");
		int gID = atoi(tokens.back());
		
		tokens.pop_back();
		assert((int)tokens.size() == m_nKernel);
		for (int k = 0; k < m_nKernel; ++k)
			kids[k] = atoi(tokens[k]);
		kernel.len = m_nKernel;
		kernel.vals = kids;
		Insert(kids, gID);
		kernel.vals = NULL;
	}
	return true;
}
