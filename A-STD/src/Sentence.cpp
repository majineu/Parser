#include "Sentence.h"
CNumberer<wstring, WStrHash> SSenNode::s_nbW;
CNumberer<wstring, WStrHash> SSenNode::s_nbT;
CNumberer<wstring, WStrHash> SSenNode::s_nbP;
const wchar_t * SSenNode::s_sEnd = L"</s>";
const wchar_t * SSenNode::s_sPuncNul = L"<punc>";
int SSenNode::s_iEnd = 0;
int SSenNode::s_iPuncNul = 0;
wstring SSenNode::s_puncs[] = {L"-B-", L"-E-", L",", L".", L":",  L"``", L"''", L"-LRB-", L"-RRB-", L"PU"};
unordered_set<int> SSenNode::s_sPunc;
unordered_set<int> SSenNode::s_sBPunc;
unordered_set<int> SSenNode::s_sEPunc;
unordered_map<int, int> 	SSenNode::s_mBPunc;
unordered_map<int, int> 	SSenNode::s_mEPunc;
unordered_map<int, int> 	SSenNode::s_mPunc2Sep;


void SSenNode::
InitPuncIDSet()
{
	fprintf(stderr, "\npunct id pair:\n");
	for (size_t i = 0; i < sizeof(s_puncs)/sizeof(*s_puncs); ++i) {
		fprintf(stderr, "[%s:%d] ", 
						wstr2utf8(s_puncs[i]).c_str(), 
						s_nbW.GetId(s_puncs[i], true));
		s_sPunc.insert(s_nbW.GetId(s_puncs[i], true));
	}
	
	fprintf(stderr, "\nboundary id pair:\n");
	
	int SepBID = s_nbW.GetId(L"-SEP-", true);
	int SepEID = s_nbW.GetId(L"-/SEP-", true);
	int PairID = s_nbW.GetId(L"-pair-", true);

	for (size_t i = 0; i < CConfig::vBoundaryPairs.size(); i += 2) {
		int bID = s_nbW.GetId(utf82wstr(CConfig::vBoundaryPairs[i]), true);
		int eID = s_nbW.GetId(utf82wstr(CConfig::vBoundaryPairs[i + 1]), true);
		s_sBPunc.insert(bID);
		s_sEPunc.insert(eID);
	
		s_mPunc2Sep[bID] = SepBID;
		s_mPunc2Sep[eID] = SepEID;
		s_mBPunc[SepBID] = PairID;
		s_mEPunc[SepEID] = PairID;
	}
}


void SSenNode::SaveNumberer(const string &path)
{
	FILE *fp = fopen(path.c_str(), "w");
	assert(fp);
	for (int i = 0; i < s_nbW.size(); ++i)
		fprintf(fp, "%s\n", wstr2utf8(*s_nbW.GetKey(i)).c_str());

	fclose(fp);
}


void SSenNode::LoadNumberer(const string &path)
{
	FILE *fp = fopen(path.c_str(), "r");
	assert(fp);
	wchar_t buf[65535];
	while (fgetws(buf, 65535, fp))
	{
		removeNewLine(buf);
		s_nbW.GetId(buf, true);
	}
	fclose(fp);
	InitPuncIDSet();
}


