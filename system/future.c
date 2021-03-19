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
  /*
  if (f->mode == FUTURE_SHARED) {
  	f->get_queue = newqueue();
  } 

  if (f->mode == FUTURE_QUEUE) {
  	f->get_queue = newqueue();
  	f->set_queue = newqueue();
  	f->max_elems = nelem;
  	f->count = 0;
  	f->head = 0; // index of slot at which the next element will be removed
  	f->tail = 0; //  index of next slot to which the next element will be added
  }
  */ 
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
		// !char* headelemptr = f->data + (f->head * f->size);
		if (f->pid > 0) {   
			return SYSERR;
		}
		// printf("hello\n");
		if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_WAITING;
			f->pid = getpid();
			suspend(f->pid);
			// memcpy(out, headelemptr, f->size);
			memcpy(out, f->data, f->size);
			return OK;
		}

		if (f->state == FUTURE_WAITING) { 
			// all threads waiting on a future for value should be queued 
			// in get_queue of the future
			f->pid = getpid();
			if (f->mode == FUTURE_SHARED) {
				enqueue(f->pid, f->get_queue);
				suspend(f->pid);
				// memcpy(out, headelemptr, f->size);
				memcpy(out, f->data, f->size); 
				return OK;
			}

			return SYSERR; 
		}

		if(f->state == FUTURE_READY) {
			// when it is FUTURE_READY, the next future_get() call return set value 
			// and the future becomes EMPTY
			if (f->mode == FUTURE_SHARED) {
				// memcpy(out, headelemptr, f->size);
				memcpy(out, f->data, f->size);
				return OK;
			}
			// memcpy(out, headelemptr, f->size);
			memcpy(out, f->data, f->size); //return the set value
			f->state = FUTURE_EMPTY;
			f->pid = -1;
			return OK;
		}

		return OK;
	}

	syscall future_set(future_t* f, char* in) {
		// !char* tailelemptr = f->data + (f->tail * f->size);
		
		// call future_set on empty 'future' returns to state FUTURE_READY
		if (f->state == FUTURE_EMPTY) {
			f->state = FUTURE_READY;
			// memcpy(tailelemptr,in, f->size);
			memcpy(f->data,in, f->size);
		} else if (f->state == FUTURE_READY) {
			// subsequent future_set call on FUTURE_READY should return ERROR
			return SYSERR;
		} else if (f -> state == FUTURE_WAITING) {
			// memcpy(tailelemptr,in, f->size);
			memcpy(f->data,in, f->size);
			if (f->mode == FUTURE_SHARED) { 
				while (!isempty(f->get_queue)) {
					f->state = FUTURE_READY;
					resume(dequeue(f->get_queue));
				}
			/*
			else if (f->mode == FUTURE_QUEUE) {
				if(f->max_elems == f->count) { // queue is full
					enqueue(f->pid, f->set_queue);
				} else {
					memcpy(tailelemptr,in, f->size);
					resume(dequeue(f->get_queue));
				}*/
			} 
			 else { // FUTURE_EXCLUSIVE mode
				f->state = FUTURE_EMPTY;
				resume(f->pid);
				f->pid = -1;
			}
		}

		return OK;
	}



