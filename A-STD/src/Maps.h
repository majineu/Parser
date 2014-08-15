#ifndef __MAPS_H__
#define __MAPS_H__
#include <vector>
#include <cassert>
#include "Hash.h"
#include "Pool.h"

using std::vector;

class CStrHashMap
{
public:
	typedef std::pair<wstring, size_t>    ITEM_TYPE;
	typedef list<ITEM_TYPE>::iterator  iterator;

private:
	list<ITEM_TYPE> m_lItemList;
	size_t m_nTableSize;
	size_t m_nItemNum;
	double m_occupyRate;
	struct SBucket
	{
		iterator beg, end;
	};
	WStrHash m_Hash;
	SBucket *m_pBuckets;
	
private:
	size_t nextPrime(size_t candidate)
	{
		candidate |= 1;
		while (((size_t)-1) != candidate)
		{
			size_t div = 3;
			size_t squ = div*div;
			while(squ < candidate && candidate % div)
			{
				squ += ++div * 4;
				++div;
			}

			if(candidate % div != 0)
				break;
			candidate += 2;
		}
	
		return candidate;
	}

	bool Grow()
	{
		/* 1. reallocate space */
		static size_t sizes[] ={	193, 389, 769, 1543, 3079, 6151, 12289, 24593,
									49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469,
									12582917, 25165843 };
		static size_t sizeLen = sizeof(sizes)/sizeof(size_t);
		size_t k = 1;
		for(k = 1; k < sizeLen && sizes[k] <= m_nTableSize; ++k);
	
		if(k != sizeLen)
			m_nTableSize = sizes[k];
		else
			m_nTableSize = nextPrime(m_nTableSize * 2);


		delete[] m_pBuckets;
		m_pBuckets = new SBucket[m_nTableSize];
		CHK_NEW_FAILED(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;

		/* 2. rehash */
		iterator it = --end, beg = m_lItemList.begin() ;
		while(it != beg)
		{
			size_t index = m_Hash(it->first) % m_nTableSize;
			if(m_pBuckets[index].beg == m_pBuckets[index].end)
			{	
				/* 1. empty buckets */
				m_pBuckets[index].beg = m_pBuckets[index].end = it--;
				m_pBuckets[index].end ++;
			}
			else if(--m_pBuckets[index].beg != it)
			{	
				/* 2. here we need to rearrange the current element */
				m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it--);
				++m_pBuckets[index].beg;
			}
			else 
				/* 3. only update the iterators */
				--it;
		}

		/* 3. handle the last item */
		size_t index = m_Hash(it->first) % m_nTableSize;
		if(m_pBuckets[index].beg == m_pBuckets[index].end)
		{	
			/* 1. empty buckets */
			m_pBuckets[index].beg = m_pBuckets[index].end = it;
			m_pBuckets[index].end ++;
		}
		else if(--m_pBuckets[index].beg != it)
		{	
			/* 2. here we need to rearrange the current element */
			m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it);
			++m_pBuckets[index].beg;
		}

		return true;
	}