bool SSentence::AppendPuncs(CPool &rPool, vector<int> &vHIDs) 
{
	vector<int> puncCount;
	map<int, int> rmCounter;
	
	for (int i = 0; i < m_len; ++i)
		if (vHIDs[i] != -1 && SSenNode::IsPunc(TID(vHIDs[i])) == true)
			return true;
	bool consecutiveBoundary = false;
	int nContinue = 0;

	// 1. convert hpy
	ConvertHpy(rPool);

	// 2. remove puncs near boundary
	RmPuncsNearBE(vHIDs, rmCounter);

	// 3. attach ordinary puncs if needed 
	if (CConfig::bAddNonSepPunc)
		AttachNonSepPunc(vHIDs);
	
	for (int i = 0; i < m_len; ++i) {
		if (SSenNode::IsBEPunc(TID(i))) {
			if (i != 0 &&	SSenNode::IsBPunc(TID(i - 1)))
				consecutiveBoundary = true;
			puncCount.push_back(i == 0 ? 1 : (puncCount.back() + 1));
		} else {
			puncCount.push_back(i == 0 ? 0 : puncCount.back());
		}
	}
	
	bool verbose = false;
	if (verbose == true || consecutiveBoundary) {
		printf("before: %d\n", ++nContinue);
		Display(stdout, vHIDs);
	}
	

	for (int i = 0, last = -1; i < m_len; ++i) {
		if (SSenNode::IsBEPunc(TID(i))) {
			if (SSenNode::IsBPunc(TID(i))) {
				// attach the left most b punc to node[k]
				// k is the nearest WORD that on the right side of i 
				int firstB = i, k = i + 1;
				while (k < m_len && SSenNode::IsBEPunc(TID(k)))
					++k;
				
				if (k < m_len) {
					m_pNodes[k]->m_iP = SSenNode::Punc2SepID(TID(firstB));
					m_pNodes[k]->m_nSep ++;
				}
			} else {
				// attach the current punc, node[i] to node[last]
				if (last == -1)
					continue;
				
				int PairID = SSenNode::BMap(PuncID(last));
				if (PairID != -1 && PairID == SSenNode::EMap(SSenNode::Punc2SepID(TID(i)))) {
					if (--m_pNodes[last]->m_nSep == 0)
						m_pNodes[last]->m_iP = PairID;//, singlePair = true;
				} else {
					m_pNodes[last]->m_iP = SSenNode::Punc2SepID(TID(i));
					m_pNodes[last]->m_nSep ++;
				}
			}
		} else {
			last = i;
		}
	}
		
	for (int i = 0; i < m_len; ++i)
	{
		if (SSenNode::IsBEPunc(TID(i)))
			continue;
		vHIDs[i] = vHIDs[i] == -1 ? -1 : vHIDs[i] - puncCount[vHIDs[i]];
		vHIDs[i - puncCount[i]]	=	vHIDs[i];
		m_pNodes[i - puncCount[i]] = m_pNodes[i];
	}
	m_len -= puncCount.back();
	vHIDs.resize(m_len);

	if (verbose || consecutiveBoundary)// || singlePair == true)
	{
		printf("Done: length of head vec %lu\n",	vHIDs.size());
		Display(stdout, vHIDs);
	}
	return false;
}


void SSentence::
ConvertHpy(CPool &rPool)
{
	int hpy = 0;
	for (int i = 0; i < m_len; ++i)
		if (wcscmp(Word(i), L"--") == 0)
			hpy ++;


	if (hpy > 0 && hpy % 2 == 0) {
		for (int i = 0, hpy = 0; i < m_len; ++i) {
			if (wcscmp(Word(i), L"--") == 0) {
				const wchar_t * pwzTag = ++hpy % 2 == 1 ? L"-B-" : L"-E-";
				m_pNodes[i]->m_pwzTag = copyWcs(pwzTag, &rPool);
				m_pNodes[i]->m_iT = SSenNode::GetId(Tag(i));
			}
		}
	}
}


// nonSepPunc are those that do not belong to boundary puncs
void SSentence::
AttachNonSepPunc(vector<int> &vHIDs)
{
	vector<int> vRM(m_len, 0);
	for (int i = 0, last = -1; i < m_len; ++i) {
		if (SSenNode::IsPunc(TID(i)) == true && 
				SSenNode::IsBEPunc(TID(i)) == false) {
			if (last != -1) {
				vRM[i] = i == 0 ? 1 : vRM[i-1] + 1;
				if (!(i == m_len - 1 && CConfig::bIgnoreLastPunc == true))
					m_pNodes[last]->m_iP = TID(i);
			}
		} else {
			last = i;
			vRM[i] = i == 0 ? 0: vRM[i - 1];
		}
	}
	
	for (int i = 0; i < m_len; ++i) {
		if ((i == 0 && vRM[i] == 1) || vRM[i] == vRM[i-1] + 1)
			continue;
		vHIDs[i] = vHIDs[i] == -1 ? -1 : vHIDs[i] - vRM[vHIDs[i]];
		vHIDs[i - vRM[i]]	=	vHIDs[i];
		m_pNodes[i - vRM[i]] = m_pNodes[i];
	}

	m_len -= vRM.back();
	vHIDs.resize(m_len);
}


