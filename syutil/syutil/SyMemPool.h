//
//  MemPool.h
//  syutil
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//

#ifndef __WBX_CATMEMPOOL_H__
#define __WBX_CATMEMPOOL_H__

#include <MacTypes.h>
#include "SyDebug.h"
#pragma pack(push, 1)

#define CPOOL_DEFAULT 	0

START_UTIL_NS

typedef struct _memHeader
{
	unsigned long m_size;
	unsigned char m_pad;
	unsigned char m_aux;
	unsigned char m_end;
	unsigned char m_used;
} memHeader;

class CSyMemoryPool
{
public:
	CSyMemoryPool(unsigned long size = CPOOL_DEFAULT,
				  bool useSysMem = false);
	virtual ~CSyMemoryPool();
	
	bool Init(unsigned long size, bool useSysMem = false);
	bool Close();
	
	Ptr CAlloc(unsigned long );
	Ptr CReAlloc(Ptr ,unsigned long);
	void CFree(Ptr);
	void sCFree(Ptr y);
	void SplitBlock(memHeader *m, unsigned long newsize);
	Ptr NextBlock(Ptr p);
	static unsigned long GetSize(Ptr );
	void Check();
	void FreeAllBlocks();
	
	void SetHeader(memHeader *m)
	{
		m->m_aux=m->m_end=m->m_used=m->m_pad=0;
		m->m_size=0;
	};
	
	long m_userData;
	
	Ptr m_data;
	unsigned long m_size;
	
	unsigned long m_spaceleft;
	unsigned long m_maxblock;
	unsigned long m_amountallocated;
	unsigned long m_numberofblocks;
	unsigned long m_minblock;
};

END_UTIL_NS

#pragma pack(pop)

#endif
