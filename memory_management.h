#include "kernel/types.h"
#include "user.h"

#define NULL 0

typedef struct header {
        int block_size;
        int is_free;
        struct header* next;
} header;

int nr_pages(int size_bytes);
void * _malloc(int size);
void _free(void *ptr);