public:
	CStrHashMap()
	{
		m_nTableSize = 97, m_nItemNum = 0;
		m_occupyRate = 0.8;
		
		m_pBuckets = new SBucket[ m_nTableSize ];
		CHK_NEW_FAILED(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;
	}

	~CStrHashMap()													{delete[] m_pBuckets;	}
	iterator begin()												{return m_lItemList.begin();}
	iterator end()													{return m_lItemList.end();}
	size_t size()														{return m_nItemNum;}

	iterator Insert(const wstring & key, size_t val)
	{
		return Insert(key.c_str(), val);
	}

	iterator Insert(const wchar_t * pwz, size_t val)
	{
		size_t index = m_Hash(pwz) % m_nTableSize;
		SBucket *bucket = &m_pBuckets[index];
		iterator iter = bucket->beg;

		for (; iter != bucket->end; ++iter)
		{
			if (wcscmp(iter->first.c_str(), pwz) == 0)
				return iter;
		}

		if((double)(++m_nItemNum)/m_nTableSize > m_occupyRate)
		{
			Grow();
			index = m_Hash(pwz) % m_nTableSize;
			bucket = & m_pBuckets[index];	
		}

		if(bucket->beg == bucket->end)
		{
			bucket->end = bucket->beg = m_lItemList.insert(m_lItemList.begin(), ITEM_TYPE(pwz, val));
			return bucket->end++;
		}
		else
			return m_lItemList.insert(bucket->end, ITEM_TYPE(pwz, val));
	}

	iterator Find(const wstring & refStr)
	{
		return Find(refStr.c_str());
	}

	iterator Find(const wchar_t * pwz)
	{
		if(m_nItemNum == 0)
			return m_lItemList.end();

		size_t index = m_Hash(pwz) % m_nTableSize;
		SBucket &backet = m_pBuckets[index];
		iterator it = backet.beg;
		for(; it != backet.end ; ++it)
			if(wcscmp(it->first.c_str(), pwz) == 0)
				return it;

		return m_lItemList.end();
	}
};


class CIDMap
{
public:
	enum ACTION_TYPE {SHIFT, LEFT_REDUCE, RIGHT_REDUCE};
	const static size_t UNK_ID = (size_t) -1;
	static bool ReadDepLabelFile(const string &strDepFile)
	{
		FILE *fp = fopen(strDepFile.c_str(), "r");
		CHK_OPEN_FAILED(fp, strDepFile.c_str()); 
		wchar_t buf[100];

		while (fgetws(buf, 100, fp))
		{
			removeNewLine(buf);
			int len = wcslen(buf);
			while (len-1 >=0 && (buf[len - 1] == ' '|| buf[len - 1] == '\t'))
				buf[len - 1] = 0;

			/* skip empty line */
			if (wcslen(buf) == 0)
				continue;

			AddDependencyLabel(buf);
		}
		fclose(fp);

		BuildOutcomeIds();

		fwprintf(stderr, L"\n");
		fwprintf(stderr, L"# dependency label: %d\n", (int)GetDepLabelNum());
		fwprintf(stderr, L"# outcomes:         %d\n", (int)GetOutcomeNum());
		return true;
	}

	static bool SaveDepLabel(const string &strPath)
	{
		FILE *fp = fopen(strPath.c_str(), "w");
		CHK_OPEN_FAILED(fp, strPath.c_str());
		for (size_t i = 0; i < m_depLabelVec.size(); ++i)
			fwprintf(fp, L"%ls\n", m_depLabelVec[i]->c_str());

		fclose(fp);
		return true;
	}

	static size_t AddDependencyLabel(const wchar_t * pwzDepLabel){return add(pwzDepLabel, DEP_MAP);}
	static size_t AddPredict(const wchar_t * pwzFeature)		{return add(pwzFeature, PRED_MAP);}
	static size_t GetPredictId(const wchar_t *pwzFeature)		{return getId(pwzFeature, PRED_MAP);}
	static size_t GetOutcomeId(const wchar_t *pwzLabel)			{return getId(pwzLabel, OUTCOME_MAP);}
	static const int GetDepLabelId(const wchar_t *pwz)			{return getId(pwz, DEP_MAP);}
	static CStrHashMap & GetDepLabelMap()										{return m_depLabelMap;}
	static vector<wstring *> & GetDepLabelVec()							{return m_depLabelVec;}
	static size_t GetPredNum()															{return m_predVec.size();}
	static size_t GetOutcomeNum()														{return m_outcomeVec.size();}
	static size_t GetDepLabelNum()													{return m_depLabelMap.size();	}

	static const wchar_t * GetPredict(size_t fid)
	{
		if (fid < m_predVec.size())
			return m_predVec[fid]->c_str();
		else
		{
			fwprintf(stderr, L"Error: Invalid predict id %d\n", fid);
			return NULL;
		}
	}

	static const wchar_t * GetDepLabel(const int Id) 
	{
		assert(Id < m_nLabel);// m_depLabelVec.size())
		return m_depLabelVec[Id]->c_str();				//Ji 2012-06-13
	}


