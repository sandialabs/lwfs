#include <stdio.h>

void _premain (void) __attribute__ ((constructor));
void _postmain (void) __attribute__ ((destructor));

void _premain(void){
    printf("starting\n");
    _lwfs_premain();
}

void _postmain(void){
    printf("finishing\n");
}

