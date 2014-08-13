#include "IO.h"


vector<CDepTree *> CReader::
ReadTrees(const char *pszPath, 
					CPool &rPool, 
					vector<SSentence *> *pSenVec, 
					bool AddDLabel)
{
	FILE * fpIn = fopen(pszPath, "r");
	CHK_OPEN_FAILED(fpIn, pszPath);

	char buf[BUF_LEN];
	vector<wstring> lines;
	vector<CDepTree *> treeVec;
	vector<int> idxVec;
	int nLine = 0;

	if (pSenVec != NULL)
		pSenVec->clear();

	int nRm = 0;
	while (fgets(buf, BUF_LEN, fpIn) != NULL)
	{
		if (++nLine % 100000 == 0)
			fprintf(stderr, "Reading %d lines\r", nLine);
		removeNewLine(buf);

		if (strlen(buf) == 0)
		{
			if (lines.size() != 0)
			{
				SSentence *pSen = SSentence::CreateSentence(rPool, lines, true, &idxVec);
				bool puncHead = pSen->AppendPuncs(rPool, idxVec);// == true)
				nRm += puncHead;
				
				if (AddDLabel)
					for (int i = 0; i < pSen->Length(); ++i)
						CIDMap::AddDependencyLabel(pSen->Label(i));
				
				CDepTree *pTree = CDepTree::BuildTree(idxVec, pSen, rPool);
				if (puncHead == true)
				{
					pTree->DisplayTreeStructure(stdout);
					fprintf(stdout, "\n");
				}
				else
					treeVec.push_back(pTree);
				
				lines.clear();
				if (puncHead == false && pSenVec != NULL)
					pSenVec->push_back(pSen);
			}
		}
		else
		{
			wstring line = CCodeConversion::UTF8ToUnicode(buf);
			lines.push_back(line);
		}
	}

	// handle the last one
	if (lines.size() != 0)
	{
		SSentence *pSen = SSentence::CreateSentence(rPool, lines, true, &idxVec);
		bool puncHead = pSen->AppendPuncs(rPool, idxVec);// == true)
		nRm += puncHead;
		
		if (AddDLabel)
			for (int i = 0; i < pSen->Length(); ++i)
				CIDMap::AddDependencyLabel(pSen->Label(i));
				
		CDepTree *pTree = CDepTree::BuildTree(idxVec, pSen, rPool);
		if (puncHead == true)
			pTree->DisplayTreeStructure(stdout);
		else
			treeVec.push_back(pTree);
				
		lines.clear();
		if (puncHead == false && pSenVec != NULL)
			pSenVec->push_back(pSen);
	}

	fprintf(stderr, "Total %lu sentences readed, %d punc head \n", pSenVec->size(), nRm);
	fclose(fpIn);
	CIDMap::BuildOutcomeIds();
	return treeVec;
}


vector<SSentence *> CReader::
ReadSentences(const char *pszPath, CPool &rPool)
{
	FILE * fpIn = fopen(pszPath, "r");
	CHK_OPEN_FAILED(fpIn, pszPath);

	char buf[BUF_LEN];
	vector<wstring> lines;
	vector<SSentence *> vSen;
	vector<int> idxVec;
	int nLine = 0;

	while (fgets(buf, BUF_LEN, fpIn) != NULL)
	{
		if (++nLine % 100000 == 0)
			fprintf(stderr, "Reading %d lines\r", nLine);
		removeNewLine(buf);

		if (strlen(buf) == 0)
		{
			if (lines.size() != 0)
			{
				SSentence *pSen = SSentence::CreateSentence(rPool, lines, true, &idxVec);
				lines.clear();
				vSen.push_back(pSen);
			}
		}
		else
		{
			wstring line = CCodeConversion::UTF8ToUnicode(buf);
			lines.push_back(line);
		}
	}

	// handle the last one
	if (lines.size() != 0)
	{
		SSentence *pSen = SSentence::CreateSentence(rPool, lines, true, &idxVec);
		lines.clear();
		vSen.push_back(pSen);
	}

	fprintf(stderr, "Total %lu sentences readed\n",	vSen.size());
	fclose(fpIn);
	return vSen;
}