	static int GetOutcomeId(const ACTION_TYPE action, 
													const int label)
	{
		if (action == SHIFT)
			return 0;
		
		int oId = 1 + label;
		return action == LEFT_REDUCE ? oId: (oId + m_nLabel);
	}

	static void BuildOutcomeIds()
	{
		if (m_depLabelMap.size() == 0)
		{
			fwprintf(stderr, L"Error: Building class label with empty depLabelMap or tagMap\n");
			exit(0);
		}

		wchar_t buf[200] = L"shift";
		add(buf, OUTCOME_MAP);
	
		m_nLabel = (int)m_depLabelVec.size();
		for (int iter = 0; iter < 2; ++ iter)
		{
			for (int i= 0; i < (int)m_depLabelVec.size(); ++i)
			{
				wcscpy(buf, iter == 0 ? L"left_": L"right_");
				wcscat(buf, m_depLabelVec[i]->c_str());
				add(buf, OUTCOME_MAP);
			}
		}
	}

	static ACTION_TYPE Interprate(const int classId,   
															  int &label)
	{
		label = classId - 1;
		if (classId == 0)
			return SHIFT;
		
		if (classId > m_nLabel)
		{
			label -= m_nLabel;
			return RIGHT_REDUCE;
		}
		else
			return LEFT_REDUCE;
	}

	


private:
	enum MAP_TYPE {PRED_MAP, OUTCOME_MAP, DEP_MAP};

	static size_t getId(const wchar_t *pwzStr, MAP_TYPE type)
	{
		CStrHashMap &refMap = type == PRED_MAP ? m_predMap:
							  type == OUTCOME_MAP ? m_outcomeMap:m_depLabelMap;//:m_tagMap;
									
		CStrHashMap::iterator iter = refMap.Find(pwzStr);
		return iter != refMap.end() ? iter->second : UNK_ID;
	}

	static size_t add(const wchar_t *pwzStr, MAP_TYPE type)
	{
		CStrHashMap &refMap = type == PRED_MAP ? m_predMap:
							  type == OUTCOME_MAP ? m_outcomeMap:m_depLabelMap;//:m_tagMap;
		
		int size = refMap.size();
		CStrHashMap::iterator iter = refMap.Find(pwzStr);
		if (iter != refMap.end())
			return iter->second;
		
		iter = refMap.Insert(pwzStr, size);
		assert(iter != refMap.end());
	
		/* register in the corresponding vector */
		std::vector<wstring *> &refVec = type == PRED_MAP ? m_predVec:
										 type == OUTCOME_MAP ? m_outcomeVec: m_depLabelVec;
		refVec.push_back(&iter->first);
		return iter->second;
	}

	static CStrHashMap m_predMap;		// predMap
	static CStrHashMap m_outcomeMap;	// class label
	static CStrHashMap m_depLabelMap;	// dependency label

	static int m_nLabel;
	static std::vector<wstring *> m_predVec;
	static std::vector<wstring *> m_outcomeVec;
	static std::vector<wstring *> m_depLabelVec;
};


template <typename T_Key, typename T_Value, typename T_Hasher>
class CCompactHashMap
{
public:
	struct HashNode
  {
    T_Key   first;
    T_Value second;
    HashNode * m_pNext;

    HashNode(const T_Key &key, const T_Value &val):first(key), second(val), m_pNext(NULL){}
  };
//  typedef typename HashNode ITEM_TYPE;
  typedef HashNode ITEM_TYPE;
	
  struct SBucket
  {
    ITEM_TYPE *head;
    ITEM_TYPE *tail;

    bool empty()					{return tail == (ITEM_TYPE *)-1;}
    SBucket ()						{head = 0; tail = (ITEM_TYPE *)-1;}
  };

  struct SIter
  {
    ITEM_TYPE *m_pItem;

    SIter(ITEM_TYPE *pItem):m_pItem(pItem){}
    SIter():m_pItem(NULL){}

