#include <xinu.h>
#include <stdio.h>
#include <string.h>
#include <future.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  intmask mask;
  mask = disable();

  if (size == 0 || nelem == 0) {
  	restore(mask);
  	return (future_t*) SYSERR;
  }

  future_t *f = (future_t*) getmem(sizeof(future_t)); // this returns an address 'return (char *)(curr); ''

  // struct future_t *future1;
  f->data = (char *) getmem(size*nelem);
  f->state = FUTURE_EMPTY; 
  f->size = size;
  f->pid = -1;
  f->mode = mode;
  f->get_queue = newqueue();
  f->set_queue = newqueue();
  f->max_elems = nelem;
  f->count = 0;
  f->head = 0; // index of slot at which the next element will be removed
  f->tail = 0; //  index of next slot to which the next element will be added
  
  restore(mask);
  return f;
}

 // write your code here for future_free, future_get and future_set
	syscall future_free(future_t* f) {
		// free allocated 'future' use freemem
		// return syscall, either SYSERR or OK
		// kill() any process thats waiting for the future, use 'pid'
		if (f->pid > 0) {
			kill(f->pid);
		} 

		int m = 1;
		if (f->mode == FUTURE_SHARED) { 
			m = delqueue(f->get_queue);
		}
		return freemem((char*)f, f->size) && m; 
	} 												// is this how to free queue?

	syscall future_get(future_t* f,  char* out) {
		// returns SYSERR if another process is already waiting on the target future
		//printf("f.pid result: %s\n", f->pid);
		intmask mask;
  		mask = disable();

		char* headelemptr;
		f->pid = getpid();

		if (f->pid > 0 && f->mode == FUTURE_EXCLUSIVE) {   
			restore(mask);
			return SYSERR;
		}
		
		if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_WAITING;
			enqueue(f->pid, f->get_queue);
			suspend(f->pid);
			headelemptr = f->data + (f->head * f->size);
			memcpy(out, headelemptr, f->size);
			f->head = (f->head + 1) % f->max_elems;
			f->count = f->count - 1; // or f->count = 0
			// resume(dequeue(f->set_queue)); 
			// kprintf("step 4\n");

			restore(mask);
			return OK;
		}

		if (f->state == FUTURE_WAITING) {
			
			if(f->mode == FUTURE_EXCLUSIVE) {
				restore(mask);
				return SYSERR;
			}
			enqueue(f->pid, f->get_queue);
			suspend(f->pid);
			// f->pid = getpid();
			headelemptr = f->data + (f->head * f->size);
			memcpy(out, headelemptr, f->size);
			f->head = (f->head + 1) % f->max_elems; 
			f->count = f->count - 1;
			// resume(dequeue(f->set_queue)); 

			restore(mask);
			return OK;
		}

		if(f->state == FUTURE_READY) {
			// when it is FUTURE_READY, the next future_get() call return set value 
			// and the future becomes EMPTY
			f->pid = getpid();
			if(f->count <= 0) { // queue is empty
				
				// kprintf("step 5\n");
				enqueue(f->pid, f->get_queue);
				suspend(f->pid);
				headelemptr = f->data + (f->head * f->size);
				memcpy(out, headelemptr, f->size);	
				f->head = (f->head + 1) % f->max_elems;
				f->count = f->count - 1;		

				restore(mask);
				return OK;
			} 
			// kprintf("step 6\n");
			headelemptr = f->data + (f->head * f->size);
			memcpy(out, headelemptr, f->size);
			resume(dequeue(f->get_queue));
			f->head = (f->head + 1) % f->max_elems;
			f->count = f->count - 1;
			// resume(dequeue(f->set_queue)); 

			if(f->mode == FUTURE_EXCLUSIVE) {
				f->state = FUTURE_EMPTY;
				f->pid = -1;
			}
			restore(mask);
			return OK;
		}
		restore(mask);
		return OK;
	}

	syscall future_set(future_t* f, char* in) {

		intmask mask;
  		mask = disable();
  		f->pid = getpid();

		char* tailelemptr; 

		if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_READY;
			tailelemptr = f->data + (f->tail * f->size); 
			memcpy(tailelemptr,in, f->size);
			f->count = f->count+1;
			f->tail = (f->tail + 1) % f->max_elems;
			resume(dequeue(f->get_queue));

			restore(mask);
			return OK;			
		}
		
		if (f->state == FUTURE_WAITING) {
			if(f->mode == FUTURE_EXCLUSIVE) {
				f->state = FUTURE_EMPTY;
				f->count = f->count+1;
				tailelemptr = f->data + (f->tail * f->size); 
				memcpy(tailelemptr,in, f->size);
				f->tail = (f->tail + 1) % f->max_elems;
				resume(dequeue(f->get_queue));
				f->pid = -1;				
			}

			if(f->mode == FUTURE_SHARED) {
				f->state = FUTURE_READY;
				resume(dequeue(f->get_queue));
			}

			if (f->mode == FUTURE_QUEUE) {
				f->state = FUTURE_READY;
				if(f->max_elems == f->count) { // queue is full
					enqueue(f->pid, f->set_queue);
					suspend(f->pid);
				} else { // queue not full
					// kprintf("step 3 or 6\n");
					tailelemptr = f->data + (f->tail * f->size); 
					memcpy(tailelemptr,in, f->size);
					f->tail = (f->tail + 1) % f->max_elems;
					f->count = f->count+1;
					resume(dequeue(f->get_queue)); 
				}
			}
			restore(mask);
			return OK;
		}

		if(f->state == FUTURE_READY) {
			if (f->mode == FUTURE_EXCLUSIVE || f->mode == FUTURE_SHARED) {
				restore(mask);
				return SYSERR;
			}
			
			if(f->max_elems == f->count) { // queue is full
				enqueue(f->pid, f->set_queue);
				suspend(f->pid);
			} else { // queue not full
				// kprintf("step 7\n");
				tailelemptr = f->data + (f->tail * f->size); 
				memcpy(tailelemptr,in, f->size);
				f->tail = (f->tail + 1) % f->max_elems;
				f->count = f->count+1;
				resume(dequeue(f->get_queue)); 

			}	
		}
		restore(mask);
		return OK;
	}
