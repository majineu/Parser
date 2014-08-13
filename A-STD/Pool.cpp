#include "Pool.h"
#include <ctime>
#include <vector>
CPool::~CPool(void)
{
	Free();
}

void * CPool::Allocate(size_t size)
{
	if(size <= 0)
	{
		printf("Error allocate block with size %lu\n", size);
		return NULL;
	}

	size = ((size + 3)/4)*4;
	if(pCurBlock == NULL || pCurBlock->m_nBlkSize <= size + pCurBlock->m_nUsed)
	{	/* a new block is needed */
		it = m_listFree.begin(), end = m_listFree.end();
		for(; it != end; ++it)
		{	/* find a block big enough */
			if((*it)->m_nBlkSize > size)
			{
				pCurBlock = *it;
				m_listFree.erase(it);
				break;
			}
		}

		if(pCurBlock == NULL || pCurBlock->m_nUsed + size >= pCurBlock->m_nBlkSize)
			pCurBlock = allocateBlock(size);
		m_listUsed.push_back(pCurBlock);
	}

	/* return memory */
	pCurBlock->m_nUsed += size;
	return pCurBlock->m_pData + pCurBlock->m_nUsed - size ;
}


void CPool::Free()
{
	while(!m_listUsed.empty())
	{
		free(m_listUsed.front());
		m_listUsed.pop_front();
	}
	while(!m_listFree.empty())
	{
		free(m_listFree.front());
		m_listFree.pop_front();
	}
}


void CPool::Recycle()
{
	it  = m_listUsed.begin();
	end = m_listUsed.end();
	for (; it != end; ++it)
		(*it)->m_nUsed = 0;
	m_listFree.splice(m_listFree.begin(),  m_listUsed);
	pCurBlock = NULL;
}

void CPool::Test()
{
	
	int iterNum = 1000000;

	std::vector<char*> pVec(iterNum);
	char * pCur;
	clock_t start = clock();
	for(int k = 0; k < 0; ++k)
	{
		for(int i = 0; i < iterNum; ++i)
		{
			pCur = (char*) malloc(10);
			strcpy(pCur, "hello");
			pVec[i] = pCur;
		}
		for(int i = 0; i < iterNum; ++i)
			delete pVec[i];
	}
	printf("time eclipsed %ld\n", clock() - start);
	system("pause");
	start = clock();
	for(int k = 0; k < 40; ++k)
	{
		for(int i = 0; i < iterNum; ++i)
		{
			pCur = (char*) Allocate(10);
			strcpy(pCur, "hello");
			pVec[i] = pCur;
		}
		Recycle();
	}
	Free();
	pVec.clear();
	printf("time eclipsed %ld\n", clock() - start);
	system("pause");
}


