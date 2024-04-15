#include "memory_management.h"

void* heap_base = NULL;
int total_pages = 0;
header* header_head = NULL;
header* header_tail = NULL;

int nr_pages(int size_bytes) {
    return ((size_bytes/4096) + 1);
}

void *_malloc(int size) {

    if(size == 0) return NULL;

    if(header_head == NULL) {
        heap_base = sbrk(4096 * nr_pages(size + sizeof(header)));
        if((unsigned long)heap_base == -1) return 0;
        total_pages = total_pages + nr_pages(size + sizeof(header));

        header_head = heap_base;
        header_head->block_size = size;
        header_head->is_free = 0;
        header_head->next = NULL;

        header_tail = header_head;

        return (heap_base + sizeof(header));
    }

    else if(heap_base == NULL) {
        exit(-2);
    }

    // Check if any free space without allocating a new page
    // Naive approach - the first one that fits

    header* i = header_head;

    while(i != NULL) {

        // Find the amount of space left in this block

        unsigned long free_space = 0;

        // This is the last header - if there's any space left at the end, assign with enough new pages

        if(i->next == NULL) {

            // Find how much space is left in this page

            unsigned long space_from_page_beginning = ((unsigned long)((char*)i+sizeof(header)+i->block_size) - (unsigned long)heap_base) % 4096;
            free_space = 4096 - space_from_page_beginning;

            // Check if empty
            if(i->is_free) {
              if(size > free_space) {
                unsigned long size_required = size - free_space;
                if((unsigned long)sbrk(4096*nr_pages(size_required)) == -1) return 0;
                total_pages = total_pages + nr_pages(size_required);

              }
              i->is_free = 0;
              i->block_size = size;
              return (void*)((char*)i+sizeof(header));
            }

            if(size + sizeof(header) > free_space) {
                unsigned long size_required = size + sizeof(header) - free_space;
                if((unsigned long)sbrk(4096 * nr_pages(size_required)) == -1) return 0;
                total_pages = total_pages + nr_pages(size_required);
            }



            header* new_header = (void*)((char*)i+sizeof(header)+i->block_size);

            new_header->block_size = size;
            new_header->is_free = 0;
            new_header->next = NULL;

            i->next = new_header;

            return((void*)((char*)new_header + sizeof(header)));

        }

        // This is not the last header - find nr of bytes until the next header
        else {

            free_space = (unsigned long)i->next - (unsigned long)((char*)i+sizeof(header)+i->block_size);

            // This block is free - just change the header
            if(size <= free_space && i->is_free) {
                i->is_free = 0;
                i->block_size = size;
                return (void*)((char*)i+sizeof(header));
            }

            // This block is not free - make a new block with header inside of it
            else if(size + sizeof(header) <= free_space) {

                header* new_header = (header*)((char*)i+sizeof(header)+i->block_size);
                new_header->block_size = size;
                new_header->is_free = 0;
                new_header->next = i->next;

                i->next = new_header;

                return (void*)((char*)i+2*sizeof(header)+i->block_size);
            }
        }

        i = i->next;
    }

    // Not enough free space found - allocate a new page
    // This should normally not happen, but just in case

    void* new_beginning = sbrk(4096 * nr_pages(size + sizeof(header)));
    if((unsigned long)new_beginning == -1) return 0;
    total_pages = total_pages + nr_pages(size + sizeof(header));

    header* new_header = new_beginning;
    new_header->block_size = size;
    new_header->is_free = 0;
    new_header->next = NULL;

    header_tail->next = new_header;
    header_tail = new_header;

    return (void*)((char*)new_beginning + sizeof(header));
}

void _free(void *ptr) {

    if(ptr == NULL) return;

    // Find the header corresponding to that memory block
    header* i = header_head;

    while(i != NULL) {
        if((void*)((char*)i+sizeof(header)) == ptr) break;
        i = i->next;
    }

    i->is_free = 1;
    i->block_size = 0;

    // Merge with free blocks, if any, on left and right



    //Right
    if(i->next != NULL && (i->next)->is_free) {
        i->next = (i->next)->next;

    }
    header* freed_header = i;

    //Left
    header* j = header_head;

    while(j != NULL) {
        if(j->is_free && j->next == i) {
            j->next = i->next;
            freed_header = j;
            break;
        }
        j = j->next;
    }



    // Extension task:
    if(freed_header->next == NULL) {

      void* heap_top = (void*)((char*)heap_base + 4096*total_pages);
      int empty_pages = ((char*)heap_top - (char*)freed_header)/4096;
      if(empty_pages == 0) return;
      if(freed_header == header_head) return;

      header* m = header_head;

      while(m != NULL) {

          if(m->next == freed_header) {

              m->next = NULL;
              break;
          }
          m = m->next;
      }

      if((unsigned long)sbrk(-4096*empty_pages) == -1) {
        return;
      }
      total_pages = total_pages - empty_pages;
    }
}
