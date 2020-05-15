/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "thread.H"

extern Scheduler* SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {

  size = 0;
  this->disk_queue =  new Queue();
}

void BlockingDisk::wait_until_ready() {

   // Console::puts("In BlockingDisk::wait_until_ready() \n"); 
    if (!is_ready()) {                                // checking if ready
        Thread *cur_thread = Thread::CurrentThread(); // store CurrentThread in a ariable
        this->disk_enqueue(cur_thread);               // add this to the queue
        SYSTEM_SCHEDULER->yield();                    // yield the CPU
    }

}

void BlockingDisk::disk_enqueue(Thread *thread) {
    this->disk_queue->enqueue(thread);
    size++;
}

bool BlockingDisk::is_ready() {
    return SimpleDisk::is_ready();    // same as the implementation in SimpleDisk class
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::read(_block_no, _buf);

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::write(_block_no, _buf);
}
