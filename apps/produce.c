#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>
// where to use mutex?
void producer_bb(int id, int count) {
  // Iterate from 0 to count and for each iteration add iteration value to the global array `arr_q`, 
  // print producer id (starting from 0) and written value as,
  // name : producer_X, write : X
    for(int i = 0; i <= count; i++) {
        wait(w_sem);
        wait(mutex);
        arr_q[head] = i;
        printf("name : producer_%d,write : %d\n", id, i);
        head = (head + 1) % 5;
        signal(mutex);
        signal(r_sem);
        
    }
}


void producer(int count) {
    // Iterates from 0 to count, setting
    // the value of the global variable 'n'
    // each time.
    //print produced value e.g. produced : 8
    for (int i = 0; i <= count; i++) {
    	wait(w_sem);
    	n = i;
    	printf("produced : %d\n", n);
    	signal(r_sem);
    }
}