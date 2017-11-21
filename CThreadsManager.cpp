#include <stdexcept>
#include "CThreadsManager.h"

#define MAX_THREADS_COUNT		1000
#define DEFAULT_THREADS_COUNT	100

using namespace std;

CThreadsManager::CThreadsManager(unsigned int uiThreads)
{
	if ((uiThreads == 0) || (uiThreads > MAX_THREADS_COUNT))
	{
		// Log warning here about invalid number of threads input parameter, and set it to the default.
		fwprintf(stderr, L"Method(%s):Line(%d)WARNING: Invalid number of threads (%u) was passed-in to CThreadsManager constructor. The default number (%u) will be used instead.\n",
			__FUNCTIONW__, __LINE__, uiThreads, DEFAULT_THREADS_COUNT);
		uiThreads = DEFAULT_THREADS_COUNT;
	}
	m_uiThreads = uiThreads;
	m_hThread = NULL;
	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hStopThreadsEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hCompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_bRunning = false;
	m_uiRunningThreadsCounter = 0;
	m_uiThreads = uiThreads;

	// No exception handling for InitializeCriticalSection, because In this program I neither support Windows Server 2003 nor Windows XP.
	// The following is written on MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/ms683472(v=vs.85).aspx
	// Windows Server 2003 and Windows XP : In low memory situations, InitializeCriticalSection can raise a STATUS_NO_MEMORY exception.
	// This exception was eliminated starting with Windows Vista.
	InitializeCriticalSection(&m_MembersProtector);
}

CThreadsManager::~CThreadsManager()
{
	Stop();

	CloseHandle(m_hStopEvent);
	CloseHandle(m_hStopThreadsEvent);
	CloseHandle(m_hCompletionEvent);

	if (m_hThread != NULL)
		CloseHandle(m_hThread);

	DeleteCriticalSection(&m_MembersProtector);
}

//--------------------------------------------------------------------------------------------------
/*!
* This method starts the main thread of the thread pool manager.
*
* @ingroup : CThreadsManager
*
* @param none.
*
* @return void.
*/
void CThreadsManager::Start()
{
	DWORD	dwThreadId;

	// Reset the events.
	ResetEvent(m_hStopEvent);
	ResetEvent(m_hStopThreadsEvent);
	ResetEvent(m_hCompletionEvent);

	// Create the main thread of the thread pool manager and return back to the caller
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadMain, this, 0, &dwThreadId);

	if (m_hThread == NULL)
	{
		// Log error for failure of creating the main thread of the thread pool manager
		// You may also need to exit the application if the thread pool is a critical component in it and the application will not function properly without it.
		fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Failed to create the main thread of the thread pool manager. The application will now exit.\n", __FUNCTIONW__, __LINE__);
		exit(0);
	}
}

//--------------------------------------------------------------------------------------------------
/*!
* This method stops the main thread of the thread pool manager.
*
* @ingroup : CThreadsManager
*
* @param none
*
* @return void.
*/
void CThreadsManager::Stop()
{
	SetEvent(m_hStopEvent);

	// Wait 10 seconds for the main thread to complete execution and get closed along with all threads in the thread pool
	// associated with this manager. If  m_hCompletionEvent hasn't got signaled after 10 seconds, proceed.
	WaitForSingleObject(m_hCompletionEvent, 10000);
}

//--------------------------------------------------------------------------------------------------
/*!
* This is the thread main method of the thread pool manager, which is responsible for managing the thread pool.
*
* @ingroup : CThreadsManager
*
* @param pParam : IN - void pointer to this object (type cast it to CThreadsManager* to use it).
*
* @return void.
*/
void CThreadsManager::ThreadMain(void *pParam)
{
	bool					bDone = false;
	CThread*				pThread = NULL;
	CQueue*					pQueue = NULL;
	ThreadList::iterator	iter;
	CThreadsManager*		pThis = (CThreadsManager*)pParam;

	pThis->m_uiRunningThreadsCounter = 0;
	pThis->m_bRunning = true;

	// Create and start the processing threads.
	pThis->CreateAndStartThreads();

	while (!bDone)
	{
		// Check the state of all threads in the thread pool, then move the threads that have completed their work from the active list to the idle list.
		pThis->MoveCompletedThreadsToIdleList();

		// Assign work to all idle threads.
		pThis->AssignWorkToIdleThreads();

		// Service threads and queues every 1 second.
		if (WaitForSingleObject(pThis->m_hStopEvent, 1000) == WAIT_OBJECT_0)
			bDone = true;
	}

	// Stop and destroy the threads in the thread pool.
	pThis->StopAndDestroyThreads();
	pThis->m_bRunning = false;

	SetEvent(pThis->m_hCompletionEvent);
}

