#include <stdio.h>
#include <stdlib.h>

void start(void) __attribute__ ((constructor));
void stop(void) __attribute__ ((destructor));

void
start(void)
{
    printf("hello world!\n");
}

void
stop(void)
{
    printf("goodbye world!\n");
}
