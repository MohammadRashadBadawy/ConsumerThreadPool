#include "CThread.h"

CThread::CThread(int iId, HANDLE* phStopEvent, unsigned int* pCounter)
{
	DWORD	dwThreadId;

	m_iId = iId;
	m_phStopEvent = phStopEvent;
	m_puiThreadsCounter = pCounter;
	m_State = eIdle;
	m_bRunning = false;
	m_pItem = NULL;
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadMain, this, 0, &dwThreadId);

	if (m_hThread == NULL)
	{
		// Log error about the failure of creating the thread.
		fwprintf(stderr, L"Method(%s):Line(%d)ERROR: Failed to create processing thread(id: %d)\n", __FUNCTIONW__, __LINE__, iId);
		m_State = eDead;
	}
}


CThread::~CThread()
{
	if (m_hThread)
		CloseHandle(m_hThread);
}

//--------------------------------------------------------------------------------------------------
/*!
* This is the thread main method of the processing thread.
*
* @ingroup CThread
*
* @param pParam : IN - void pointer to this object (type cast it to CThread* to use it).
*
* @return void.
*/
void CThread::ThreadMain(void *pParam)
{
	bool			bDone = false;
	CThread			*pThis = (CThread*)pParam;

	InterlockedIncrement(pThis->m_puiThreadsCounter);
	pThis->m_bRunning = true;

	while (!bDone)
	{
		// Does this thread have work to do?
		if (pThis->IsActive())
		{
			if (pThis->m_pItem != NULL)
			{
				// Process the item assigned to this thread.
				pThis->m_pItem->SetWorkStarted();
				pThis->ProcessItem(pThis->m_pItem);
				pThis->m_pItem->SetWorkComplete();

				// NOTE: The owner of this item is responsible for monitoring its state, to be able to de-allocate it after it is processed.
				// It is NOT de-allocated here.
				pThis->m_pItem = NULL;
			}
			else
				pThis->m_State = eIdle;
		}

		// Check if the stop event was signaled
		if (WaitForSingleObject(*(pThis->m_phStopEvent), pThis->IsIdle() ? 500 : 0) == WAIT_OBJECT_0)
		{
			// Log information message here about detecting stop event.
			bDone = true;
		}
	}
	pThis->m_bRunning = false;
	InterlockedDecrement(pThis->m_puiThreadsCounter);
}
