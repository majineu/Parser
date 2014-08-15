#include "Maps.h"
/* static members for global access */
CStrHashMap CIDMap::m_depLabelMap;
CStrHashMap CIDMap::m_outcomeMap;
CStrHashMap CIDMap::m_predMap;
int CIDMap::m_nLabel = 0;

vector<wstring *> CIDMap::m_outcomeVec;
vector<wstring *> CIDMap::m_predVec;
vector<wstring *> CIDMap::m_depLabelVec;
