#pragma once
#include "CQueue.h"

class CThread
{
public:
	typedef enum { eActive, eIdle, eDead } States;

private:
	int				m_iId;
	HANDLE			m_hThread;
	bool			m_bRunning;
	States			m_State;
	unsigned int*	m_puiThreadsCounter; // Pointer to unsigned int member variable in CQueue, which holds the number of threads working together on the that queue.
	HANDLE*			m_phStopEvent;
	CQueueItem*		m_pItem;

public:
	bool IsDead() { return m_State == eDead; }
	bool IsIdle() { return m_State == eIdle; }
	bool IsActive() { return m_State == eActive; }
	void SetIdle() { m_State = eIdle; }
	void SetActive() { m_State = eActive; }
	bool IsRunning() { return m_bRunning; }
	int GetThreadId() { return m_iId; }
	HANDLE GetThreadHandle() { return m_hThread; }
	CQueueItem* GetItem() { return m_pItem; }
	void SetItem(CQueueItem* pItem) { m_pItem = pItem; }
	CThread(int iId, HANDLE* phStopEvent, unsigned int* pCounter);
	virtual ~CThread();

private:
	static void __stdcall ThreadMain(void *pParam);

protected:
	virtual void ProcessItem(CQueueItem* pQItem) = 0;
};