//--------------------------------------------------------------------------------------------------
/*!
* This method creates the processing threads and starts them, and put them all in the idle threads list.
*
* @ingroup CThreadsManager
*
* @param pParam none.
*
* @return void.
*/
void CThreadsManager::CreateAndStartThreads()
{
	for (unsigned int i = 0; i < m_uiThreads; i++)
	{
		CThread	*pThread = CreateNewThread(i, &(m_hStopThreadsEvent), &(m_uiRunningThreadsCounter));

		if (pThread->IsDead())
		{
			delete pThread;
			// Log error message here about failing to create a processing thread(id) due to running out of memory
			fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Not enough memory for allocating new processing thread(id: %d).\n", __FUNCTIONW__, __LINE__, i);
		}
		else
		{
			m_IdleThreadList.push_back(pThread);
		}
	}

	if (m_IdleThreadList.size() == 0)
	{
		// Log error here for failure of creating at least one processing thread.
		// You may also need to exit the application if the thread pool is critical part in it, and the application will not function properly without it.
		fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Failed to create processing threads.\n", __FUNCTIONW__, __LINE__);
	}
}

//--------------------------------------------------------------------------------------------------
/*!
* This method stops the destroys the processing threads.
*
* @ingroup CThreadsManager
*
* @param pParam none.
*
* @return void.
*/
void CThreadsManager::StopAndDestroyThreads()
{
	DWORD					dwWaitTrials = 0;
	ThreadList::iterator	ThreadIter;
	CThread*				pThread;
	QueueListIter			QueueIter;

	SetEvent(m_hStopThreadsEvent);

	// Give the threads 10 seconds to stop.
	while ((m_uiRunningThreadsCounter > 0) && (dwWaitTrials++ < 100))
		Sleep(100);

	Lock();
	for (ThreadIter = m_IdleThreadList.begin(); ThreadIter != m_IdleThreadList.end(); ++ThreadIter)
	{
		pThread = (*ThreadIter);

		if (pThread->IsRunning())
		{
			// Log information message about stopping thread (id) timed-out. The thread will be forced to terminate.

			if (TerminateThread(pThread->GetThreadHandle(), 0) == FALSE)
			{
				// Log information message here about failing to terminate worker thread (id)
			}
		}
		// Log information message about deleting thread(id)
		delete pThread;
	}
	m_IdleThreadList.clear();

	for (ThreadIter = m_ActiveThreadList.begin(); ThreadIter != m_ActiveThreadList.end(); ++ThreadIter)
	{
		pThread = (*ThreadIter);

		if (pThread->IsRunning())
		{
			// Log information message here about stopping thread (id) timed-out. The thread will be forced to terminate.

			if (TerminateThread(pThread->GetThreadHandle(), 0) == FALSE)
			{
				// Log information message here about failing to terminate worker thread (id)
			}
		}
		// Log information message here about deleting thread(id)
		delete pThread;
	}
	m_ActiveThreadList.clear();
	m_uiRunningThreadsCounter = 0;
	Unlock();
}

//--------------------------------------------------------------------------------------------------
/*!
* This method loops through the active threads list, and moves all threads that finished their work
* and became idle threads into the idle threads list, to be able to have work to do again.
*
* @ingroup CThreadsManager
*
* @param pParam none.
*
* @return void.
*/
void CThreadsManager::MoveCompletedThreadsToIdleList()
{
	ThreadList::iterator	ThreadIter;
	CThread*				pThread;
	bool					bFound = false;

	Lock();
	ThreadIter = m_ActiveThreadList.begin();

	while (ThreadIter != m_ActiveThreadList.end())
	{
		pThread = (*ThreadIter);

		if (pThread->IsIdle())
		{
			ThreadIter = m_ActiveThreadList.erase(ThreadIter);
			pThread->SetIdle();
			m_IdleThreadList.push_back(pThread);
			// Log information message about moving this thread(id) from active to idle list.
		}
		else
			++ThreadIter;
	}
	Unlock();
}

