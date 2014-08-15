#include"Eval.h"
void HIndexes(CDepTree *pTree, vector<int> &hIdx)
{
  int length = pTree->GetSen()->Length();

  hIdx.clear();
  hIdx.resize(length, -1);

	vector<CDepTree *> tNodes;
	pTree->CollectTreeNodes(tNodes);

  // fillin head indexes
	for (CDepTree *node : tNodes)
  {
    int idx = node->Index();
    hIdx[idx] = node->HIndex();
  }
}

const wchar_t *pPuncs[] = {L",", L".",  L":", L"``", L"''", L"-LRB-", L"-RRB-", L"PU"};

SEvalRec Eval(vector<SSentence *> &vSen,  vector<CDepTree *> &vTree,  vector<vector<int>> &hIdxVec)
{
	static std::set<wstring> puncSet(pPuncs, pPuncs + sizeof(pPuncs)/sizeof(*pPuncs));
	assert(vSen.size() == vTree.size());
	assert(vSen.size() == hIdxVec.size());
	bool verbose = false;
	SEvalRec er;
	vector<int> goldHidx;
	for (size_t i = 0; i < vSen.size(); ++i)
	{
		if (vSen[i]->Length() != (int)hIdxVec[i].size() ||
			vSen[i]->Length() != vTree[i]->GetSen()->Length())
		{
			fprintf(stderr, "Error: sentence length inconsistent\n");
			abort();
		}

		HIndexes(vTree[i], goldHidx);
		bool cmpMatch = true;
		bool root     = false;

		if (verbose == true)
		{
			vTree[i]->DisplayTreeStructure(stderr, 0);
			for (size_t k = 0; k < goldHidx.size(); ++k)
				fprintf(stderr, "%d, <%d, %d>\n", (int)k, hIdxVec[i][k], goldHidx[k]);
			fprintf(stderr, "------------------------------------------\n");
		}

		for (int k = 0; k < vSen[i]->Length(); ++k)
		{
			if (puncSet.find(vSen[i]->Tag(k)) == puncSet.end()) //wcscmp(vSen[i]->Tag(k), L"PU") != 0)
			{
				er.totalWord += 1;
				if (goldHidx[k] == hIdxVec[i][k])
				{
					er.nWordDep += 1;
					if (goldHidx[k] == CDepTree::ROOT_IDX)
						root = true;
				}
				else
					cmpMatch = false;
			}
		}
		
		er.nCompl += cmpMatch;
		er.nRoot  += root;

		verbose = false;
	}


	fprintf(stderr, "\n");
	
  er.nSen		= (int)vSen.size();
	er.accuracy = 100.0 * er.nWordDep / er.totalWord;
	
  fprintf(stderr, "total %d, corr %d, compl %d, root %d\n" 
          "acc %.2f, compl rate %.2f, root acc %.2f\n", 
					er.totalWord,  er.nWordDep,  er.nCompl,  er.nRoot,  
					er.accuracy,  100.0 * er.nCompl/er.nSen, 100.0*er.nRoot/er.nSen);
	return er;
}

void InBoundaryAcc(	SSentence *pSen, 
										CDepTree *pRef, CDepTree *pPred, 
										int &nTotal, int &nCorr, FILE *fp)
{
	vector<int> hPred, hRef;
	HIndexes(pRef,  hRef);
	HIndexes(pPred, hPred);
	int  nBsep = 0;
	nTotal = nCorr = 0;
	for (int i = 0; i < pSen->Length(); ++i)
	{
		int PID = pSen->PuncID(i);
		if (SSenNode::IsBSep(PID))
			nBsep += pSen->SepNum(i);
		else if (SSenNode::IsESep(PID))
			nBsep -= pSen->SepNum(i);

	
		if (SSenNode::IsPunc(pSen->TID(i)) == false)
		{
			if (nBsep > 0 || 
					(nBsep == 0 &&
					(PID == SSenNode::s_nbW.GetId(L"-pair-", false) || 
					SSenNode::IsESep(PID))))
			{
				nCorr += hRef[i] == hPred[i];
				nTotal ++;
				fprintf(fp, "%d ", i);
			}
		}
	}

	if (nTotal > 0)
		fprintf(fp, "\nIn-Score: %d/%d %.2f\n", nCorr, nTotal, 
								100.0 * nCorr/nTotal);
}

std::pair<int, int> 
BoundarySenAcc(SSentence *pSen, CDepTree *pRef, CDepTree *pPred)
{
	bool bSen = false;
	vector<int> hPred, hRef;
	HIndexes(pRef,  hRef);
	HIndexes(pPred, hPred);
	for (int i = 0; i < pSen->Length(); ++i)
	{
		int PID = pSen->PuncID(i);
		if (PID == SSenNode::s_nbW.GetId(L"-pair-", false) || 
				SSenNode::IsBSep(PID) || SSenNode::IsESep(PID))
		{
			bSen = true;
			break;
		}
	}

	if (bSen == false)
		return std::pair<int,int> (0, 0);
	
	int total = 0, corr = 0;
	for (int i = 0; i < pSen->Length(); ++i)
	{
		int TID = pSen->TID(i);
		if (SSenNode::IsPunc(TID) == false)
		{
			total++;
			corr += hRef[i] == hPred[i];
		}
	}
	return std::pair<int,int>(total, corr);
}