    ITEM_TYPE & operator *()  {return *m_pItem;}
    ITEM_TYPE * operator ->()   {return m_pItem;}
    SIter & operator ++ () 
    {
      m_pItem = m_pItem->m_pNext; 
      return *this;
    }
    bool operator == (const SIter &r)const {return m_pItem == r.m_pItem;}
    bool operator != (const SIter &r)const {return !(*this == r);}
  };

  typedef SIter iterator;

private:
  ITEM_TYPE *pItemList;

  SBucket * m_pHashTable;
	T_Hasher  m_Hash;
  size_t    m_nTableSize;
  size_t    m_nItemNum;
	double    m_occupyRate;
  CPool     m_pool;

	
private:
	size_t nextPrime(size_t candidate)
	{
		candidate |= 1;
		while (((size_t)-1) != candidate)
		{
			size_t div = 3;
			size_t squ = div*div;
			while(squ < candidate && candidate % div)
			{
				squ += ++div * 4;
				++div;
			}

			if(candidate % div != 0)
				break;
			candidate += 2;
		}
	
		return candidate;
	}


	bool Grow()
	{
		/* 1. reallocate space */
		static size_t sizes[] ={193, 769,  12289,  196613,  1572869, 3145739, 6291469,
								12582917, 25165843 };
		static size_t sizeLen = sizeof(sizes)/sizeof(size_t);
		size_t k = 1;
		for(k = 1; k < sizeLen && sizes[k] <= m_nTableSize; ++k);
	
		if(k != sizeLen)
			m_nTableSize = sizes[k];
		else
			m_nTableSize = nextPrime(m_nTableSize * 2);


		delete[] m_pHashTable;
		m_pHashTable = new SBucket[m_nTableSize];
		assert(m_pHashTable);
	

		/* 2. rehash */
    ITEM_TYPE *pCurr = pItemList;
//    ITEM_TYPE *pNext = pCurr->m_pNext;
    
    ITEM_TYPE *pPre  = NULL;
    int nRehash = 0;
    while (pCurr != NULL)
    {
      ++nRehash;
      size_t index = m_Hash(pCurr->first) % m_nTableSize;
      if (m_pHashTable[index].empty() == true)
      {
        m_pHashTable[index].head = m_pHashTable[index].tail = pCurr;

        pPre = pCurr;
        pCurr = pCurr->m_pNext;
      }
      else if (m_pHashTable[index].tail->m_pNext == pCurr)
      {
        m_pHashTable[index].tail = pCurr;
        m_pHashTable[index].tail->m_pNext = pCurr->m_pNext;

        pPre = pCurr;
        pCurr = pCurr->m_pNext;
      }
      else 
      {
        pPre->m_pNext = pCurr->m_pNext;
        pCurr->m_pNext = m_pHashTable[index].tail->m_pNext;
        m_pHashTable[index].tail->m_pNext = pCurr;
        m_pHashTable[index].tail = pCurr;

        // pre does not change
        pCurr = pPre->m_pNext;
      }
    }
    
		return true;
	}

public:
	CCompactHashMap()
	{
		m_nTableSize = 97;
    m_nItemNum   = 0;
		m_occupyRate = 0.8;
		
    m_pHashTable = new SBucket[m_nTableSize];
    pItemList = NULL;
  }

	~CCompactHashMap()
	{
    delete m_pHashTable;
    m_pHashTable = NULL;
    
    ITEM_TYPE *pCurr = pItemList;
    while (pCurr != NULL)
    {
      pCurr->first.~T_Key();
      pCurr->second.~T_Value();

      pCurr = pCurr->m_pNext;
    }
	}

	iterator begin()
	{
    if (m_nItemNum != 0)
    {
      iterator beg;
      beg.m_pItem = pItemList;
      return beg;
    }
    else
      return end();
	}

	iterator end()
	{
    return iterator(NULL);
	}

	size_t size()
	{
		return m_nItemNum;
	}

