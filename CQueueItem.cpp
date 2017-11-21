#include "CQueueItem.h"
#include <stdexcept>

using namespace std;

CQueueItem::CQueueItem()
{
	m_State = eNotStarted;
}

CQueueItem::~CQueueItem()
{
}