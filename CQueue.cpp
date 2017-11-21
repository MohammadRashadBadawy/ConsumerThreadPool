#include "CQueue.h"


CQueue::CQueue()
{
	m_bComplete = false;
	InitializeCriticalSection(&m_ItemsProtector);
}


CQueue::~CQueue()
{
	EnterCriticalSection(&m_ItemsProtector);
	for (ItemsQueueIter iter = m_ItemsQueue.begin(); iter != m_ItemsQueue.end(); ++iter)
	{
		delete (*iter);
	}
	m_ItemsQueue.clear();
	LeaveCriticalSection(&m_ItemsProtector);

	DeleteCriticalSection(&m_ItemsProtector);
}

bool CQueue::Enqueue(CQueueItem* pItem, bool bHighPriority/* = false*/)
{
	if (pItem == NULL)
	{
		// Log error for having invalid parameter.
		fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Invalid parameter (NULL item) passed to Enqueue.\n", __FUNCTIONW__, __LINE__);
		return false;
	}

	EnterCriticalSection(&m_ItemsProtector);
	if (m_ItemsQueue.size() >= m_cstMaxQueueItems) // I used >= as a safety check. It is enough to check for == not >= 
	{
		// Log error for skipping this item and not inserting it to the queue to avoid running out of memory.
		// The caller is responsible for deallocating the pItem object because it is not inserted to the queue in this case, or retry inserting it later.
		fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Failed to enqueue item for processing '%s', because we reached the maximum allowed number of items in the queue (%d).\n",
			__FUNCTIONW__, __LINE__, pItem->GetKey(), (int)m_cstMaxQueueItems);
		return false;
	}

	if (bHighPriority)
		m_ItemsQueue.push_front(pItem);
	else
		m_ItemsQueue.push_back(pItem);
	LeaveCriticalSection(&m_ItemsProtector);
	return true;
}

CQueueItem* CQueue::PeekFront()
{
	CQueueItem*		pItem = NULL;

	EnterCriticalSection(&m_ItemsProtector);
	if (m_ItemsQueue.size() > 0)
		pItem = m_ItemsQueue.front();
	LeaveCriticalSection(&m_ItemsProtector);
	return pItem;
}

CQueueItem* CQueue::Dequeue()
{
	CQueueItem* pItem = NULL;

	EnterCriticalSection(&m_ItemsProtector);
	if (m_ItemsQueue.size() > 0)
	{
		ItemsQueueIter	Iter = m_ItemsQueue.begin();
		pItem = *Iter;
		m_ItemsQueue.erase(Iter);
	}
	LeaveCriticalSection(&m_ItemsProtector);
	return pItem;
}

size_t CQueue::Size()
{
	size_t	stCount = 0;

	EnterCriticalSection(&m_ItemsProtector);
	stCount = m_ItemsQueue.size();
	LeaveCriticalSection(&m_ItemsProtector);
	return stCount;
}