	iterator Insert(const T_Key & key, T_Value val)
	{
		size_t index    = m_Hash(key) % m_nTableSize;
		SBucket *pBucket = m_pHashTable + index;
    if (pBucket->empty() == false)
    {
      ITEM_TYPE *pItem = pBucket->head;
      while (true)
      {
        if (pItem->first == key)
			  {
				  pItem->second = val;
				  return iterator(pItem);
			  }

        if (pItem == pBucket->tail)
          break;

        pItem = pItem->m_pNext;
      }
    }

		if ((double)(++m_nItemNum)/m_nTableSize > m_occupyRate)
		{
			Grow();
			index   = m_Hash(key) % m_nTableSize;
			pBucket = m_pHashTable + index;	
		}

    ITEM_TYPE * pNewNode = (ITEM_TYPE*)m_pool.Allocate(sizeof(*pNewNode));
    new(pNewNode)ITEM_TYPE(key, val);

    if (pBucket->empty() == true)
		{
      if (pItemList == NULL)
        pItemList = pNewNode;
      else
      {
        pNewNode->m_pNext = pItemList;
        pItemList         = pNewNode;
      }

			pBucket->tail = pBucket->head = pItemList;
			return iterator(pItemList);
		}
		else
    {
      pNewNode->m_pNext     = pBucket->tail->m_pNext;
      pBucket->tail->m_pNext  = pNewNode;
      pBucket->tail = pNewNode;

      return iterator(pNewNode);
    }
	}

	iterator Find(const T_Key &key)
	{
		if(m_nItemNum == 0)
			return end();

		size_t index = m_Hash(key) % m_nTableSize;
    SBucket *pBucket = m_pHashTable + index;
    if (pBucket->empty() == false)
    {
      ITEM_TYPE *pItem = pBucket->head;
      while (true)
      {
        if (pItem->first == key)
          return iterator(pItem);

        if (pItem == pBucket->tail)
          break;

        pItem = pItem->m_pNext;
      }
    }

		return end();
	}
};


template <typename T_Key, typename T_Value, typename T_Hasher>
class CHashMap
{
public:
	typedef std::pair<T_Key, T_Value>  ITEM_TYPE;
	
public:
	typedef typename std::list<ITEM_TYPE>::iterator  iterator;

private:
	list<ITEM_TYPE> m_lItemList;
	size_t m_nTableSize;
	size_t m_nItemNum;
	double m_occupyRate;
	struct SBucket
	{
		iterator beg, end;
	};
	
	
	T_Hasher m_Hash;
	SBucket *m_pBuckets;
	
private:
	size_t nextPrime(size_t candidate)
	{
		candidate |= 1;
		while (((size_t)-1) != candidate)
		{
			size_t div = 3;
			size_t squ = div*div;
			while(squ < candidate && candidate % div)
			{
				squ += ++div * 4;
				++div;
			}

			if(candidate % div != 0)
				break;
			candidate += 2;
		}
	
		return candidate;
	}

