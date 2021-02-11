#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

int readVal;

void consumer_bb(int id, int count) {
  // Iterate from 0 to count and for each iteration read the next available value from the global array `arr_q`
  // print consumer id (starting from 0) and read value as,
  // name : consumer_X, read : X
	for (int i =  0; i < count; i++) {
		wait(r_sem);
		wait(mutex);
		readVal = arr_q[tail];
		printf("name : consumer_%d,read : %d\n", id, readVal);
		tail =  (tail + 1) % 5;
		
		signal(mutex);
		signal(w_sem);

	}
}


void consumer(int count) {
  // reads the value of the global variable 'n'
  // 'count' times.
  // print consumed value e.g. consumed : 8
	for (int i = 0; i <= count; i++) {
		wait(r_sem);
		printf("consumed : %d\n", n);
		signal(w_sem);	
	}
}