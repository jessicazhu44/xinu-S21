/*Global variable for producer consumer*/
extern int n; /*this is just a declaration*/

/*function Prototype*/
void consumer(int count);
void producer(int count);

sid32 r_sem;
sid32 w_sem;
