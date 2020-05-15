/*
 File: scheduler.C
 
 Author: Dhiltom P A
 Date  : 3/28/2020
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
      size = 0;
      this ->disk =NULL;
      Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {

  if( (disk->is_ready()) && (disk->size != 0) && (disk != NULL)  ) {
        Thread *disk_head = disk->disk_queue->dequeue();
        disk->size--;
        Thread::dispatch_to(disk_head);
    } else {

        if (size == 0) {
            Console::puts("No thread available, so cannot yield \n");
        } else {
            size--;
            Thread* cur_thread = queue.dequeue();
            Thread::dispatch_to(cur_thread);
        }
   }

}

void Scheduler::resume(Thread * _thread) {
     queue.enqueue(_thread);
     size++;      
}

void Scheduler::add(Thread * _thread) {
     queue.enqueue(_thread);
     size++;
}

void Scheduler::terminate(Thread * _thread) {

     for (int i = 0; i < size; i++) {
         Thread * temp = queue.dequeue();
         if (_thread->ThreadId() == temp->ThreadId())
             size--;
         else
             queue.enqueue(temp);
     }
     
}

void Scheduler::add_BlockingDisk(BlockingDisk * _disk) {
	disk = _disk;
}