// remove punctuations near boundaries
void SSentence::
RmPuncsNearBE(vector<int> &vHIDs, map<int, int> &rmCounter)
{
	vector<int> vRM(m_len, 0);
	bool got = false;
	for (int i = 0; i < m_len; ++i)
	{
		if (SSenNode::IsPunc(TID(i)) == true && 
				SSenNode::IsBEPunc(TID(i)) == false)
		{
			// check right
			int k = i + 1;
			while (k < m_len && SSenNode::IsPunc(TID(k)) && 
						 !SSenNode::IsBEPunc(TID(k)))
				++k;
			if (k < m_len && SSenNode::IsBEPunc(TID(k)))
			{
				got = true;
				if (rmCounter.find(TID(i)) != rmCounter.end())
					rmCounter[TID(i)] ++;
				else
					rmCounter[TID(i)] = 1;
				vRM[i] = i == 0? 1: vRM[i - 1] + 1;
		
				continue;
			}

			// check left
			k = i - 1;
			while (k >= 0 && SSenNode::IsPunc(TID(k)) &&
						 !SSenNode::IsBEPunc(TID(k)))
				--k;

			if (k >= 0 && SSenNode::IsBEPunc(TID(k)))
			{
				got = true;
				if (rmCounter.find(TID(i)) != rmCounter.end())
					rmCounter[TID(i)] ++;
				else
					rmCounter[TID(i)] = 1;
				vRM[i] = i == 0 ? 1: vRM[i - 1] + 1;
				
				continue;
			}
		}

		vRM[i] = i == 0? 0:vRM[i - 1];
	}

//	if (got)
//	{
//		fprintf(stderr, "Before \n");
//		Display(stderr, vHIDs);
//	}
	
	for (int i = 0; i < m_len; ++i)
	{
		if ((i == 0 && vRM[i] == 1) || vRM[i] == vRM[i-1] + 1)
			continue;
		vHIDs[i] = vHIDs[i] == -1 ? -1 : vHIDs[i] - vRM[vHIDs[i]];
		vHIDs[i - vRM[i]]	=	vHIDs[i];
		m_pNodes[i - vRM[i]] = m_pNodes[i];
	}
	m_len -= vRM.back();
	vHIDs.resize(m_len);

//	if (got)
//	{
//		fprintf(stderr, "\nAfter \n");
//		Display(stderr, vHIDs);
//		fprintf(stderr, "\n");
//	}
}


void SSentence::
Display(FILE *fp, vector<int> &vHIDs)
{
	fprintf(fp, "len %d\n", m_len);
	for (int i = 0; i < m_len; ++i)
			fprintf(fp, "%s %s %d %s %s pNum %d\n", 
				wstr2utf8(Word(i)).c_str(),
				wstr2utf8(Tag(i)).c_str(),
				vHIDs[i] + 1,
				wstr2utf8(Label(i)).c_str(),
				PuncID(i) == -1? "":
				wstr2utf8(SSenNode::GetKey(PuncID(i))).c_str(),
				m_pNodes[i]->m_nSep);
	fprintf(fp, "\n");
}


void SSentence::
PrintMaltFormat(FILE *fp, vector<int> &vHIDs)
{
	for (int i = 0; i < m_len; ++i)
		fprintf(fp, "%s %s %d %s\n", 
				wstr2utf8(Word(i)).c_str(),
				wstr2utf8(Tag(i)).c_str(),
				vHIDs[i] + 1,
				wstr2utf8(Label(i)).c_str());
	fprintf(fp, "\n");
}