	bool Grow()
	{
		/* 1. reallocate space */
		static size_t sizes[] ={193, 769,  12289,  196613,  1572869, 3145739, 6291469,
								12582917, 25165843 };
		static size_t sizeLen = sizeof(sizes)/sizeof(size_t);
		size_t k = 1;
		for(k = 1; k < sizeLen && sizes[k] <= m_nTableSize; ++k);
	
		if(k != sizeLen)
			m_nTableSize = sizes[k];
		else
			m_nTableSize = nextPrime(m_nTableSize * 2);


		delete[] m_pBuckets;
		m_pBuckets = new SBucket[m_nTableSize];
		assert(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;

		/* 2. rehash */
		iterator it = --end, beg = m_lItemList.begin() ;
		while(it != beg)
		{
			size_t index = m_Hash(it->first) % m_nTableSize;
			if(m_pBuckets[index].beg == m_pBuckets[index].end)
			{	
				/* 1. empty buckets */
				m_pBuckets[index].beg = m_pBuckets[index].end = it--;
				m_pBuckets[index].end ++;
			}
			else if(--m_pBuckets[index].beg != it)
			{	
				/* 2. here we need to rearrange the current element */
				m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it--);
				++m_pBuckets[index].beg;
			}
			else 
				/* 3. only update the iterators */
				--it;
		}

		/* 3. handle the last item */
		size_t index = m_Hash(it->first) % m_nTableSize;
		if(m_pBuckets[index].beg == m_pBuckets[index].end)
		{	
			/* 1. empty buckets */
			m_pBuckets[index].beg = m_pBuckets[index].end = it;
			m_pBuckets[index].end ++;
		}
		else if(--m_pBuckets[index].beg != it)
		{	
			/* 2. here we need to rearrange the current element */
			m_lItemList.splice(m_pBuckets[index].end, m_lItemList, it);
			++m_pBuckets[index].beg;
		}

		return true;
	}

public:
	CHashMap()
	{
		m_nTableSize = 97, m_nItemNum = 0;
		m_occupyRate = 0.8;
		
		m_pBuckets = new SBucket[ m_nTableSize ];
		assert(m_pBuckets);
		iterator end = m_lItemList.end();
		for(size_t i = 0; i < m_nTableSize; ++i)
			m_pBuckets[i].beg = m_pBuckets[i].end = end;
	}
	~CHashMap()
	{
		delete[] m_pBuckets;
	}
	iterator begin()
	{
		return m_lItemList.begin();
	}

	iterator end()
	{
		return m_lItemList.end();
	}

	size_t size()
	{
		return m_nItemNum;
	}

	iterator Insert(const T_Key & key, T_Value val)
	{
		size_t index = m_Hash(key) % m_nTableSize;
		SBucket *bucket = &m_pBuckets[index];
		iterator iter = bucket->beg;

		for (; iter != bucket->end; ++iter)
		{
			if (iter->first == key)
			{
				iter->second = val;
				return iter;
			}
		}

		if((double)(++m_nItemNum)/m_nTableSize > m_occupyRate)
		{
			Grow();
			index = m_Hash(key) % m_nTableSize;
			bucket = &m_pBuckets[index];	
		}

		if(bucket->beg == bucket->end)
		{
			bucket->end = bucket->beg = m_lItemList.insert(m_lItemList.begin(), ITEM_TYPE(key, val));
			return bucket->end++;
		}
		else
			return m_lItemList.insert(bucket->end, ITEM_TYPE(key, val));
	}

	
	iterator Find(const T_Key &key)
	{
		if(m_nItemNum == 0)
			return m_lItemList.end();

		size_t index = m_Hash(key) % m_nTableSize;
		SBucket &backet = m_pBuckets[index];
		iterator it = backet.beg;
		for(; it != backet.end ; ++it)
			if(it->first == key)
				return it;

		return m_lItemList.end();
	}
};


template <typename KEY,  typename HASHER>
class CNumberer
{
public:
	typename CHashMap<KEY,int, HASHER>::iterator Insert(const KEY &key)
	{
		int id = (int)m_map.size();
		typename CHashMap<KEY,int, HASHER>::iterator iter = m_map.Find(key);
			
		if (iter != m_map.end())
			return iter;
		else
		{
			iter = m_map.Insert(key, id);
			m_keyVec.push_back(&(iter->first));
			return iter;
		}
	}

	int GetId(const KEY &key, bool insertIfNotExist)
	{
		typename CHashMap<KEY, int, HASHER>::iterator iter = m_map.Find(key);
		if (iter == m_map.end())
		{
			if (insertIfNotExist == false)
				return -1;
			else
				iter = Insert(key);
		}
		return iter->second;
	}

	KEY  *GetKey(int id)
	{
		assert(id >=0 && id < (int)m_map.size());
		return m_keyVec[id];
	}

	int size()						{return (int)m_map.size();}
private:
	CHashMap<KEY, int, HASHER> m_map;
	vector<KEY *> m_keyVec;
};



#endif  /*__MAPS_H__*/
