#pragma once
#include <list>

using namespace std;

class CQueueItem
{
public:
	typedef enum { eNotStarted, eInProgress, eCompleted, eCancelled } States;
private:
	States	m_State;
public:
	CQueueItem();
	~CQueueItem();
	virtual wchar_t* GetKey() = 0;
	void ReSetWorkState() { m_State = eNotStarted; }
	void SetWorkStarted() { m_State = eInProgress; }
	void SetWorkComplete() { m_State = eCompleted; }
	void SetWorkCancelled() { m_State = eCancelled; }
	bool IsCompleted() { return (m_State == eCompleted); }
	bool IsCancelled() { return (m_State == eCancelled); }
};

typedef list<CQueueItem*>		ItemsQueue;
typedef ItemsQueue::iterator	ItemsQueueIter;
