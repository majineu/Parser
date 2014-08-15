#pragma once
#include<list>
#include<cstdlib>
#include"Macros.h"
using std::list;
class CPool
{
	struct SBlock
	{
		char *m_pData;
		size_t m_nBlkSize, m_nUsed;
	};
	const static size_t BLOCK_SIZE = 40960; 
public:
	CPool(void):pCurBlock(NULL){}
	~CPool(void);
	void * Allocate(size_t size);
	void Recycle();
	void Free();
	void Test();
private:
	SBlock * allocateBlock(size_t size)
	{
		size_t allocSize = size > BLOCK_SIZE ? size : BLOCK_SIZE;
		SBlock * pBlock = (SBlock *)std::malloc(allocSize + sizeof(SBlock));
		CHK_NEW_FAILED(pBlock);
		pBlock->m_pData = (char*)((SBlock*)pBlock + 1);
		pBlock->m_nUsed = 0;
		pBlock->m_nBlkSize = allocSize;
		return pBlock;
	}


	list<SBlock *>    m_listUsed;
	list<SBlock *>    m_listFree;

	list<SBlock *>::iterator  it;
	list<SBlock *>::iterator  end;
	SBlock * pCurBlock;
};

