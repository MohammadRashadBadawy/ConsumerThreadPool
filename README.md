# Consumer Thread Pool
Base classes for consumer thread pool. These classes can be inherited by children classes to implement any thread pool that process items in a queue.
CQueueItem class should be inherited by the class that represents the item that will be inserted into the queue for processing.
CThread class should be inherited by the class that implements the required processing, that needs to be done on the objects of CQueueItem child class.
CThreadsManager class represents the thread pool manager, and it should be inherited by the class that creates instances of the child class of CThread.
