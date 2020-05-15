/*
 File: vm_pool.C
 
 Author: Dhiltom P A
 Date  : 3/10/2020
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {

    base_address = _base_address; // _base_address is the logical start address of the pool
    size         = _size;         // _size is the size of the pool in bytes
    frame_pool   = _frame_pool;   // _frame_pool points to the frame pool that provides the virtual memory pool with physical memory frames
    page_table   = _page_table;   // _page_table points to the page table that maps the logical memory references to physical addresses

    region_no    = 0;
    max_regions  = Machine::PAGE_SIZE/sizeof(region);
    regions = (struct region*)(base_address);
    page_table -> register_pool(this);

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {

    if(_size==0)
        return 0;

    unsigned long address = 0;

    unsigned long num_frames = _size / (Machine::PAGE_SIZE) ;
    unsigned long rem        = _size % (Machine::PAGE_SIZE) ;
    
    if (rem > 0)
        num_frames++;

    if(region_no == 0) { // if list is empty
        address = base_address;
        regions[region_no].base_address  = address + Machine::PAGE_SIZE ;
        regions[region_no].size          = num_frames*(Machine::PAGE_SIZE) ;
        region_no++;
        return address + Machine::PAGE_SIZE;
    }   else {
        address = regions[region_no-1].base_address + regions[region_no-1].size;
    }

    regions[region_no].base_address = address;
    regions[region_no].size         = num_frames*(Machine::PAGE_SIZE);

    region_no++;

   assert(region_no < max_regions); 
  
   Console::puts("Allocated region of memory.\n");

   return address;

}

void VMPool::release(unsigned long _start_address) {

    unsigned int cur_region = -1;

    for (unsigned int i = 0; i < max_regions; i++ ) {
 
       if(regions[i].base_address == _start_address){
            cur_region = i;
            break;
        }
    }

    assert(!(cur_region < 0));

    for (unsigned int i = 0; i < regions[cur_region].size/Machine::PAGE_SIZE; i++) {
          page_table->free_page(_start_address);
          _start_address += PageTable::PAGE_SIZE;
    }

    for (unsigned int i = cur_region; i < region_no - 1; i++) {
          regions[i] = regions[i+1];
    }

    region_no--;

    page_table->load();

    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {


    unsigned long addr_limit = base_address + size;
    unsigned long base       = base_address;
    if ((_address < addr_limit) && (_address >= base)) {
         return 1;
    }
    return 0;    

}

