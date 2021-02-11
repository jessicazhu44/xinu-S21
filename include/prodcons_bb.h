// declare globally shared array
extern int32 arr_q[5];
// declare globally shared semaphores
sid32 r_sem;
sid32 w_sem;
sid32 mutex;
// declare globally shared read and write indices
extern int32 head;	// write
extern int32 tail; // read
// function prototypes
void consumer_bb(int id, int count);
void producer_bb(int id, int count);