#ifndef MY_MEMORY_H
#define MY_MEMORY_H

#include <stdlib.h>

void    *my_malloc(size_t size);
void    my_free(void *ptr);
void    check_memory_leaks(void);

#ifdef DEBUG_MEMORY
    #define malloc(x) my_malloc(x)
    #define free(x) my_free(x)
#endif

#endif /* MY_MEMORY_H */