//-----------------------------------------------------------------------------------------------
/*!
* This method determines if there is another item to be processed. If so, it looks for an idle
* thread and queue and gets the idle thread working on the item.
*
* @ingroup : CThreadsManager
*
* @param none
*
* @return void  :
*/
void CThreadsManager::AssignWorkToIdleThreads()
{
	QueueListIter			QueueIter;
	ThreadList::iterator	ThreadIter;
	CThread*				pThread;
	CQueueItem*				pQItem;

	Lock();

	// While there is at least one idle thread and at least one item in the queue waiting to be translated, ...
	while ((m_IdleThreadList.size() > 0) && (m_WaitingQueue.Size() > 0))
	{
		// Get an item from the waiting list (queue).
		pQItem = m_WaitingQueue.Dequeue();

		// Get the next idle thread.
		ThreadIter = m_IdleThreadList.begin();
		pThread = *ThreadIter;
		m_IdleThreadList.erase(ThreadIter);

		// Assign the item to the thread and set the thread state to active.
		pThread->SetItem(pQItem);
		pThread->SetActive();

		// Add the thread to the active threads list.
		m_ActiveThreadList.push_back(pThread);
	}
	Unlock();
}

//--------------------------------------------------------------------------------------------------
/*!
* This method adds a new item to the waiting queue, to be processed by the processing threads.
* NOTE: The caller of this method is responsible for monitoring the state of the item, to be able
* to de-allocate it after it is processed.
*
* @ingroup CThreadsManager
*
* @param pItemToProcess : The item needs to be processed asynchronously.
* @param bHighPriority : The priority of the item that needs to be processed. If it is high priority, then it
*	will be pushed to the front of the waiting list. Otherwise, it is pushed to the end of the waiting
*	list by default.
*
* @return void.
*/
bool CThreadsManager::ProcessItemAsynchronous(CQueueItem* pItemToProcess, bool bHighPriority/* = false*/)
{
	if (pItemToProcess == NULL)
	{
		fwprintf(stderr, L"Method(%s):Line(%d)WARNING: Invalid item is skipped.\n", __FUNCTIONW__, __LINE__);
		return false;
	}

	bool	bResult;

	Lock();
	bResult = m_WaitingQueue.Enqueue(pItemToProcess, bHighPriority);
	Unlock();

	return bResult;
}

//--------------------------------------------------------------------------------------------------
/*!
* This method adds a new item to the waiting queue, to be processed by the processing threads.
* NOTE: The caller of this method is responsible for de-allocating the item after it is processed.
*
* @ingroup CThreadsManager
*
* @param pItemToProcess : The item needs to be processed synchronously.
* @param bHighPriority : The priority of the item that needs to be processed. If it is high priority, then it
*	will be pushed to the front of the waiting list. Otherwise, it is pushed to the end of the waiting
*	list by default.
*
* @return void.
*/
bool CThreadsManager::ProcessItemSynchronous(CQueueItem* pItemToProcess, bool bHighPriority/* = false*/)
{
	if (pItemToProcess == NULL)
	{
		fwprintf(stderr, L"Method(%s):Line(%d)WARNING: Invalid item is skipped.\n", __FUNCTIONW__, __LINE__);
		return false;
	}

	bool	bResult;

	Lock();
	bResult = m_WaitingQueue.Enqueue(pItemToProcess, bHighPriority);
	Unlock();

	while (!pItemToProcess->IsCompleted())
	{
		// Check the item status every half a second.
		if ((WaitForSingleObject(m_hStopEvent, 500) == WAIT_OBJECT_0) || pItemToProcess->IsCancelled())
			break;
	}

	return bResult;
}