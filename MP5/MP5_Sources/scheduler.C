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
      Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  if (size == 0) {
      Console::puts("No thread available, so cannot yield \n");
  } else {
      size--;
      Thread* cur_thread = queue.dequeue();
      Thread::dispatch_to(cur_thread); //run this thread
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
