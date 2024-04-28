list team members, document who did what, discuss
anything interesting about your implementation.

Team members: Cassie Zhang xzhan304, Tianai Yue tyue4

Milestone 1
Both Cassie and Tianai did relatively equal amounts of work and effort.
Tianai implemented message, table, value_stack, and debug for all the functions.
Cassie implemented message_serialization, get_value, set_value, incr_value, and debug for all the functions.

Milestone 2

1. Data Structures Resembling Concurrency.

Tables Map: The map that is shared by different slipstreams naturally contains those tables and data.
Transactions Map: This mechanism records active movements and identifiers of the Clients, and compares them to ensure order and preventing problems with race conditions.
Transaction Locks Map: Guarantees two changes will not adapt the same table simultaniously, for it is what it is called table tracks.


2. Synchronization Mechanism

Mutex Locks (pthread_mutex_t): Adjustable to protect mutual seclusion for modifying attempts or accessing the Tables Map.
Try-Lock Mechanism (pthread_mutex_trylock): Prevented deadlocks when performing transactions by not blocking when some resource was already lockede by other transaction.
Condition Variables: Designed with thread management features to handle situations like table availability to save resource wastage.

3. Confidence in Absence of Race Conditions and Deadlocks
Syncronization is what we used to avoid the occurrence of race conditions and deadlocks, so the server will work perfectly. Encrypted some important part (liek the 
shared Tables Map and transaction handle) by mutex so that operations would be done atomically in one transaction, and there are no race conditions. In order not to 
arise deadlocks, we are systematically following a specific lock order and using a try lock mechanism. This kind of technique permits failures of transactions in case 
locking of the resources is not right away available, and not spend an unlimited time waiting for the resource. We loaded the server with high loads by having many 
clients using the server at the same time, and we also made more detailed code reviews on aspects that may cause problems in the multi-thread situations. This approach 
is methodological and thus has a mixture of preventive design solutions and reactivity tests in it. The demonstration of the race conditions and deadlocks handling is 
the important point that shows the server's ability.