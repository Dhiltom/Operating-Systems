#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   
   PageTable::kernel_mem_pool  = _kernel_mem_pool;
   PageTable::process_mem_pool = _process_mem_pool;
   PageTable::shared_size      = _shared_size;

   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{

   page_directory = (unsigned long*) (process_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long * page_table = (unsigned long*) (process_mem_pool->get_frames(1)*PAGE_SIZE);
   unsigned long page_addr = 0;
   unsigned long shared_frames = ( PageTable::shared_size / PAGE_SIZE);   
   // marking the first 4MB 
   
   for(int i = 0; i<shared_frames; i++) {
     page_table[i] = page_addr | 3;  // 3 -> 011 -> kernel + write + present
     page_addr += PAGE_SIZE;
   }

   page_directory[0] = (unsigned long) page_table | 3;
   
   for(int i = 1; i<shared_frames; i++) {
     page_directory[i] = 2;  // 2 -> 010 -> kernel + write + not present
   }

   page_directory[shared_frames-1] = (unsigned long) page_directory | 3;

   for(int i = 0 ; i < MAX_VMS; i++) {
        registered_vm_pool[i] = NULL;
   }

   vm_pool_no = 0;

   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long) page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   paging_enabled = 1;
   write_cr0( read_cr0() | 0x80000000);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
 
//   unsigned long * page_dir   = (unsigned long *) read_cr3();
   unsigned long * page_dir   = (unsigned long *) 0xFFFFF000;
   unsigned long   addr       = read_cr2();
   unsigned long   error_code = _r->err_code;  // to get the error code
   unsigned long * page_tb    = NULL;
 
   // 10 bits -> PD, 10 bits -> PT, 12 bits -> offset 
   unsigned long   page_directory_addr = addr>>22;
   unsigned long   page_table_addr     = addr>>12;
/* 
   Console::puts("read_cr2() - Address : ");
   Console::puti(addr);
   Console::puts("\n");
   Console::puts("page_directory_addr : ");
   Console::puti(page_directory_addr);
   Console::puts("\n");
   Console::puts("page_table_addr : ");
   Console::puti(page_table_addr);
   Console::puts("\n");
*/

   if ( (error_code & 1) == 0 ) { // page not present

       VMPool ** vm_pool = current_page_table->registered_vm_pool;
       int vm_idx = -1;
/*
       Console::puts("vm_pool_no : ");
       Console::puti(current_page_table->vm_pool_no);
       Console::puts("\n");
*/
       for(unsigned int i=0; i < current_page_table->vm_pool_no; i++){
            if(vm_pool[i]!=NULL){
	    if (vm_pool[i]->is_legitimate(addr)){
                    vm_idx = i;
                    break;
                }
            }
       }

       //assert(!(vm_idx < 0));

       if ( (page_dir[page_directory_addr] & 1) == 1 ) { // fault in page table 
           page_tb =  (unsigned long *) ((page_directory_addr << 12) | 0xFFC00000 );
           //& 0x03FF -> to get last 10 bits , | 3 -> 011 -> kernel + write + present
           page_tb[page_table_addr & 0x03FF] = PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3;   

       } else { // fault in page directory
           page_dir[page_directory_addr] = (unsigned long ) (process_mem_pool->get_frames(1)*PAGE_SIZE | 3);
           page_tb =  (unsigned long *) ((page_directory_addr << 12) | 0xFFC00000 );

           for (int i = 0; i<1024; i++) {
             page_tb[i] = 4; // 4-> 100 - user
           }

           page_tb[page_table_addr & 0x03FF] = PageTable::process_mem_pool->get_frames(1)*PAGE_SIZE | 3;

       }
  }
   Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{

    if (vm_pool_no < MAX_VMS) {
        registered_vm_pool[vm_pool_no++] = _vm_pool;
        Console::puts("Registered VM pool\n");
    } else {
        Console::puts("ERROR : VM POOL is already full");
    }

}

void PageTable::free_page(unsigned long _page_no) {

    unsigned long   page_directory_addr = _page_no>>22;
    unsigned long   page_table_addr     = _page_no>>12;

    unsigned long * page_tb   = (unsigned long *) ((page_directory_addr << 12) | 0xFFC00000 );
    unsigned long   frame     = page_tb[page_table_addr & 0x03FF] / (Machine::PAGE_SIZE);   
    process_mem_pool->release_frames(frame);

    page_tb[page_table_addr & 0x03FF] = 2 ; // 2 -> 010 -> kernel + write + not present

    unsigned long * cur_page_dir = (unsigned long *) read_cr3();
    write_cr3((unsigned long) cur_page_dir);

    Console::puts("freed page\n");
}

