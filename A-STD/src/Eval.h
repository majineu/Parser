#ifndef __EVAL_H__
#define __EVAL_H__
#include "Config.h"
#include "DepTree.h"
#include "SRParser.h"
struct SEvalRec
{
	int			nCompl;
	int 		nRoot;
	int 		nWordDep;
	int 		totalWord;
	int 		nSen;
	double	accuracy;
	SEvalRec()		{memset(this, 0, sizeof(*this));}

	void Display(FILE *fp = stderr)
	{
		fprintf(fp, "total %d, sen %d, corr %d, compl %d, root %d, " 
					"acc %.2f, compl rate %.2f, root acc %.2f\n", 
					totalWord, nSen, nWordDep,  nCompl,  nRoot,  
					accuracy,  100.0 * nCompl / nSen, 100.0 * nRoot / nSen);
	}
};

void HIndexes(CDepTree *pTree, vector<int> &hIdx);
SEvalRec Eval(vector<SSentence *> &vSen, 
							vector<CDepTree *> &vTree,  
							vector<vector<int>> &hIdxVec);
SEvalRec ParsingAndEval(CSRParser &, vector<SSentence *> &vSen,  vector<CDepTree *> &vTree);

void InBoundaryAcc(	SSentence *pSen, 
										CDepTree *pRef, CDepTree *pPred, 
										int &nTotal, int &nCorr, 
										FILE *fp = stderr);

std::pair<int, int> 
BoundarySenAcc(SSentence *pSen, CDepTree *pRef, CDepTree *pPred);

#endif  /*__EVAL_H__*/
