/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is similar with get_frames, 
 except that you don’t need to search for the free sequence.  You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define KB * (0x1 << 10)

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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

 ContFramePool *  ContFramePool::pools;
 ContFramePool *  ContFramePool::head;
 
/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{

    // Bitmap must fit in a single frame!
    assert(_n_frames <= FRAME_SIZE * (8/2));

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;

    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }

    // Number of frames must be "fill" the bitmap!
    assert ((nframes % 4 ) == 0);

    // 00 - FREE, 01 - HEAD, 11 - ALLOCATED, 10 - INACCESSIBLE 
    // Everything ok. Proceed to mark all bits  as 0x00 in the bitmap
    for(int i=0; i*(8/2) < nframes; i++) {
        bitmap[i] = 0x00;
    }

    // Mark the first frame as being used if it is being used
    // 0x40 - 0100 0000
    if(_info_frame_no == 0) {
        bitmap[0] = 0x40;
        nFreeFrames--;
    }

    if (ContFramePool::pools == NULL) {
        ContFramePool::pools = this;
        ContFramePool::head = this;
    } else {
        ContFramePool::pools->next = this;
        ContFramePool::pools = pools->next;
    }

    Console::puts("Frame Pool initialized\n");

}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{

    unsigned int num_frames_req = _n_frames;
    unsigned int frame_no = base_frame_no;
    unsigned int i,m;
    unsigned char mask = 0xC0; //0xC0 - 1100 0000 
    int isfree = 0,n;
/*    
    Console::puts("_n_frames : ");
    Console::puti(_n_frames);
    Console::puts(", nframes : ");
    Console::puti(nframes);
    Console::puts("\n"); 

    Console::puts("bitmap before\n");
    for (i = 0; i<5; i++) {
       Console::puti(bitmap[i]); 
       Console::puts("   ");
    }
*/

    for(i=0; i<(nframes/4); i++) {

        mask = 0xC0;
        
        for(int j=0; j<4 ; j++) {
       
            if ( (bitmap[i] & mask) == 0) {
           
		if (isfree == 0) {
		     
		     isfree = 1;
                     frame_no += i*4 + j;
                     m = i;
                     n = j;
                     num_frames_req--;

		} else {

		     num_frames_req--;

		}

            } else {

                isfree = 0;
		num_frames_req = _n_frames;
                frame_no = base_frame_no;      
      
            }

            if (num_frames_req == 0) {
                break;
            }

            mask = mask >> 2; 

        }

        if (num_frames_req == 0) {
            break;
        }

    }  

    if (num_frames_req != 0) {
       
       Console::puts("ERROR : No free frame found for size : ");
       Console::puti(_n_frames);
       Console::puts("\n");
       return 0;

    }

/*
    Console::puts("\nget frames :- m : ");
    Console::puti(m);
    Console::puts(",  n : ");
    Console::puti(n);
*/
    unsigned int num_frames = _n_frames;
    unsigned char mask1 = 0x3F; //0011 1111
    mask1 = mask1 >> (n*2) | mask1 << (8 - n*2);  //right circular bit shift  
    unsigned char mask2 = 0x40; //0100 0000
    mask2 = mask2 >> (n*2);

    bitmap[m] = ((bitmap[m] & mask1) | mask2);  //01 - HEAD
/*
    Console::puts("\nmask1 : ");
    Console::puti(mask1);
    Console::puts("\n bitmap[m] : ");
    Console::puti(bitmap[m]);
    Console::puts("\n");
*/
    num_frames--;
    n++;
    mask = 0xC0; //1100 0000
    mask = mask >> (n*2);

    while (num_frames>0 && n<4) {
     
        bitmap[m] = (bitmap[m] | mask);
        num_frames--;
        n++;
        mask = mask >> 2;
    }


    for (i=m+1; i<(nframes/4); i++) {
     
        mask = 0xC0;

        for (int j = 0; j<4; j++) {
       
             if (num_frames == 0) {
                 break;
             } 
             
             bitmap[i] = (bitmap[i] | mask);
             num_frames--;
             mask = mask >> 2;

        }

        if (num_frames == 0) {
            break;
        }

    }

/*
    Console::puts("bitmap after\n");
    for (i = 0; i<5; i++) {
       Console::puti(bitmap[i]); 
       Console::puts("   ");
    }
*/

    if (num_frames == 0) {
/*
        nFreeFrames -= _n_frames;
        Console::puts("Free frame found for size : ");
        Console::puti(_n_frames);
        Console::puts(" at ");
        Console::puti(frame_no); 
        Console::puts("\n"); 
*/
        return frame_no;   

    } else {

        Console::puts("ERROR : No free frame found for size : ");
        Console::puti(_n_frames);
        Console::puts("\n");
        return 0;
    }

}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{

    nFreeFrames -= _n_frames;
    unsigned long i,m,num_of_frames_used,num_of_bits;
    unsigned long num_of_frames = _n_frames;

    num_of_frames_used = (_base_frame_no - base_frame_no);
    num_of_bits = num_of_frames_used*2;
    m = num_of_bits / 8;
    int n = (num_of_bits % 8)/2;

/*
    Console::puts("_base_frame_no : ");
    Console::puti(_base_frame_no);
    Console::puts("\nbitmap before\n");
    for (i = m-1; i<m+65; i++) {
       Console::puti(bitmap[i]);
       Console::puts("   ");
    }
*/

    // 00 - FREE, 01 - HEAD, 11 - ALLOCATED, 10 - INACCESSIBLE 

    unsigned char mask1 = 0x80; // 1000 0000
    unsigned char mask2 = 0x3F; // 0011 1111
    unsigned char mask3 = 0x40; // 0100 0000
    mask3 = mask3 >> (n*2);
    mask2= mask2 >> (n*2) | mask2 << (8 - n*2);

    bitmap[m] = (bitmap[m] & mask2) | mask3; //01 - head of sequence
    
    n++;
    mask1 = mask1 >> (n*2);
    mask2 = mask2 >> 2 | mask2 << (8 - 2);
    num_of_frames--;

    while (n<4 && num_of_frames>0 ) {
   
         bitmap[m] = (bitmap[m] & mask2) | mask1; //10 - inaccessible
         mask1 = mask1 >> 2;
         mask2 = mask2 >> 2 | mask2 << (8 - 2);       
         num_of_frames--;
         n++;

    }

    for (i = m+1; i < m + (_n_frames/4); i++ ) {

         mask1 = 0x80;
         mask2 = 0x3F;

         for (int j = 0; j<4; j++) {

             if (num_of_frames == 0) {
                 break;
             }             

             bitmap[i] = (bitmap[i] & mask2) | mask1;
             mask1 = mask1 >> 2;
             mask2 = mask2 >> 2 | mask2 << (8 - 2);
             num_of_frames--;
        
         }

         if (num_of_frames == 0) {
             break;
         } 

    }

/*
    Console::puts("\nbitmap after\n");
    for (i = m-1; i<m+65; i++) {
       Console::puti(bitmap[i]);
       Console::puts("   ");
    }
*/

    if (num_of_frames == 0) {

        Console::puts("Marking as inaccessible successful for size : ");
        Console::puti(_n_frames);
        Console::puts(" starting at ");
        Console::puti(_base_frame_no);
        Console::puts("\n");

    } else {

        Console::puts("ERROR : Marking as inaccessible failed for size : ");
        Console::puti(_n_frames);
        Console::puts(" starting at ");
        Console::puti(_base_frame_no);
        Console::puts("\n");
    }

}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{

/*
    Console::puts("\n_first_frame_no : ");
    Console::puti(_first_frame_no);
    Console::puts("\n");
*/

    ContFramePool *cur = ContFramePool::head;
   
    while ((cur->base_frame_no + cur->nframes <= _first_frame_no) || (cur->base_frame_no > _first_frame_no) ) {
   
        if (cur->next != NULL) {
            cur = cur->next;
        } else {
            Console::puts("ERROR : This frame is not present in any pool : ");
            Console::puti(_first_frame_no);
            Console::puts("\n");
            return;
        }
	
    }

   // Console::puts("cur->base_frame_no : ");
   // Console::puti(cur->base_frame_no);

    unsigned long i,m,num_of_frames_used,num_of_bits;

    num_of_frames_used = (_first_frame_no - cur->base_frame_no);
    num_of_bits = num_of_frames_used*2;
    m = num_of_bits / 8;
    int n = (num_of_bits % 8)/2;    

    unsigned char *bits = cur->bitmap;
    unsigned char mask1 = 0xC0; // 1100 0000
    unsigned char mask2 = 0x40; // 0100 0000
    unsigned char mask  = 0x3F; // 0011 1111
    mask1 = mask1 >> (n*2);
    mask2 = mask2 >> (n*2);
    mask = mask >> (n*2) | mask << (8 - n*2);  //right circular bit shift
/*  
    Console::puts("\nRelease, m : ");
    Console::puti(m);
    Console::puts(" , n : ");
    Console::puti(n);
    Console::puts("\nbitmap before\n");
    for (i = m; i<m+10; i++) {
       Console::puti(bits[i]);
       Console::puts("   ");
    }
*/
 
    // 00 - FREE, 01 - HEAD, 11 - ALLOCATED, 10 - INACCESSIBLE 

    if ((bits[m] & mask1) == mask2) { //if HEAD 
         
         bits[m] = bits[m] & mask; 
         cur->nFreeFrames++;
         n++;
         mask1 = mask1 >> 2;
         mask = mask >> 2 | mask << (8 - 2);

         while (n<4) {
            if ((bits[m] & mask1) == mask1) {
                 bits[m] = bits[m] & mask;
                 cur->nFreeFrames++;
                 n++;
                 mask = mask >> 2 | mask << (8 - 2);
                 mask1 = mask1 >> 2;
            } else {
                 break;
            }
         }

         int c = 1;
         for (i = m+1; i < (cur->base_frame_no + cur->nframes/4); i++) {
              mask = 0x3F;
              mask1 = 0xC0; // 1100 0000
              for (int j = 0; j < 4; j++) {
                   if ((bits[i] & mask1) == mask1) {
                       bits[i] = bits[i] & mask;
                       cur->nFreeFrames++;
                       mask = mask >> 2 | mask << (8 - 2);
                       mask1 = mask1 >> 2;
                   } else {
                       Console::puts("All frames released\n");
                       c = 0;
                       break;
                   }
              } 
              if (c==0) {break;}           
         }

    } else {
        Console::puts("ERROR : This frame is not HEAD of Sequence \n");
        Console::puti(_first_frame_no);
        Console::puts("\n");
       // return;
    }
/*
    Console::puts("\nRelease end, m : ");
    Console::puti(m);
    Console::puts("\nbitmap after\n");
    for (i = m; i<m+10; i++) {
       Console::puti(bits[i]);
       Console::puts("   ");
    }
*/
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{

    // 4*4096 = 16KB frames  
    return (_n_frames / (16 KB) + (_n_frames % (16 KB) > 0 ? 1 : 0));
    
}
