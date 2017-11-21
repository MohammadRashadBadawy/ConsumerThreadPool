#pragma once
#include <Windows.h>
#include "CThread.h"
#include "CQueue.h"
#include <vector>

using namespace std;

typedef vector<CThread*>	ThreadList;

class CThreadsManager
{
	HANDLE				m_hThread;
	HANDLE				m_hStopEvent;
	HANDLE				m_hStopThreadsEvent;
	HANDLE				m_hCompletionEvent;
	bool				m_bRunning;
	unsigned int		m_uiRunningThreadsCounter;
	unsigned int		m_uiThreads;
	ThreadList			m_IdleThreadList;
	ThreadList			m_ActiveThreadList;
	CRITICAL_SECTION	m_MembersProtector;
protected:
	CQueue				m_WaitingQueue;

public:
	CThreadsManager(unsigned int uiThreads);
	~CThreadsManager();
	void Start();
	bool ProcessItemAsynchronous(CQueueItem* pItemToProcess, bool bHighPriority = false);
	bool ProcessItemSynchronous(CQueueItem* pItemToProcess, bool bHighPriority = false);

private:
	void Stop();
	static void ThreadMain(void *pParam);
	void CreateAndStartThreads();
	void StopAndDestroyThreads();
	void MoveCompletedThreadsToIdleList();
	void AssignWorkToIdleThreads();

protected:
	void Lock() { EnterCriticalSection(&m_MembersProtector); }
	void Unlock() { LeaveCriticalSection(&m_MembersProtector); }
	virtual CThread* CreateNewThread(int ThreadId, HANDLE* StopThreadsEvent, unsigned int* m_uiRunningThreadsCounter) = 0;
};
