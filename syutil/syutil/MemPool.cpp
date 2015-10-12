//
//  MemPool.cpp
//  syutil
//
//  Created by LiSiyuan on 15/10/12.
//  Copyright (c) 2015年 soya. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include "MemPool.h"

#pragma pack(push, 1)

#define NEXT_BLOCK(a) ((memHeader*)(((char*)(a))+sizeof(*(a))+(a)->m_size))
#define PTR_TO_HEADER(a) ((memHeader*)(((char*)(a)-sizeof(memHeader))))
#define DATA(a) ((Ptr)(((char*)a)+sizeof(memHeader)))
#define FIRST_BLOCK ((memHeader*)m_data)

#pragma mark • Create / Destroy

using namespace sy;

CMemoryPool::CMemoryPool(unsigned long size, bool useSysMem)
{	
	if(size == 0)
	{
		m_userData=NULL;
		m_size = 0;
		
		m_spaceleft=0;
		m_maxblock=0;
		m_minblock=0;
		m_amountallocated=NULL;
		m_numberofblocks=0;
	}
	else
	{
		Init(size, useSysMem);
	}
}

CMemoryPool::~CMemoryPool()
{
	Close();
}

bool CMemoryPool::Init(unsigned long size, bool useSysMem)
{
#pragma unused(useSysMem)
	
	m_data = (char*)malloc(size);
	if(!m_data)
		return false;
	
	m_size = size;
	SetHeader(FIRST_BLOCK);
	
	FIRST_BLOCK->m_size = size-sizeof(memHeader);
	FIRST_BLOCK->m_end = true;
	
	m_userData=NULL;
	
	m_spaceleft=size-sizeof(memHeader);
	m_maxblock=m_spaceleft;
	m_minblock=0;
	m_amountallocated=NULL;
	m_numberofblocks=1;
	
	return true;
}

bool CMemoryPool::Close()
{
	if(m_data != NULL)
	{
		free(m_data);
		m_data=NULL;
	}
	
	m_size=0;
	
	m_spaceleft=0;
	m_maxblock=0;
	m_minblock=0;
	m_amountallocated=NULL;
	m_numberofblocks=0;
	
	return false;
}

#pragma mark • Block Info

unsigned long CMemoryPool::GetSize(Ptr p)
{
	if(!p) return(NULL);
	return( PTR_TO_HEADER(p)->m_size-PTR_TO_HEADER(p)->m_pad);
}

Ptr CMemoryPool::NextBlock(Ptr p)
{
	memHeader *m;
	if(!m_data) return(NULL);
	if(!m_size) return(NULL);
	
	if(!p) 
	{
		m=FIRST_BLOCK;
		if(m->m_used) return(DATA(m));
	}
	else
		m=PTR_TO_HEADER(p);
	
	if(!m) return(NULL);
	
	if(m->m_end) return(NULL);
	
	while(1)
	{
		m=NEXT_BLOCK(m);
		
		if(m->m_used) return(DATA(m));
		if(m->m_end) break;
	}
	
	return(NULL);
}

#pragma mark • Alloc/ Dealloc

// all of these are inturrupt safe (that's the point)
Ptr CMemoryPool::CAlloc(unsigned long t)
{
	memHeader *m=FIRST_BLOCK;
	
	if(!m_data) return NULL;
	if(!m_size) return NULL;
	
	Check();
	
	t = (t + 3L)&0xFFFFFFFC;
	while((m->m_size<t) || (m->m_used==true)) 
	{
		if(m->m_end) 
		{ 
			return NULL; 
		}
		
		m = NEXT_BLOCK(m);
	}
	
	SplitBlock(m, t);
	m->m_used = true;
	
	Check();	

	return(DATA(m));
}

void CMemoryPool::FreeAllBlocks()
{
	SetHeader(FIRST_BLOCK);
	
	FIRST_BLOCK->m_size=m_size-sizeof(memHeader);
	FIRST_BLOCK->m_end=true;
	
	m_userData=NULL;
	
	m_spaceleft=m_size-sizeof(memHeader);
	m_maxblock=m_spaceleft;
	m_minblock=0;
	m_amountallocated=NULL;
	m_numberofblocks=1;
}

void CMemoryPool::CFree(Ptr y)
{	
	if(!y) return;
	if(!m_data) return;
	if(!m_size) return;
	
	memHeader *m=PTR_TO_HEADER(y);
	
	m->m_used=false;
	m_spaceleft+=m->m_size;
	
	Check();
}

void CMemoryPool::sCFree(Ptr y)
{
	if(!y) return;
	
	memHeader *m=PTR_TO_HEADER(y);
	
	m->m_used=false;
}

// turns m into a block of size newsize
// and might turn the rest of the block into another free one
void CMemoryPool::SplitBlock(memHeader *m, unsigned long newsize)
{
	if(m->m_size-newsize<sizeof(memHeader))
	{
		// we can't split the block
		// Add some slack
		m->m_pad=m->m_size-newsize;
		return;
	}
	
	memHeader *next=(memHeader *) (((char *)m)+sizeof(memHeader)+newsize);
	
	next->m_size=m->m_size-sizeof(memHeader)-newsize;
	next->m_used=false;
	next->m_aux=false;
	next->m_end=m->m_end;
	next->m_pad=0;
	
	m->m_end=false;
	m->m_size=newsize;	
	m->m_aux=false;
	m->m_pad=0;
}

// Amalgamate consecutive free blocks 
#pragma mark • HouseKeeping
void CMemoryPool::Check()
{
	if(!m_data) return;
	if(!m_size) return;
	
	memHeader *m,*next;
	
	m=FIRST_BLOCK;
	
	m_maxblock=0;
	m_amountallocated=NULL;
	m_numberofblocks=0;
	m_minblock=m_size+1;
	unsigned long oldminblock=m_minblock;
	m_spaceleft = 0;
	
	while(1)
	{
		m_numberofblocks++;
		
		// Size check
		if(!m->m_used)
		{
			m_spaceleft+=m->m_size;
			
			if(m->m_size>m_maxblock) m_maxblock=m->m_size;
			if(m->m_size<m_minblock)
			{
				oldminblock=m_minblock;
				m_minblock=m->m_size;
			}
		}
		else
			m_amountallocated+=m->m_size;
		
		if(m->m_end)
			return;
		
		// we know m isn't the m_end so we can legitimatly call NEXT_BLOCK
		next=NEXT_BLOCK(m);
		if(!next->m_used)
		{
			// the following block is unused
			// expand it by amalgamating slack bytes at start and finish
			
			// remove slack from start
			if(m->m_pad)
			{
				char *s,*d;
				
				next->m_size+=m->m_pad;
				m->m_size-=m->m_pad;		// move m->m_pad bytes from m to next
				
				s=(char *)next;
				d=((char *)next)-m->m_pad;
				
				// move next memHeader back by m->m_pad bytes
				for(unsigned int k=0;k<sizeof(memHeader);k++)	d[k]=s[k];
				
				next=NEXT_BLOCK(m);
				
				m->m_pad=0;
			}
			if(next->m_pad)
			{
				next->m_pad=0;
			}
		}
		
		if((!m->m_used)&&(!next->m_used))
		{
			// m is going to be added to next
			m_spaceleft-=m->m_size;
			m_numberofblocks--;
			m_minblock=oldminblock;
			
			m->m_size+=sizeof(memHeader)+next->m_size;
			if(next->m_end)
			{
				m->m_end=true;
			}
		}
		else
			m=next;
	}
	
	return;
}

#pragma pack(pop)