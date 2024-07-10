#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    
    int parent_id = getpid();
    int pid = fork();
    int alloc_size = 256;
    char *allocated_memory = malloc(alloc_size);
    

    if(pid > 0){
        wait(0);
        printf((char*)allocated_memory);
        free(allocated_memory);
    }
    else{
        printf("Size before map_shared_pages: %d\n", sbrk(0));
        uint64 msg = map_shared_pages(parent_id, (uint64)allocated_memory, alloc_size);
        printf("Size after map_shared_pages: %d\n", sbrk(0));
        strcpy((char *)msg, "Hello daddy");
        unmap_shared_pages(msg, alloc_size);
        printf("Size after unmap_shared_pages: %d\n", sbrk(0));
        malloc(alloc_size);
        printf("Size after malloc: %d\n", sbrk(0));
    }
    exit(0);
}
