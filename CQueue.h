#pragma once
#include <Windows.h>
#include "CQueueItem.h"

class CQueue
{
	bool				m_bComplete;
	const size_t		m_cstMaxQueueItems = 100000; // This is just a simple solution to avoid running out of memory when we have huge number of input data
												   // items that need to be processed. In real product, I will use Virtual Memory to dump queue items on
												   // disk files when they reach a maximum threshold (during enqueue), and then I'll load those items from
												   // disk back to memory when the number of items in queue in memory reach a minimum threshold (during dequeue).
	ItemsQueue			m_ItemsQueue;
	CRITICAL_SECTION	m_ItemsProtector;
public:
	CQueue();
	~CQueue();
	bool Enqueue(CQueueItem* pItem, bool bHighPriority = false);
	CQueueItem* Dequeue();
	CQueueItem* PeekFront();
	size_t Size();
	void SetWorkComplete(bool bComplete) { m_bComplete = bComplete; }
	bool IsWorkComplete() { return m_bComplete; }
};

typedef list<CQueue*>			QueueList;
typedef QueueList::iterator		QueueListIter;
